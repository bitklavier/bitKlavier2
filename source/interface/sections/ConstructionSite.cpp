//
// Created by Davis Polito on 1/17/24.
//

#include "ConstructionSite.h"
#include "Preparations.h"
#include "SampleLoadManager.h"
#include "sound_engine.h"
#include "synth_gui_interface.h"
#include "open_gl_line.h"

ConstructionSite::ConstructionSite(const juce::ValueTree &v, juce::UndoManager &um, OpenGlWrapper &open_gl,
                                   SynthGuiData *data) : SynthSection("Construction Site"),
                                                        prep_list(*data->synth->preparationList.get()),
                                                         undo(um),
                                                         open_gl(open_gl),
                                                         cableView(*this),
                                                         modulationLineView(*this),
                                                         preparationSelector(*this), parent(v)
//_line(std::make_shared<OpenGlLine>(nullptr,nullptr,nullptr))
{
    //setWantsKeyboardFocus(true);
    addKeyListener(this);
    setSkinOverride(Skin::kConstructionSite);
    setInterceptsMouseClicks(false, true);
    //data->synth->getEngine()->addChangeListener(this);

    cableView.setAlwaysOnTop(true);
    addSubSection(&cableView);
    cableView.setAlwaysOnTop(true);
    addSubSection(&modulationLineView);
    modulationLineView.setAlwaysOnTop(false);
    prep_list.addListener(this);
    // prep_list.addChangeListener(this);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeDirect, DirectPreparation::createDirectSection);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeKeymap, KeymapPreparation::createKeymapSection);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeVST, PluginPreparation::createPluginSection);
}



void ConstructionSite::reset() {
    DBG("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
    if (_parent == nullptr)
        return;
        if (_parent->getSynth() != nullptr) {
            _parent->getSynth()->getEngine()->resetEngine();
            prep_list.setValueTree(_parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO).getChildWithName(
                IDs::PREPARATIONS));
        }

        cableView.reset();
        modulationLineView.reset();
}
PreparationSection *ConstructionSite::getComponentForPlugin(juce::AudioProcessorGraph::NodeID nodeID) const {
    {
        for (auto &comp : plugin_components)
            if (comp->pluginID == nodeID)
                return comp.get();

        return nullptr;
    }
}


void ConstructionSite::createWindow(juce::AudioProcessorGraph::Node* node, PluginWindow::Type type) {
    jassert (node != nullptr);

#if JUCE_IOS || JUCE_ANDROID
    closeAnyOpenPluginWindows();
#else
    for (auto* w : activePluginWindows)
        if (w->node.get() == node && w->type == type)
            w->toFront(true);
#endif

    if (auto* processor = node->getProcessor())
    {
        if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*> (processor))
        {
            auto description = plugin->getPluginDescription();



            auto window = activePluginWindows.add (new PluginWindow (node, type, activePluginWindows));
            window->toFront(true);
        }
    }

}

void ConstructionSite::moduleAdded(PluginInstanceWrapper* wrapper) {
    auto * interface = findParentComponentOfClass<SynthGuiInterface>();
    auto s = nodeFactory.CreateObject(wrapper->state.getProperty(IDs::type), wrapper->state,interface );
    addSubSection(s.get());
    Skin default_skin;
    s->setSkinValues(default_skin, false);
    s->setDefaultColor();
    s->setSizeRatio(size_ratio_);
    s->setCentrePosition(s->x, s->y);
    s->setSize(s->width, s->height);


    s->addSoundSet(&interface->sampleLoadManager->samplerSoundset);
    if (!interface->sampleLoadManager->samplerSoundset.empty()) {
        s->addSoundSet(
            &interface->sampleLoadManager->samplerSoundset[interface->sampleLoadManager->globalSoundset_name],
            &interface->sampleLoadManager->samplerSoundset[interface->sampleLoadManager->globalHammersSoundset_name],
            &interface->sampleLoadManager->samplerSoundset[interface->sampleLoadManager->globalReleaseResonanceSoundset_name],
            &interface->sampleLoadManager->samplerSoundset[interface->sampleLoadManager->globalPedalsSoundset_name]);
    }
    s->selectedSet = &(preparationSelector.getLassoSelection());
    preparationSelector.getLassoSelection().addChangeListener(s.get());
    s->addListener(&cableView);
    s->addListener(&modulationLineView);
    s->addListener(this);
    s->setNodeInfo();
    plugin_components.push_back(std::move(s));
}

void ConstructionSite::renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate)
{
    juce::ScopedLock lock(open_gl_critical_section_);
    SynthSection::renderOpenGlComponents(open_gl, animate);

}


void ConstructionSite::removeModule(PluginInstanceWrapper* wrapper){
    int index = -1;
    for (int i=0; i<plugin_components.size(); i++){
        if (plugin_components[i]->pluginID == wrapper->node_id){
            index = i;
            break;
        }
    }
    if (index == -1) jassertfalse;

    //cleanup
    preparationSelector.getLassoSelection().removeChangeListener (plugin_components[index].get());
    //cleanup opengl
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        removeSubSection (plugin_components[index].get());
    }

    //delete opengl
    plugin_components[index]->destroyOpenGlComponents (open_gl);
    //delete heap memory
    plugin_components.erase(plugin_components.begin()+index);
DBG("moduleRemoved");
}

ConstructionSite::~ConstructionSite(void) {
    removeMouseListener(&cableView);
    removeChildComponent(&selectorLasso);
}

void ConstructionSite::paintBackground(juce::Graphics &g) {
    paintBody(g);
    paintChildrenBackgrounds(g);
}

void ConstructionSite::resized() {
    // Get the current local bounds of this component
    auto bounds = getLocalBounds();

    // Debug log the bounds
    DBG("Local Bounds: " + bounds.toString());

    cableView.setBounds(getLocalBounds());
    cableView.updateCablePositions();
    modulationLineView.setBounds(getLocalBounds());

    // _line->setBounds(get);

    SynthSection::resized();
}

void ConstructionSite::redraw(void) {
    draw();
}

bool ConstructionSite::keyPressed(const juce::KeyPress &k, juce::Component *c) {
    int code = k.getKeyCode();
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (code == 68) //D Direct
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeDirect, nullptr);
        t.setProperty(IDs::width, 245, nullptr);
        t.setProperty(IDs::height, 125, nullptr);
        t.setProperty(IDs::x, lastX - 245 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 125 / 2, nullptr);
        prep_list.appendChild(t,  interface->getUndoManager());
    } else if (code == 78) // N nostalgic
    {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeNostalgic, nullptr);
        // t.setProperty(IDs::width, 260, nullptr);
        // t.setProperty(IDs::height, 132, nullptr);
        // t.setProperty(IDs::x, lastX - (260 / 2), nullptr);
        // t.setProperty(IDs::y, lastY - (132 / 2), nullptr);
        //
        // prep_list.appendChild(t,  nullptr);
    } else if (code == 75) // K Keymap
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeKeymap, nullptr);
        t.setProperty(IDs::width, 185, nullptr);
        t.setProperty(IDs::height, 105, nullptr);
        t.setProperty(IDs::x, lastX - 185 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 105 / 2, nullptr);
        prep_list.appendChild(t,  nullptr);
    } else if (code == 82) // R resonance
    {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeResonance, nullptr);
        // t.setProperty(IDs::width, 260, nullptr);
        // t.setProperty(IDs::height, 132, nullptr);
        // t.setProperty(IDs::x, lastX - 260 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 132 / 2, nullptr);
        // prep_list.appendChild(t,  nullptr);
    } else if (code == 83) // S synchronic
    {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeSynchronic, nullptr);
        // t.setProperty(IDs::width, 260, nullptr);
        // t.setProperty(IDs::height, 132, nullptr);
        // t.setProperty(IDs::x, lastX - 260 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 132 / 2, nullptr);
        // prep_list.appendChild(t,  nullptr);
    } else if (code == 66) // B blendronic
    {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeBlendronic, nullptr);
        // t.setProperty(IDs::width, 260, nullptr);
        // t.setProperty(IDs::height, 132, nullptr);
        // t.setProperty(IDs::x, lastX - 260 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 132 / 2, nullptr);
        // prep_list.appendChild(t,  nullptr);
    } else if (code == 77) // M tempo
    {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTempo, nullptr);
        // t.setProperty(IDs::width, 132, nullptr);
        // t.setProperty(IDs::height, 260, nullptr);
        // t.setProperty(IDs::x, lastX - 132 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 260 / 2, nullptr);
        // prep_list.appendChild(t,  nullptr);
    } else if (code == 84) // T tuning
    {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTuning, nullptr);
        // t.setProperty(IDs::width, 125, nullptr);
        // t.setProperty(IDs::height, 245, nullptr);
        // t.setProperty(IDs::x, lastX - 125 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 245 / 2, nullptr);
        // prep_list.appendChild(t,  nullptr);
    } else if (k.getTextCharacter() == 'c') {
        // juce::ValueTree t(IDs::PREPARATION);
        //
        // t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeModulation, nullptr);
        // t.setProperty(IDs::width, 100, nullptr);
        // t.setProperty(IDs::height, 100, nullptr);
        // t.setProperty(IDs::x, lastX - 100 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 100 / 2, nullptr);
        // prep_list.appendChild(t,  nullptr);
    }
    return true;
}

void ConstructionSite::itemIsBeingDragged(BKItem *thisItem, const juce::MouseEvent &e) {
    repaint();
}

void ConstructionSite::draw(void) {
    //    int offset = 0;
    //    for (auto item : processor.currentPiano->getItems())
    //    {
    //        addExistingItem(item);
    //    }

    repaint();
}

void ConstructionSite::prepareItemDrag(BKItem *item, const juce::MouseEvent &e, bool center) {
    //    if (center)
    //    {
    //        float X = item->getPosition().getX() + item->getWidth() / 2.0f;
    //        float Y = item->getPosition().getY() + item->getHeight() / 2.0f;
    //        juce::Point<float>pos(X,Y);
    //        juce::MouseEvent newEvent = e.withNewPosition(pos);
    //
    //        item->prepareDrag(newEvent);
    //    }
    //    else
    //    {
    //        item->prepareDrag(e);
    //    }
}

void ConstructionSite::mouseMove(const juce::MouseEvent &eo) {
    juce::MouseEvent e = eo.getEventRelativeTo(this);
    //juce::MouseEvent a = eo.getEventRelativeTo(this);
    //    if (e.x != lastEX) lastX = e.x;
    //
    //    if (e.y != lastEY) lastY = e.y;

    //    lastEX = eo.x;
    //    lastEY = eo.y;mouse = e.position;
    mouse = e.position;
    lastX = e.x;
    lastY = e.y;
    //DBG("screen" + juce::String(lastX) + " " + juce::String(lastY));
    //DBG("global" + juce::String(e.getMouseDownX()) +" " + juce::String(e.getMouseDownY()));
    //DBG("site" + juce::String(a.getMouseDownX()) +" " + juce::String(a.getMouseDownY()));
    if (connect) {
        repaint();
    }
}

void ConstructionSite::mouseDown(const juce::MouseEvent &eo) {
    juce::MouseEvent e = eo.getEventRelativeTo(this);
    auto itemToSelect = dynamic_cast<PreparationSection *>(e.originalComponent->getParentComponent());
    if (itemToSelect == nullptr) {
        preparationSelector.getLassoSelection().deselectAll();
        //DBG ("mousedown empty space");
    }

    addChildComponent(selectorLasso);
    selectorLasso.beginLasso(e, &preparationSelector);
    //////Fake drag so the lasso will select anything we click and drag////////
    auto thisPoint = e.getPosition();
    thisPoint.addXY(10, 10);
    selectorLasso.dragLasso(e.withNewPosition(thisPoint));

    held = false;

    lastX = eo.x;
    lastY = eo.y;

    mouse = e.position;

    // // This must happen before the right-click menu or the menu will close
    // grabKeyboardFocus();

    if (e.mods.isPopupMenu()) {
        _parent = findParentComponentOfClass<SynthGuiInterface>();
          auto callback = [=](int selection,int index) {handlePluginPopup(selection,index);};
    auto cancel = [=]() {

    };
        showPopupSelector(this, e.getPosition(), _parent->getPluginPopupItems(),callback,cancel);
    }
    // Stop trying to make a connection on blank space click
    connect = false;
}
void ConstructionSite::handlePluginPopup(int selection, int index) {
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (selection < bitklavier::BKPreparationType::PreparationTypeVST) {
        juce::ValueTree t(IDs::PREPARATION);
        t.setProperty(IDs::type, static_cast<bitklavier::BKPreparationType>(selection), nullptr);
        t.setProperty(IDs::width, 132, nullptr);
        t.setProperty(IDs::height, 260, nullptr);
        t.setProperty(IDs::x, lastX - 132 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 260 / 2, nullptr);
        prep_list.appendChild(t,  interface->getUndoManager());
    } else {
        _parent = findParentComponentOfClass<SynthGuiInterface>();
        juce::ValueTree t(IDs::PREPARATION);
        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeVST, nullptr);
        t.setProperty(IDs::width, 245, nullptr);
        t.setProperty(IDs::height, 125, nullptr);
        t.setProperty(IDs::x, lastX - 245 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 125 / 2, nullptr);

        auto desc = _parent->userPreferences->userPreferences->pluginDescriptionsAndPreference[selection - static_cast<int>(bitklavier::BKPreparationType::PreparationTypeVST)];
        juce::ValueTree plugin = juce::ValueTree::fromXml(*desc.pluginDescription.createXml());
        t.addChild(plugin,-1, nullptr);
        prep_list.addPlugin(desc.pluginDescription,t);

    }


}


void ConstructionSite::mouseUp(const juce::MouseEvent &eo) {
    //inLasso = false;
    //    DBG ("mouseupconst");
    selectorLasso.endLasso();
    removeChildComponent(&selectorLasso);
    if (edittingComment)
        return;



    juce::MouseEvent e = eo.getEventRelativeTo(this);
    cableView.mouseUp(e);
    // Do nothing on right click mouse up
    if (e.mods.isRightButtonDown())
        return;
#if JUCE_MAC
    // Do nothing on ctrl click mouse up
    if (e.mods.isCtrlDown())
        return;
#endif

    //    DBG(e.x);
    //    DBG(e.y);
    //    DBG(eo.x);
    //    DBG(eo.y);
    //getParentComponent()->grabKeyboardFocus();

    //grabKeyboardFocus();

    //    if (lassoSelection.getNumSelected())
    //    {
    //        for (auto item : lassoSelection)
    //        {
    //            graph->select(item);
    //        }
    //
    //        lassoSelection.deselectAll();
    //
    //        return;
    //    }

    // Uncomment for press-hold-release connection behavior when using CMD shortcut
    // Otherwise make connections with CMD+click and click on target
    //  if (connect) makeConnection(e.x, e.y, e.mods.isAltDown());

    //    bool shouldStore = false;
    //    for (auto item : graph->getSelectedItems())
    //    {
    //        if (item->isDragging)
    //        {
    //            item->isDragging = false;
    //            shouldStore = true;
    //        }
    //    }
    //    if (shouldStore && e.getDistanceFromDragStart() > 0) processor.saveGalleryToHistory("Move");

    redraw();
}

void ConstructionSite::mouseDrag(const juce::MouseEvent &e) {
    if (edittingComment)
        return;

    cableView.mouseDrag(e);
    // Do nothing on right click drag
    if (e.mods.isRightButtonDown())
        return;

    if (e.mods.isShiftDown()) {
        selectorLasso.toFront(false);
        selectorLasso.dragLasso(e);
    }

    repaint();
}

void ConstructionSite::updateComponents() {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    for (int i = prep_list.size(); --i >= 0;) {
        //            if (parent->getSynth()->getNodeForId(objects.getUnchecked(i)->pluginID) == nullptr) {
        //                parent.removeChild(objects.getUnchecked(i)->parent, nullptr);
        //            }
    }

    //    for (auto* fc : objects)
    //        fc->update();

    cableView.updateComponents();
    //        for (auto* f : graph.graph.getNodes())
    //        {
    //            if (getComponentForPlugin (f->nodeID) == nullptr)
    //            {
    //                auto* comp = nodes.add (new PluginComponent (*this, f->nodeID));
    //                addAndMakeVisible (comp);
    //                comp->update();
    //            }
    //        }
}
