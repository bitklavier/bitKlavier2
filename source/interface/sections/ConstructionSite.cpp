//
// Created by Davis Polito on 1/17/24.
//

#include "ConstructionSite.h"
#include "Preparations.h"
#include "SampleLoadManager.h"
#include "sound_engine.h"
#include "synth_gui_interface.h"
#include "open_gl_line.h"

ConstructionSite::ConstructionSite(juce::ValueTree &v, juce::UndoManager &um, OpenGlWrapper &open_gl,
                                   SynthGuiData *data) : SynthSection("Construction Site"),
                                                         tracktion::engine::ValueTreeObjectList<PreparationSection>(v),
                                                         undo(um),
                                                         open_gl(open_gl),
                                                         cableView(*this),
                                                         modulationLineView(*this),
                                                         preparationSelector(*this)
//_line(std::make_shared<OpenGlLine>(nullptr,nullptr,nullptr))
{
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    setSkinOverride(Skin::kConstructionSite);
    setInterceptsMouseClicks(false, true);
    //addAndMakeVisible (cableView);
    data->synth->getEngine()->addChangeListener(this);
    //addMouseListener (&cableView, true);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeDirect, DirectPreparation::createDirectSection);
    //    prepFactory.Register (bitklavier::BKPreparationType::PreparationTypeNostalgic, NostalgicPreparation::createNostalgicSection);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeKeymap, KeymapPreparation::createKeymapSection);
    //    prepFactory.Register (bitklavier::BKPreparationType::PreparationTypeResonance, ResonancePreparation::createResonanceSection);
    //    prepFactory.Register (bitklavier::BKPreparationType::PreparationTypeSynchronic, SynchronicPreparation::createSynchronicSection);
    //    prepFactory.Register (bitklavier::BKPreparationType::PreparationTypeBlendronic, BlendronicPreparation::createBlendronicSection);
    //    prepFactory.Register (bitklavier::BKPreparationType::PreparationTypeTempo, TempoPreparation::createTempoSection);
    prepFactory.Register (bitklavier::BKPreparationType::PreparationTypeTuning, TuningPreparation::createTuningSection);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeModulation,
                         ModulationPreparation::createModulationSection);
    //    cableView.toBack();
    cableView.setAlwaysOnTop(true);
    addSubSection(&cableView);
    cableView.setAlwaysOnTop(true);
    addSubSection(&modulationLineView);
    modulationLineView.setAlwaysOnTop(false);
    //    modulationLineView.setAlwaysOnTop(true);
    //    addOpenGlComponent(_line);
}

void ConstructionSite::valueTreeParentChanged(juce::ValueTree &changed) {
    //    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    //    ///changed.copyPropertiesAndChildrenFrom(_parent->getSynth()->getValueTree(),nullptr);
    //
    //    parent = _parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO);
}

void ConstructionSite::valueTreeRedirected(juce::ValueTree &) {
    SynthGuiInterface *interface = findParentComponentOfClass<SynthGuiInterface>();

    deleteAllObjects();
    rebuildObjects();
    for (auto object: objects) {
        newObjectAdded(object);
    }
} // may need to add handling if this is hit

void ConstructionSite::deleteObject(PreparationSection *at) {
    if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {
        SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();

        //safe to do on message thread because we have locked processing if this is called
        at->setVisible(false);
        open_gl.context.executeOnGLThread([this, &at](juce::OpenGLContext &openGLContext) {
                                              this->removeSubSection(at);
                                          },
                                          false);
    } else
        delete at;
}

PreparationSection *ConstructionSite::createNewObject(const juce::ValueTree &v) {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    //must use auto * so that it doesnt create a copy and call this constructor twice


    auto *s = prepFactory.CreateObject((int) v.getProperty(IDs::type), v, parent);

    addSubSection(s);
    Skin default_skin;
    s->setSkinValues(default_skin, false);
    s->setDefaultColor();
    s->setSizeRatio(size_ratio_);
    s->setCentrePosition(s->x, s->y);
    s->setSize(s->width, s->height);


    s->addSoundSet(&parent->sampleLoadManager->samplerSoundset);
    if (!parent->sampleLoadManager->samplerSoundset.empty()) {
        s->addSoundSet(
            &parent->sampleLoadManager->samplerSoundset[parent->sampleLoadManager->globalSoundset_name],
            &parent->sampleLoadManager->samplerSoundset[parent->sampleLoadManager->globalHammersSoundset_name],
            &parent->sampleLoadManager->samplerSoundset[parent->sampleLoadManager->globalReleaseResonanceSoundset_name],
            &parent->sampleLoadManager->samplerSoundset[parent->sampleLoadManager->globalPedalsSoundset_name]);
    }
    s->selectedSet = &(preparationSelector.getLassoSelection());
    preparationSelector.getLassoSelection().addChangeListener(s);
    s->addListener(&cableView);
    s->addListener(&modulationLineView);

    //only add non modulations to the modulation line view in order to be able to check what is under the mouse that isnt a modprep
    //this is cheap and hacky. should setup better down the line. possibly using a key modifier
    //    if(dynamic_cast<ModulationPreparation*>(s) == nullptr)
    //        modulationLineView.addAndMakeVisible(s);
    // so that the component dragging doesnt actually move the componentn
    return s;
}

void ConstructionSite::reset() {
    DBG("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
    if (_parent == nullptr)
        return;
        if (_parent->getSynth() != nullptr) {
            _parent->getSynth()->getEngine()->resetEngine();
            parent = _parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO).getChildWithName(
                IDs::PREPARATIONS);
        }

        DBG("exit");
        cableView.reset();
        modulationLineView.reset();
}

void ConstructionSite::newObjectAdded(PreparationSection *object) {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    parent->addProcessor(object);
}

ConstructionSite::~ConstructionSite(void) {
    removeMouseListener(&cableView);
    removeChildComponent(&selectorLasso);
    freeObjects();
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

    if (code == 68) //D Direct
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeDirect, nullptr);
        t.setProperty(IDs::width, 245, nullptr);
        t.setProperty(IDs::height, 125, nullptr);
        t.setProperty(IDs::x, lastX - 245 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 125 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (code == 78) // N nostalgic
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeNostalgic, nullptr);
        t.setProperty(IDs::width, 260, nullptr);
        t.setProperty(IDs::height, 132, nullptr);
        t.setProperty(IDs::x, lastX - (260 / 2), nullptr);
        t.setProperty(IDs::y, lastY - (132 / 2), nullptr);

        parent.addChild(t, -1, nullptr);
    } else if (code == 75) // K Keymap
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeKeymap, nullptr);
        t.setProperty(IDs::width, 114, nullptr);
        t.setProperty(IDs::height, 76, nullptr);
        t.setProperty(IDs::x, lastX - 114 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 76 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (code == 82) // R resonance
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeResonance, nullptr);
        t.setProperty(IDs::width, 260, nullptr);
        t.setProperty(IDs::height, 132, nullptr);
        t.setProperty(IDs::x, lastX - 260 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 132 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (code == 83) // S synchronic
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeSynchronic, nullptr);
        t.setProperty(IDs::width, 260, nullptr);
        t.setProperty(IDs::height, 132, nullptr);
        t.setProperty(IDs::x, lastX - 260 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 132 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (code == 66) // B blendronic
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeBlendronic, nullptr);
        t.setProperty(IDs::width, 260, nullptr);
        t.setProperty(IDs::height, 132, nullptr);
        t.setProperty(IDs::x, lastX - 260 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 132 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (code == 77) // M tempo
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTempo, nullptr);
        t.setProperty(IDs::width, 132, nullptr);
        t.setProperty(IDs::height, 260, nullptr);
        t.setProperty(IDs::x, lastX - 132 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 260 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (code == 84) // T tuning
    {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTuning, nullptr);
        t.setProperty(IDs::width, 132, nullptr);
        t.setProperty(IDs::height, 260, nullptr);
        t.setProperty(IDs::x, lastX - 132 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 260 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
    } else if (k.getTextCharacter() == 'c') {
        juce::ValueTree t(IDs::PREPARATION);

        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeModulation, nullptr);
        t.setProperty(IDs::width, 100, nullptr);
        t.setProperty(IDs::height, 100, nullptr);
        t.setProperty(IDs::x, lastX - 100 / 2, nullptr);
        t.setProperty(IDs::y, lastY - 100 / 2, nullptr);
        parent.addChild(t, -1, nullptr);
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

    // This must happen before the right-click menu or the menu will close
    grabKeyboardFocus();

    //        processor.gallery->setGalleryDirty(true);

    // Right mouse click (must be first so right click overrides other mods)
    if (e.mods.isRightButtonDown()) {
        //            if (!itemToSelect->getSelected())
        //            {
        //                graph->deselectAll();
        //                graph->select(itemToSelect);
        //            }
        //            if (processor.wrapperType == juce::AudioPluginInstance::wrapperType_Standalone)
        //            {
        //                getEditMenuStandalone(&buttonsAndMenusLAF, graph->getSelectedItems().size(), false, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
        //            else
        //            {
        //                getEditMenu(&buttonsAndMenusLAF, graph->getSelectedItems().size(), false, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
    }
    // Control click (same as right click on Mac)
#if JUCE_MAC
    else if (e.mods.isCtrlDown()) {
        //            if (!itemToSelect->getSelected())
        //            {
        //                graph->deselectAll();
        //                graph->select(itemToSelect);
        //            }
        //            if (processor.wrapperType == juce::AudioPluginInstance::wrapperType_Standalone)
        //            {
        //                getEditMenuStandalone(&buttonsAndMenusLAF, graph->getSelectedItems().size(), false, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
        //            else
        //            {
        //                getEditMenu(&buttonsAndMenusLAF, graph->getSelectedItems().size(), false, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
    }
#endif
    else if (e.mods.isShiftDown()) {
        //            // also select this item
        //            if (itemToSelect != nullptr)
        //            {
        //                if (!itemToSelect->getSelected())   graph->select(itemToSelect);
        //                else                                graph->deselect(itemToSelect);
        //            }
        //
        //            for (auto item : graph->getSelectedItems())
        //            {
        //                prepareItemDrag(item, e, true);
        //            }
    } else if (e.mods.isAltDown()) {
        //            // make a connection and look to make another
        //            if (connect) makeConnection(e.x, e.y, true);
        //
        //            // Copy and drag
        //            if (!itemToSelect->getSelected())
        //            {
        //                graph->deselectAll();
        //                graph->select(itemToSelect);
        //            }
    }
    // Command click
    else if (e.mods.isCommandDown()) {
        //            if (!itemToSelect->getSelected())
        //            {
        //                graph->deselectAll();
        //                graph->select(itemToSelect);
        //            }
        //            if (connect) makeConnection(e.x, e.y);
        //            else startConnection(e.x, e.y);
    }
    // Unmodified left mouse click
    else {
        //            if (connect) makeConnection(e.x, e.y);
        //
        //            if (!itemToSelect->getSelected())
        //            {
        //                graph->deselectAll();
        //                graph->select(itemToSelect);
        //            }
        //
        //            for (auto item : graph->getSelectedItems())
        //            {
        //                prepareItemDrag(item, e, true);
        //            }
    }

    // Clicking on blank graph space

    if (e.mods.isRightButtonDown()) {
        //            if (processor.wrapperType == juce::AudioPluginInstance::wrapperType_Standalone)
        //            {
        //                getEditMenuStandalone(&buttonsAndMenusLAF, graph->getSelectedItems().size(), true, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
        //            else
        //            {
        //                getEditMenu(&buttonsAndMenusLAF, graph->getSelectedItems().size(), true, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
    }
#if JUCE_MAC
    else if (e.mods.isCtrlDown()) {
        //            if (processor.wrapperType == juce::AudioPluginInstance::wrapperType_Standalone)
        //            {
        //                getEditMenuStandalone(&buttonsAndMenusLAF, graph->getSelectedItems().size(), true, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
        //            else
        //            {
        //                getEditMenu(&buttonsAndMenusLAF, graph->getSelectedItems().size(), true, true).showMenuAsync
        //                    (juce::PopupMenu::Options(), juce::ModalCallbackFunction::forComponent (editMenuCallback, this) );
        //            }
    }
#endif
    else {
        //            if (!e.mods.isShiftDown())
        //            {
        //                graph->deselectAll();
        //            }
        //            lassoSelection.deselectAll();
        //
        //            lasso = std::make_unique<juce::LassoComponent<BKItem*>>();
        //            addAndMakeVisible(*lasso);
        //
        //            lasso->setAlpha(0.5);
        //            lasso->setColour(juce::LassoComponent<BKItem*>::ColourIds::lassoFillColourId, juce::Colours::lightgrey);
        //            lasso->setColour(juce::LassoComponent<BKItem*>::ColourIds::lassoOutlineColourId, juce::Colours::antiquewhite);
        //
        //            lasso->beginLasso(eo, this);
        //            inLasso = true;
    }
    // Stop trying to make a connection on blank space click
    connect = false;
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

    grabKeyboardFocus();

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
    for (int i = objects.size(); --i >= 0;) {
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
