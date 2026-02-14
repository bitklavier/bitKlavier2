//
// Created by Davis Polito on 1/17/24.
//


#include "ConstructionSite.h"
#include "Preparations.h"

#include "sound_engine.h"
#include "synth_gui_interface.h"
#include "open_gl_line.h"
#include "tracktion_ValueTreeUtilities.h"

static constexpr std::array<std::pair<float, float>,
    static_cast<size_t>(bitklavier::BKPreparationType::PreparationTypeVST) - 1> prepSizes =
{{
    /* 3 Keymap      */ { 185.0f, 105.0f },
    /* 1 Direct      */ { 245.0f, 125.0f },
    /* 5 Synchronic  */ { 260.0f, 132.0f },
    /* 2 Nostalgic   */ { 245.0f, 125.0f },
    /* 6 Blendronic  */ { 245.0f, 125.0f }, // default
    /* 4 Resonance   */ { 245.0f, 125.0f },
    /* 8 Tuning      */ { 125.0f, 245.0f },
    /* 7 Tempo       */ { 132.0f, 260.0f },
    /* 9 MidiFilter  */ { 75.0f,  75.0f  },
    /* 10 MidiTarget */ { 75.0f,  75.0f  },
    /* 12 Modulation */ { 100.0f, 100.0f },
    /* 13 Reset      */ { 100.0f, 100.0f },
    /* 11 PianoMap   */ { 150.0f, 120.0f },
}};

ConstructionSite::ConstructionSite(const juce::ValueTree &v, juce::UndoManager &um, OpenGlWrapper &open_gl,
                                   SynthGuiData *data, juce::ApplicationCommandManager &_manager)
                                                        : SynthSection("Construction Site"),
                                                        prep_list(data->synth->getActivePreparationList()),
                                                         undo(um),
                                                         open_gl(open_gl),
                                                         cableView(*this, um,data),
                                                         modulationLineView(*this, um,data),
                                                         preparationSelector(*this), parent(v),
                                                        commandManager (_manager)
//_line(std::make_shared<OpenGlLine>(nullptr,nullptr,nullptr))
{
    commandManager.registerAllCommandsForTarget (this);

    setWantsKeyboardFocus(true);
    //addKeyListener(this);
    setSkinOverride(Skin::kConstructionSite);
    setInterceptsMouseClicks(false, true);
    //data->synth->getEngine()->addChangeListener(this);
    // data->synth->setGainProcessor(&gainProcessor);

    cableView.setAlwaysOnTop(true);
    addSubSection(&cableView);
    cableView.setAlwaysOnTop(true);
    addSubSection(&modulationLineView);
    modulationLineView.setAlwaysOnTop(false);
    prep_list->addListener(this);

    // prep_list->addChangeListener(this);
    nodeFactory.Register(IDs::direct, DirectPreparation::create);
    nodeFactory.Register(IDs::blendronic, BlendronicPreparation::create);
    nodeFactory.Register(IDs::synchronic, SynchronicPreparation::create);
    nodeFactory.Register(IDs::keymap, KeymapPreparation::create);
    nodeFactory.Register(IDs::vst, PluginPreparation::create);
    nodeFactory.Register(IDs::modulation, ModulationPreparation::create);
    nodeFactory.Register(IDs::tuning,TuningPreparation::create);
    nodeFactory.Register(IDs::reset,ResetPreparation::create);
    nodeFactory.Register(IDs::midiFilter, MidiFilterPreparation::create);
    nodeFactory.Register(IDs::midiTarget, MidiTargetPreparation::create);
    nodeFactory.Register(IDs::pianoMap, PianoSwitchPreparation::create);
    nodeFactory.Register(IDs::tempo, TempoPreparation::create);
    nodeFactory.Register(IDs::nostalgic, NostalgicPreparation::create);
    nodeFactory.Register(IDs::resonance, ResonancePreparation::create);

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
    commands.addArray({
        direct,
        nostalgic,
        keymap,
        resonance,
        synchronic,
        tuning,
        blendronic,
        tempo,
        modulation,
        deletion,
        resetMod,
        midifilter,
        miditarget,
        pianoswitch});
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

    /*
     * todo: prepScale should be settable by the user, and saved
     *          - it should also scale distances between preps
     *          - not sure if this is the right place to set this overall, but it scales the prep sizes at least
     */
    float prepScale = 0.6;

    float prepWidth = 245.0f;
    float prepHeight = 125.0f;
    prepWidth *= prepScale;
    prepHeight *= prepScale;

        switch (info.commandID) {
            case direct:
            {
                juce::ValueTree t(IDs::direct);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeDirect, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - prepWidth / 2., lastY - prepHeight / 2.)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case nostalgic:
            {
                juce::ValueTree t(IDs::nostalgic);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeNostalgic, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - prepWidth / 2., lastY - prepHeight / 2.)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case keymap:
            {
                prepWidth = 185.0f;
                prepHeight = 105.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::keymap);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeKeymap, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2., lastY -  roundToInt(t.getProperty(IDs::height))/ 2.)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case resonance:
            {
                juce::ValueTree t(IDs::resonance);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeResonance, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - prepWidth / 2,lastY - prepHeight / 2)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case synchronic:
            {
                prepWidth = 260.0f;
                prepHeight = 132.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;

                 juce::ValueTree t(IDs::synchronic);
                 t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeSynchronic, nullptr);
                 t.setProperty(IDs::width, prepWidth, nullptr);
                 t.setProperty(IDs::height, prepHeight, nullptr);
                 t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - prepWidth / 2., lastY - prepHeight / 2.)), nullptr);
                 prep_list->appendChild(t,  &undo);
                 return true;
            }
            case blendronic:
            {
                prepWidth = 245.0f;
                prepHeight = 125.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                 juce::ValueTree t(IDs::blendronic);
                 t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeBlendronic, nullptr);
                 t.setProperty(IDs::width, prepWidth, nullptr);
                 t.setProperty(IDs::height, prepHeight, nullptr);
                 t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
                 prep_list->appendChild(t,  &undo);
                 return true;
            }
            case tempo:
            {
                prepWidth = 132.0f;
                prepHeight = 260.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::tempo);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTempo, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case tuning:
            {
                prepWidth = 125.0f;
                prepHeight = 245.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::tuning);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeTuning, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                // t.setProperty(IDs::x, lastX - 125 / 2, nullptr);
                // t.setProperty(IDs::y, lastY - 245 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case midifilter:
            {
                prepWidth = 120.0f;
                prepHeight = 120.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::midiFilter);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeMidiFilter, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case miditarget:
            {
                prepWidth = 120.0f;
                prepHeight = 120.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::midiTarget);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeMidiTarget, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case pianoswitch:
            {
                prepWidth = 150.0f;
                prepHeight = 120.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::pianoMap);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypePianoMap, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case modulation:
            {
                prepWidth = 100.0f;
                prepHeight = 100.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::modulation);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeModulation, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                    juce::Point<int>(lastX - roundToInt(t.getProperty(IDs::width)) / 2,lastY -  roundToInt(t.getProperty(IDs::height))/ 2)), nullptr);

                // t.setProperty(IDs::x, lastX - 100 / 2, nullptr);
                // t.setProperty(IDs::y, lastY - 100 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case resetMod: {
                prepWidth = 100.0f;
                prepHeight = 100.0f;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::reset);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeReset, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
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

void ConstructionSite::createWindow (juce::AudioProcessorGraph::Node* node, PluginWindow::Type type)
{
    jassert (node != nullptr);

    #if JUCE_IOS || JUCE_ANDROID
    closeAnyOpenPluginWindows();
    #else
    for (auto* w : activePluginWindows)
        if (w->node.get() == node && w->type == type)
        {
            w->toFront (true);
            return; // <-- prevent creating a second editor for the same processor/type
        }
    #endif

    if (auto* processor = node->getProcessor())
    {
        if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*> (processor))
        {
            auto description = plugin->getPluginDescription();
            auto window = activePluginWindows.add (new PluginWindow (node, type, activePluginWindows));
            window->toFront (true);
        }
    }
}

void ConstructionSite::moduleAdded(PluginInstanceWrapper* wrapper) {
    auto * interface = findParentComponentOfClass<SynthGuiInterface>();
    auto s = nodeFactory.CreateObject(wrapper->state.getType(), wrapper->state,interface );
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
void ConstructionSite::linkedPiano() {

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
    //paintBody(g);
    /*
     * todo: setup color setting better, or document how/where to set these colors
     */
    paintBody(g, getLocalBounds(), juce::Colours::burlywood.withMultipliedBrightness(0.4));
    paintChildrenBackgrounds(g);
}

void ConstructionSite::resized()
{
    cableView.setBounds(getLocalBounds());
    cableView.updateCablePositions();
    modulationLineView.setBounds(getLocalBounds());

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
        auto itemToSelect = dynamic_cast<PreparationSection *>(e.originalComponent->getParentComponent());
        if (itemToSelect == nullptr) {
            itemToSelect = dynamic_cast<PreparationSection *>(e.originalComponent);
        }

        if (itemToSelect != nullptr) {
            return;
        }

        _parent = findParentComponentOfClass<SynthGuiInterface>();
          auto callback = [=](int selection,int index) {handlePluginPopup(selection,index);};
    auto cancel = [=]() {

    };
        showPopupSelector(this, e.getPosition(), _parent->getPreparationPopupItems(),callback,cancel);
    }
    // Stop trying to make a connection on blank space click
    connect = false;
}

void ConstructionSite::addItem (int selection, bool center)
{
    float prepScale = 0.6;
    if (selection < bitklavier::BKPreparationType::PreparationTypeVST) {
        //const auto idx = static_cast<size_t>(selection - 1);
        const auto idx = static_cast<size_t>(selection);
        jassert (idx < prepSizes.size());

        const auto [baseW, baseH] = prepSizes[idx];

        const float prepWidth  = baseW * prepScale;
        const float prepHeight = baseH * prepScale;

        juce::ValueTree t(preparationIDs[idx]);
        t.setProperty(IDs::type,
                      static_cast<bitklavier::BKPreparationType>(selection),
                      nullptr);
        t.setProperty(IDs::width,  prepWidth,  nullptr);
        t.setProperty(IDs::height, prepHeight, nullptr);

        if (center)
        {
            t.setProperty(IDs::x_y,
            juce::VariantConverter<juce::Point<int>>::toVar(
                juce::Point<int>(getLocalBounds().getCentreX() * 0.25, getLocalBounds().getCentreY() * 0.25)),
            nullptr);
        }
        else
        {
            t.setProperty(IDs::x_y,
               juce::VariantConverter<juce::Point<int>>::toVar(
                   juce::Point<int>(lastX - prepWidth  / 2.0f, lastY - prepHeight / 2.0f)),
               nullptr);
        }

        prep_list->appendChild(t,  &undo);
    }
    else {
        DBG("adding VST? " + juce::String(selection) + "");

        const float prepWidth  = 245.f * prepScale;
        const float prepHeight = 125.f * prepScale;
        _parent = findParentComponentOfClass<SynthGuiInterface>();
        juce::ValueTree t(IDs::vst);
        t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeVST, nullptr);
        t.setProperty(IDs::width, prepWidth, nullptr);
        t.setProperty(IDs::height, prepHeight, nullptr);
        //t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - prepWidth / 2., lastY - prepHeight / 2.)), nullptr);
        //t.setProperty(IDs::x_y,juce::VariantConverter<juce::Point<int>>::toVar( juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
        // t.setProperty(IDs::y, lastY - 125 / 2, nullptr);

        if (center)
        {
            t.setProperty(IDs::x_y,
            juce::VariantConverter<juce::Point<int>>::toVar(
                juce::Point<int>(getLocalBounds().getCentreX() * 0.25, getLocalBounds().getCentreY() * 0.25)),
            nullptr);
        }
        else
        {
            t.setProperty(IDs::x_y,
               juce::VariantConverter<juce::Point<int>>::toVar(
                   juce::Point<int>(lastX - prepWidth  / 2.0f, lastY - prepHeight / 2.0f)),
               nullptr);
        }

        auto desc = _parent->getSynth()->user_prefs->userPreferences->pluginDescriptionsAndPreference[selection - static_cast<int>(bitklavier::BKPreparationType::PreparationTypeVST)];
        juce::ValueTree plugin = juce::ValueTree::fromXml(*desc.pluginDescription.createXml());
        t.addChild(plugin,-1, &undo);
        prep_list->addPlugin(desc.pluginDescription,t);
    }
}

void ConstructionSite::handlePluginPopup(int selection, int index) {

    addItem(selection, false);
    // float prepScale = 0.6;
    // if (selection < bitklavier::BKPreparationType::PreparationTypeVST) {
    //     //const auto idx = static_cast<size_t>(selection - 1);
    //     const auto idx = static_cast<size_t>(selection);
    //     jassert (idx < prepSizes.size());
    //
    //     const auto [baseW, baseH] = prepSizes[idx];
    //
    //     const float prepWidth  = baseW * prepScale;
    //     const float prepHeight = baseH * prepScale;
    //
    //     juce::ValueTree t(preparationIDs[idx]);
    //     t.setProperty(IDs::type,
    //                   static_cast<bitklavier::BKPreparationType>(selection),
    //                   nullptr);
    //     t.setProperty(IDs::width,  prepWidth,  nullptr);
    //     t.setProperty(IDs::height, prepHeight, nullptr);
    //
    //     t.setProperty(IDs::x_y,
    //         juce::VariantConverter<juce::Point<int>>::toVar(
    //             juce::Point<int>(lastX - prepWidth  / 2.0f,
    //                              lastY - prepHeight / 2.0f)),
    //         nullptr);
    //     prep_list->appendChild(t,  &undo);
    // }
    // else {
    //     const float prepWidth  = 245.f * prepScale;
    //     const float prepHeight = 125.f * prepScale;
    //     _parent = findParentComponentOfClass<SynthGuiInterface>();
    //     juce::ValueTree t(IDs::PREPARATION);
    //     t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeVST, nullptr);
    //     t.setProperty(IDs::width, prepWidth, nullptr);
    //     t.setProperty(IDs::height, prepHeight, nullptr);
    //     t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX - prepWidth / 2., lastY - prepHeight / 2.)), nullptr);
    //     //t.setProperty(IDs::x_y,juce::VariantConverter<juce::Point<int>>::toVar( juce::Point<int>(lastX - 245 / 2,lastY - 125 / 2)), nullptr);
    //     // t.setProperty(IDs::y, lastY - 125 / 2, nullptr);
    //
    //     auto desc = _parent->getSynth()->user_prefs->userPreferences->pluginDescriptionsAndPreference[selection - static_cast<int>(bitklavier::BKPreparationType::PreparationTypeVST)];
    //     juce::ValueTree plugin = juce::ValueTree::fromXml(*desc.pluginDescription.createXml());
    //     t.addChild(plugin,-1, &undo);
    //     prep_list->addPlugin(desc.pluginDescription,t);
    // }
}

void ConstructionSite::mouseUp(const juce::MouseEvent &eo) {
    //inLasso = false;
    //    DBG ("mouseupconst");
    selectorLasso.endLasso();
    removeChildComponent(&selectorLasso);
    if (edittingComment) return;

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
    if (prep_list)
    {
        // Ensure existing GUI elements are removed before detaching
        prep_list->deleteAllGui();
        prep_list->removeListener(this);
    }
    prep_list = nullptr;
    cableView.removeAllGuiListeners();
    modulationLineView.removeAllGuiListeners();
}
