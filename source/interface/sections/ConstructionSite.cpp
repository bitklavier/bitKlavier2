//
// Created by Davis Polito on 1/17/24.
//


#include "ConstructionSite.h"
#include "Preparations.h"
#include "SampleLoadManager.h"
#include "sound_engine.h"
#include "synth_gui_interface.h"
#include "open_gl_line.h"
#include "tracktion_ValueTreeUtilities.h"

ConstructionSite::ConstructionSite(const juce::ValueTree &v, juce::UndoManager &um, OpenGlWrapper &open_gl,
                                   SynthGuiData *data, juce::ApplicationCommandManager &_manager)
                                                        : SynthSection("Construction Site"),
                                                        prep_list(data->synth->getActivePreparationList()),
                                                         undo(um),
                                                         open_gl(open_gl),
                                                         cableView(*this, um,data),
                                                         modulationLineView(*this, um,data),
                                                         preparationSelector(*this), parent(v),
                                                        commandManager (_manager),
                                                        gainProcessor (*data->synth, v)
//_line(std::make_shared<OpenGlLine>(nullptr,nullptr,nullptr))
{
    commandManager.registerAllCommandsForTarget (this);



    setWantsKeyboardFocus(true);
    //addKeyListener(this);
    setSkinOverride(Skin::kConstructionSite);
    setInterceptsMouseClicks(false, true);
    //data->synth->getEngine()->addChangeListener(this);
    data->synth->setGainProcessor(&gainProcessor);

    cableView.setAlwaysOnTop(true);
    addSubSection(&cableView);
    cableView.setAlwaysOnTop(true);
    addSubSection(&modulationLineView);
    modulationLineView.setAlwaysOnTop(false);
    prep_list->addListener(this);

    // prep_list->addChangeListener(this);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeDirect, DirectPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeBlendronic, BlendronicPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeSynchronic, SynchronicPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeKeymap, KeymapPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeVST, PluginPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeModulation, ModulationPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeTuning,TuningPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeReset,ResetPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeMidiFilter, MidiFilterPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeMidiTarget, MidiTargetPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypePianoMap, PianoSwitchPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeTempo, TempoPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeNostalgic, NostalgicPreparation::create);
    nodeFactory.Register(bitklavier::BKPreparationType::PreparationTypeResonance, ResonancePreparation::create);

}

// Define your command IDs
enum CommandIDs {
    direct = 0x0613,
    nostalgic = 0x0614,
    keymap = 0x0615,
    resonance = 0x0616,
    synchronic = 0x0617,
    blendronic = 0x0618,
    tempo = 0x0619,
    tuning = 0x0620,
    modulation = 0x0621,
    deletion = 0x0622,
    resetMod = 0x0623,
    midifilter = 0x0624,
    miditarget = 0x0625,
    pianoswitch = 0x0626
};

void ConstructionSite::getAllCommands(juce::Array<juce::CommandID> &commands) {
    commands.addArray({direct, nostalgic, keymap, resonance, synchronic, tuning, blendronic, tempo, modulation, deletion, resetMod, midifilter, miditarget, pianoswitch});
}
void ConstructionSite::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo &info)
{
    switch (id) {
        case direct:
            info.setInfo("Direct", "Create Direct Preparation", "Edit", 0);
            info.addDefaultKeypress('d', juce::ModifierKeys::noModifiers);
            break;
        case nostalgic:
            info.setInfo("Nostalgic", "Create Nostalgic Preparation", "Edit", 0);
            info.addDefaultKeypress('n', juce::ModifierKeys::noModifiers);
            break;
        case keymap:
            info.setInfo("Keymap", "Create Keymap Preparation", "Edit", 0);
            info.addDefaultKeypress('k', juce::ModifierKeys::noModifiers);
            break;
        case resonance:
            info.setInfo("Resonance", "Create Resonance Preparation", "Edit", 0);
            info.addDefaultKeypress('r', juce::ModifierKeys::noModifiers);
            break;
        case synchronic:
            info.setInfo("Synchronic", "Create Synchronic Preparation", "Edit", 0);
            info.addDefaultKeypress('s', juce::ModifierKeys::noModifiers);
            break;
        case blendronic:
            info.setInfo("Blendronic", "Create Blendronic Preparation", "Edit", 0);
            info.addDefaultKeypress('b', juce::ModifierKeys::noModifiers);
            break;
        case tempo:
            info.setInfo("Tempo", "Create Tempo Preparation", "Edit", 0);
            info.addDefaultKeypress('m', juce::ModifierKeys::noModifiers);
            break;
        case tuning:
            info.setInfo("Tuning", "Create Tuning Preparation", "Edit", 0);
            info.addDefaultKeypress('t', juce::ModifierKeys::noModifiers);
            break;
        case modulation:
            info.setInfo("Modulation", "Create Modulation", "Edit", 0);
            info.addDefaultKeypress('c', juce::ModifierKeys::noModifiers);
            break;
        case deletion:
            info.setInfo("Deletion", "Deletes Preparation", "Edit", 0);
            info.addDefaultKeypress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers);
            break;
        case resetMod:
            info.setInfo("Reset", "Create Reset Preparation", "Edit", 0);
            info.addDefaultKeypress('\\', juce::ModifierKeys::noModifiers);
            break;
        case midifilter:
            info.setInfo("Midifilter", "Create Midifilter Preparation", "Edit", 0);
            info.addDefaultKeypress('f', juce::ModifierKeys::shiftModifier);
            break;
        case miditarget:
            info.setInfo("Miditarget", "Create Miditarget Preparation", "Edit", 0);
            info.addDefaultKeypress('t', juce::ModifierKeys::shiftModifier);
            break;
        case pianoswitch:
            info.setInfo("Pianoswitch", "Create Pianoswitch Preparation", "Edit", 0);
            info.addDefaultKeypress('p', juce::ModifierKeys::noModifiers);
            break;
    }
}

bool ConstructionSite::perform(const InvocationInfo &info) {
        switch (info.commandID) {
            case direct:
            {
                juce::ValueTree t(IDs::direct);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeDirect, nullptr);
                t.setProperty(IDs::width, 245, nullptr);
                t.setProperty(IDs::height, 125, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
                //t.setProperty(IDs::y, lastY - 125 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case nostalgic:
            {
                juce::ValueTree t(IDs::nostalgic);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeNostalgic, nullptr);
                t.setProperty(IDs::width, 245, nullptr);
                t.setProperty(IDs::height, 125, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case keymap:
            {
                juce::ValueTree t(IDs::keymap);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeKeymap, nullptr);
                t.setProperty(IDs::width, 185, nullptr);
                t.setProperty(IDs::height, 105, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case resonance:
            {
                juce::ValueTree t(IDs::resonance);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeResonance, nullptr);
                t.setProperty(IDs::width, 245, nullptr);
                t.setProperty(IDs::height, 125, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case synchronic:
            {
                 juce::ValueTree t(IDs::synchronic);

                 t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeSynchronic, nullptr);
                 t.setProperty(IDs::width, 260, nullptr);
                 t.setProperty(IDs::height, 132, nullptr);
                 t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
                 prep_list->appendChild(t,  &undo);
                 return true;
            }
            case blendronic:
            {
                 juce::ValueTree t(IDs::blendronic);
                 t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeBlendronic, nullptr);
                 t.setProperty(IDs::width, 245, nullptr);
                 t.setProperty(IDs::height, 125, nullptr);
                 t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
                 prep_list->appendChild(t,  &undo);
                 return true;
            }
            case tempo:
            {
                juce::ValueTree t(IDs::tempo);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTempo, nullptr);
                t.setProperty(IDs::width, 132, nullptr);
                t.setProperty(IDs::height, 260, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case tuning:
            {
                juce::ValueTree t(IDs::tuning);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTuning, nullptr);
                t.setProperty(IDs::width, 125, nullptr);
                t.setProperty(IDs::height, 245, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                // t.setProperty(IDs::x, lastX - 125 / 2, nullptr);
                // t.setProperty(IDs::y, lastY - 245 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case midifilter:
            {
                juce::ValueTree t(IDs::midiFilter);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeMidiFilter, nullptr);
                t.setProperty(IDs::width, 75, nullptr);
                t.setProperty(IDs::height, 75, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case miditarget:
            {
                juce::ValueTree t(IDs::midiTarget);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeMidiTarget, nullptr);
                t.setProperty(IDs::width, 75, nullptr);
                t.setProperty(IDs::height, 75, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case pianoswitch:
            {
                juce::ValueTree t(IDs::pianoMap);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypePianoMap, nullptr);
                t.setProperty(IDs::width, 150, nullptr);
                t.setProperty(IDs::height, 120, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case modulation:
            {
                juce::ValueTree t(IDs::modulation);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeModulation, nullptr);
                t.setProperty(IDs::width, 100, nullptr);
                t.setProperty(IDs::height, 100, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                // t.setProperty(IDs::x, lastX - 100 / 2, nullptr);
                // t.setProperty(IDs::y, lastY - 100 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case resetMod: {
                juce::ValueTree t(IDs::reset);

                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeReset, nullptr);
                t.setProperty(IDs::width, 100, nullptr);
                t.setProperty(IDs::height, 100, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                // t.setProperty(IDs::x, lastX - 100 / 2, nullptr);
                // t.setProperty(IDs::y, lastY - 100 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case deletion:
            {

                auto& lasso = preparationSelector.getLassoSelection();
                auto lassoCopy  = lasso;
                lasso.deselectAll();
                for (auto prep : lassoCopy)
                {
                    prep_list->removeChild(prep->state, &undo);
                }

                return true;
            }
            default:
                return false;
        }
}

void ConstructionSite::reset() {
    DBG("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
    if (_parent == nullptr)
        return;
        if (_parent->getSynth() != nullptr) {
            // _parent->getSynth()->getEngine()->resetEngine();
            // prep_list->setValueTree(_parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO).getChildWithName(
            // IDs::PREPARATIONS));
            // setActivePiano();
        }
//        if(prep_list)
//            prep_list->removeListener(this);
//        prep_list = nullptr;
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
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        addSubSection (s.get());
    }
    Skin default_skin;
    s->setSkinValues(default_skin, false);
    s->setDefaultColor();
    s->setSizeRatio(size_ratio_);
    s->setCentrePosition (s->curr_point);
    // s->setCentrePosition(s->x, s->y);
    s->setSize(s->width, s->height);

    /**
     * todo: this stuff should be moved outside of the GUI to the back end
     * should be getting added to the PreparationList, and a few other things that will have happen around it!
     */

    s->selectedSet = &(preparationSelector.getLassoSelection());
    preparationSelector.getLassoSelection().addChangeListener(s.get());
    s->addListener(&cableView);
    s->addListener(&modulationLineView);
    s->addListener(this);
    s->setNodeInfo();
    {
        juce::ScopedLock lock (open_gl_critical_section_);
        plugin_components.push_back (std::move (s));
    }
}

void ConstructionSite::renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate)
{
    juce::ScopedLock lock(open_gl_critical_section_);
    SynthSection::renderOpenGlComponents(open_gl, animate);

}

void ConstructionSite::removeModule(PluginInstanceWrapper* wrapper){
    // find preparation section with the same id as the one we're removing
    int index = -1;
    if (plugin_components.empty())
        return;
    for (int i=0; i<plugin_components.size(); i++){
        if (plugin_components[i]->pluginID == wrapper->node_id){
            index = i;
            break;
        }
    }
    if (index == -1) jassertfalse;
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    interface->getGui()->mod_popup->reset();
    interface->getGui()->prep_popup->reset();
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
DBG("moduleRemoved construction site");
}

ConstructionSite::~ConstructionSite(void) {
    removeMouseListener(&cableView);
    removeChildComponent(&selectorLasso);
    prep_list->removeListener(this);
}

void ConstructionSite::paintBackground(juce::Graphics &g) {
    paintBody(g);
    paintChildrenBackgrounds(g);
}

void ConstructionSite::resized() {
    // Get the current local bounds of this component
    auto bounds = getLocalBounds();
    int title_width = getTitleWidth();

    // the level meter and output gain slider (right side of preparation popup)
    // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly

    auto& gainParams = gainProcessor.getState().params;
    auto& listeners  = gainProcessor.getState().getParameterListeners();
    levelMeter = std::make_unique<PeakMeterSection>(
        "BusGain",
        gainParams.outputGain,
        listeners,
        &gainParams.outputLevels
    );
    levelMeter->setLabel("Main");
    levelMeter->setColor (juce::Colours::black);
    addSubSection(levelMeter.get());

    // Debug log the bounds
    DBG("Local Bounds: " + bounds.toString());

    cableView.setBounds(getLocalBounds());
    cableView.updateCablePositions();
    modulationLineView.setBounds(getLocalBounds());
    bounds.removeFromRight(title_width);
    bounds.removeFromTop(50);

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);
    // _line->setBounds(get);

    SynthSection::resized();
}

void ConstructionSite::redraw(void) {
    draw();
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
    //grabKeyboardFocus();

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
        // t.setProperty(IDs::x, lastX - 132 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 260 / 2, nullptr);
        // prep_list->appendChild(t,  interface->getUndoManager());
        prep_list->appendChild(t,  &undo);
    } else {
        _parent = findParentComponentOfClass<SynthGuiInterface>();
        juce::ValueTree t(IDs::PREPARATION);
        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeVST, nullptr);
        t.setProperty(IDs::width, 245, nullptr);
        t.setProperty(IDs::height, 125, nullptr);
        // t.setProperty(IDs::x, lastX - 245 / 2, nullptr);
        // t.setProperty(IDs::y, lastY - 125 / 2, nullptr);

        auto desc = _parent->getSynth()->user_prefs->userPreferences->pluginDescriptionsAndPreference[selection - static_cast<int>(bitklavier::BKPreparationType::PreparationTypeVST)];
        juce::ValueTree plugin = juce::ValueTree::fromXml(*desc.pluginDescription.createXml());
        t.addChild(plugin,-1, &undo);
        prep_list->addPlugin(desc.pluginDescription,t);

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
    for (int i = prep_list->size(); --i >= 0;) {
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

void ConstructionSite::setActivePiano() {
    if(prep_list != nullptr)
    {
        prep_list->deleteAllGui();
        prep_list->removeListener (this);
    }

    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    prep_list= interface->getSynth()->getActivePreparationList();
    parent = prep_list->getValueTree().getParent();
    prep_list->addListener(this);
    prep_list->rebuildAllGui();

    cableView.setActivePiano();
    modulationLineView.setActivePiano();
}

void ConstructionSite::removeAllGuiListeners()
{
    if(prep_list)
        prep_list->removeListener(this);
    prep_list = nullptr;
    cableView.removeAllGuiListeners();
    modulationLineView.removeAllGuiListeners();
}