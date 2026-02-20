//
// Created by Davis Polito on 1/17/24.
//


#include "ConstructionSite.h"
#include "Preparations.h"

#include "sound_engine.h"
#include "synth_gui_interface.h"
#include "open_gl_line.h"
#include "tracktion_ValueTreeUtilities.h"
#include "valuetree_utils/ValueTreeIdsRemapper.h"
#include <set>

juce::ValueTree ConstructionSite::clipboard;

static constexpr std::array<std::pair<float, float>,
    static_cast<size_t>(bitklavier::BKPreparationType::BKPreparationTypeNil)> prepSizes =
{{
    /* 0 Keymap      */ { 185.0f, 105.0f },
    /* 1 Direct      */ { 245.0f, 125.0f },
    /* 2 Synchronic  */ { 260.0f, 132.0f },
    /* 3 Nostalgic   */ { 245.0f, 125.0f },
    /* 4 Blendronic  */ { 245.0f, 125.0f },
    /* 5 Resonance   */ { 245.0f, 125.0f },
    /* 6 Tuning      */ { 125.0f, 245.0f },
    /* 7 Tempo       */ { 132.0f, 260.0f },
    /* 8 MidiFilter  */ { 75.0f,  75.0f  },
    /* 9 MidiTarget */ { 75.0f,  75.0f  },
    /* 10 Modulation */ { 100.0f, 100.0f },
    /* 11 Reset      */ { 100.0f, 100.0f },
    /* 12 PianoMap   */ { 150.0f, 120.0f },
    /* 13 Comment    */ { 100.0f, 100.0f },
    /* 14 Compressor */ { 245.0f, 125.0f },
    /* 15 EQ         */ { 245.0f, 125.0f },
    /* 16 VST        */ { 245.0f, 125.0f },
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
                                                        commandManager (_manager),
                                                        connection_list (data->synth->getActiveConnectionList())
//_line(std::make_shared<OpenGlLine>(nullptr,nullptr,nullptr))
{
    commandManager.registerAllCommandsForTarget (this);

    setWantsKeyboardFocus(true);
    //addKeyListener(this);
    setSkinOverride(Skin::kConstructionSite);
    setInterceptsMouseClicks(true, true);
    //data->synth->getEngine()->addChangeListener(this);
    // data->synth->setGainProcessor(&gainProcessor);

    cableView.setAlwaysOnTop(true);
    addSubSection(&cableView);
    cableView.setAlwaysOnTop(true);
    addSubSection(&modulationLineView);
    modulationLineView.setAlwaysOnTop(false);
    prep_list->addListener(this);

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

    lassoVisual = std::make_shared<OpenGlImageComponent>("lassoVisual");
    lassoVisual->setComponent(&selectorLasso);
    lassoVisual->setUseAlpha(true);
    lassoVisual->setVisible(false);
    lassoVisual->setAlwaysOnTop(true);
    lassoVisual->setStatic(false);
    addOpenGlComponent(lassoVisual, false, true);

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
    pianoswitch = 0x0626,
    horizontallyAlignSelected = 0x0627,
    verticallyAlignSelected = 0x0628,
    nudgeUp = 0x0629,
    nudgeDown = 0x062a,
    nudgeLeft = 0x062b,
    nudgeRight = 0x062c,
    selectAll = 0x062d,
    copy = 0x062e,
    paste = 0x062f
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
        pianoswitch,
        horizontallyAlignSelected,
        verticallyAlignSelected,
        nudgeUp,
        nudgeDown,
        nudgeLeft,
        nudgeRight,
        selectAll,
        copy,
        paste});
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
            info.addDefaultKeypress(juce::KeyPress::deleteKey, juce::ModifierKeys::noModifiers);
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
        case horizontallyAlignSelected:
            info.setInfo("Horizontally Align Selected", "Aligns Selected Preparations Horizontally", "Edit", 0);
            info.addDefaultKeypress('h', juce::ModifierKeys::shiftModifier);
            break;
        case verticallyAlignSelected:
            info.setInfo("Vertically Align Selected", "Aligns Selected Preparations Vertically", "Edit", 0);
            info.addDefaultKeypress('v', juce::ModifierKeys::shiftModifier);
            break;
        case nudgeUp:
            info.setInfo("Nudge Up", "Moves selected items up", "Edit", 0);
            info.addDefaultKeypress(juce::KeyPress::upKey, juce::ModifierKeys::noModifiers);
            break;
        case nudgeDown:
            info.setInfo("Nudge Down", "Moves selected items down", "Edit", 0);
            info.addDefaultKeypress(juce::KeyPress::downKey, juce::ModifierKeys::noModifiers);
            break;
        case nudgeLeft:
            info.setInfo("Nudge Left", "Moves selected items left", "Edit", 0);
            info.addDefaultKeypress(juce::KeyPress::leftKey, juce::ModifierKeys::noModifiers);
            break;
        case nudgeRight:
            info.setInfo("Nudge Right", "Moves selected items right", "Edit", 0);
            info.addDefaultKeypress(juce::KeyPress::rightKey, juce::ModifierKeys::noModifiers);
            break;
        case selectAll:
            info.setInfo("Select All", "Selects all preparations", "Edit", 0);
            info.addDefaultKeypress('a', juce::ModifierKeys::commandModifier);
            break;
        case copy:
            info.setInfo("Copy", "Copies selected preparations", "Edit", 0);
            info.addDefaultKeypress('c', juce::ModifierKeys::commandModifier);
            break;
        case paste:
            info.setInfo("Paste", "Pastes copied preparations", "Edit", 0);
            info.addDefaultKeypress('v', juce::ModifierKeys::commandModifier);
            break;
    }
}

bool ConstructionSite::perform(const InvocationInfo &info) {

    /*
     * todo: prepScale should be settable by the user, and saved
     *          - it should also scale distances between preps
     *          - not sure if this is the right place to set this overall, but it scales the prep sizes at least
     *      - yeah this is not the right way to do this, but will leave for now....
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
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX, lastY)), nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case nostalgic:
            {
                juce::ValueTree t(IDs::nostalgic);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeNostalgic, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX, lastY)), nullptr);
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
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX, lastY)), nullptr);

                juce::ValueTree midiInput(IDs::midiInput);
                midiInput.setProperty(IDs::midiDeviceId, IDs::defaultMidiInput.toString(), nullptr);
                t.appendChild(midiInput, nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case resonance:
            {
                juce::ValueTree t(IDs::resonance);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypeResonance, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX, lastY)), nullptr);
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
                 t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX, lastY)), nullptr);
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
                 t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(juce::Point<int>(lastX, lastY)), nullptr);
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
                    juce::Point<int>(lastX, lastY)), nullptr);
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
                    juce::Point<int>(lastX, lastY)), nullptr);

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
                                             juce::Point<int>(lastX, lastY)), nullptr);

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
                                             juce::Point<int>(lastX, lastY)), nullptr);

                prep_list->appendChild(t,  &undo);
                return true;
            }
            case pianoswitch:
            {
                prepWidth = 150.0f * 1.5;
                prepHeight = 120.0f * 1.25;
                prepWidth *= prepScale;
                prepHeight *= prepScale;
                juce::ValueTree t(IDs::pianoMap);
                t.setProperty(IDs::type, bitklavier::BKPreparationType::PreparationTypePianoMap, nullptr);
                t.setProperty(IDs::width, prepWidth, nullptr);
                t.setProperty(IDs::height, prepHeight, nullptr);
                t.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(
                                             juce::Point<int>(lastX, lastY)), nullptr);

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
                    juce::Point<int>(lastX, lastY)), nullptr);

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
                    juce::Point<int>(lastX, lastY)), nullptr);

                // t.setProperty(IDs::x, lastX - 100 / 2, nullptr);
                // t.setProperty(IDs::y, lastY - 100 / 2, nullptr);
                prep_list->appendChild(t,  &undo);
                return true;
            }
            case horizontallyAlignSelected:
            {
                auto& lasso = preparationSelector.getLassoSelection();
                if (lasso.getNumSelected() > 1)
                {
                    undo.beginNewTransaction();
                    float sumY = 0;
                    for (int i = 0; i < lasso.getNumSelected(); ++i)
                        sumY += lasso.getSelectedItem(i)->getBounds().getCentreY();

                    int avgY = std::round(sumY / lasso.getNumSelected());

                    for (int i = 0; i < lasso.getNumSelected(); ++i)
                    {
                        auto* fc = lasso.getSelectedItem(i);
                        auto center = fc->getBounds().getCentre();
                        fc->curr_point = juce::Point<int>(center.getX(), avgY);
                    }
                    cableView._update();
                    modulationLineView._update();
                }
                return true;
            }
            case verticallyAlignSelected:
            {
                auto& lasso = preparationSelector.getLassoSelection();
                if (lasso.getNumSelected() > 1)
                {
                    undo.beginNewTransaction();
                    float sumX = 0;
                    for (int i = 0; i < lasso.getNumSelected(); ++i)
                        sumX += lasso.getSelectedItem(i)->getBounds().getCentreX();

                    int avgX = std::round(sumX / lasso.getNumSelected());

                    for (int i = 0; i < lasso.getNumSelected(); ++i)
                    {
                        auto* fc = lasso.getSelectedItem(i);
                        auto center = fc->getBounds().getCentre();
                        fc->curr_point = juce::Point<int>(avgX, center.getY());
                    }
                    cableView._update();
                    modulationLineView._update();
                }
                return true;
            }
            case nudgeUp:
            case nudgeDown:
            case nudgeLeft:
            case nudgeRight:
            {
                auto& lasso = preparationSelector.getLassoSelection();
                if (lasso.getNumSelected() > 0)
                {
                    undo.beginNewTransaction();
                    juce::Point<int> delta(0, 0);
                    const int amount = 5; // small amount as requested

                    if (info.commandID == nudgeUp) delta.setY(-amount);
                    else if (info.commandID == nudgeDown) delta.setY(amount);
                    else if (info.commandID == nudgeLeft) delta.setX(-amount);
                    else if (info.commandID == nudgeRight) delta.setX(amount);

                    for (int i = 0; i < lasso.getNumSelected(); ++i)
                    {
                        auto* fc = lasso.getSelectedItem(i);
                        auto center = fc->getBounds().getCentre();
                        fc->curr_point = center + delta;
                    }
                    cableView._update();
                    modulationLineView._update();
                }
                return true;
            }
            case selectAll:
            {
                auto& lasso = preparationSelector.getLassoSelection();
                lasso.deselectAll();
                for (auto& fc : plugin_components)
                {
                    lasso.addToSelection(fc.get());
                }
                return true;
            }
            case deletion:
            {
                if (prep_list == nullptr)
                    return false;

                auto& lasso = preparationSelector.getLassoSelection();
                auto lassoCopy  = lasso;
                lasso.deselectAll();

                juce::Array<juce::ValueTree> prepsToRemove;
                for (auto prep : lassoCopy)
                {
                    prepsToRemove.add (prep->state);
                }

                juce::Array<juce::ValueTree> connectionsToRemove;
                if (connection_list != nullptr)
                {
                    const bitklavier::ConnectionList::ScopedLockType sl (connection_list->arrayLock);
                    // Copy the current objects to avoid iterator invalidation if the list is modified during removal.
                    // bitklavier::ConnectionList inherits from ValueTreeObjectList which has an 'objects' member.
                    for (int i = 0; i < connection_list->size(); ++i)
                    {
                        if (auto* connection = connection_list->at(i))
                        {
                            if (connection->state.isValid() && (bool) connection->state.getProperty (IDs::isSelected, false))
                            {
                                connectionsToRemove.add (connection->state);
                            }
                        }
                    }
                }

                juce::Array<juce::ValueTree> modConnectionsToRemove;
                if (modulationLineView.connection_list != nullptr)
                {
                    const bitklavier::ModConnectionList::ScopedLockType sl (modulationLineView.connection_list->arrayLock);
                    for (int i = 0; i < modulationLineView.connection_list->size(); ++i)
                    {
                        if (auto* modConnection = modulationLineView.connection_list->at(i))
                        {
                            if (modConnection->state.isValid() && (bool) modConnection->state.getProperty (IDs::isSelected, false))
                            {
                                modConnectionsToRemove.add (modConnection->state);
                            }
                        }
                    }
                }

                for (auto& vt : prepsToRemove)
                {
                    prep_list->removeChild (vt, &undo);
                }

                if (connection_list != nullptr)
                {
                    for (auto& vt : connectionsToRemove)
                    {
                        if (vt.isValid() && vt.getParent().isValid())
                            connection_list->removeChild (vt, &undo);
                    }
                }

                if (modulationLineView.connection_list != nullptr)
                {
                    for (auto& vt : modConnectionsToRemove)
                    {
                        if (vt.isValid() && vt.getParent().isValid())
                            modulationLineView.connection_list->removeChild (vt, &undo);
                    }
                }

                return true;
            }
            case copy:
                copySelectedItems();
                return true;
            case paste:
                pasteItems();
                return true;
            default:
                return false;
        }
    }

    void ConstructionSite::copySelectedItems()
    {
        auto selected = preparationSelector.getLassoSelection();
        if (selected.getNumSelected() == 0)
            return;

        clipboard = juce::ValueTree ("CLIPBOARD");
        auto preps = juce::ValueTree (IDs::PREPARATIONS);
        clipboard.addChild (preps, -1, nullptr);

        std::set<juce::String> selectedUuids;
        std::set<juce::AudioProcessorGraph::NodeID> selectedNodeIds;

        juce::Rectangle<int> bounds;

        for (int i = 0; i < selected.getNumSelected(); ++i)
        {
            auto* item = selected.getSelectedItem(i);
            if (item != nullptr && item->state.isValid())
            {
                preps.addChild (item->state.createCopy(), -1, nullptr);
                selectedUuids.insert (item->state.getProperty(IDs::uuid).toString());
                selectedNodeIds.insert (item->pluginID);

                if (item->state.hasProperty(IDs::x_y))
                {
                    auto p = juce::VariantConverter<juce::Point<int>>::fromVar (item->state.getProperty (IDs::x_y));
                    if (bounds.isEmpty())
                        bounds = juce::Rectangle<int>(p.x, p.y, 1, 1);
                    else
                        bounds = bounds.getUnion(juce::Rectangle<int>(p.x, p.y, 1, 1));
                }
            }
        }

        if (!bounds.isEmpty())
        {
            clipboard.setProperty(IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar(bounds.getCentre()), nullptr);
        }

        // Copy Connections
        auto connections = juce::ValueTree (IDs::CONNECTIONS);
        clipboard.addChild (connections, -1, nullptr);
        auto currentConnections = parent.getChildWithName (IDs::CONNECTIONS);
        for (int i = 0; i < currentConnections.getNumChildren(); ++i)
        {
            auto c = currentConnections.getChild(i);
            auto srcId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (c.getProperty(IDs::src));
            auto dstId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (c.getProperty(IDs::dest));

            if (selectedNodeIds.count(srcId) && selectedNodeIds.count(dstId))
                connections.addChild (c.createCopy(), -1, nullptr);
        }

        // Copy ModConnections
        auto modConnections = juce::ValueTree (IDs::MODCONNECTIONS);
        clipboard.addChild (modConnections, -1, nullptr);
        auto currentModConnections = parent.getChildWithName (IDs::MODCONNECTIONS);
        for (int i = 0; i < currentModConnections.getNumChildren(); ++i)
        {
            auto c = currentModConnections.getChild(i);
        
            if (c.hasType(IDs::ModulationConnection))
            {
                auto srcUuid = c.getProperty(IDs::src).toString().upToFirstOccurrenceOf ("_", false, false);
                auto dstUuid = c.getProperty(IDs::dest).toString().upToFirstOccurrenceOf ("_", false, false);
                if (selectedUuids.count(srcUuid) && selectedUuids.count(dstUuid))
                    modConnections.addChild (c.createCopy(), -1, nullptr);
            }
            else if (c.hasProperty(IDs::src) && c.hasProperty(IDs::dest))
            {
                auto srcId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (c.getProperty(IDs::src));
                auto dstId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (c.getProperty(IDs::dest));
                if (selectedNodeIds.count(srcId) && selectedNodeIds.count(dstId))
                {
                    auto copy = c.createCopy();
                    // Also check children of the wrapper
                    for (int j = copy.getNumChildren() - 1; j >= 0; --j)
                    {
                        auto mc = copy.getChild(j);
                        auto srcUuid = mc.getProperty(IDs::src).toString().upToFirstOccurrenceOf ("_", false, false);
                        auto dstUuid = mc.getProperty(IDs::dest).toString().upToFirstOccurrenceOf ("_", false, false);
                        if (!selectedUuids.count(srcUuid) || !selectedUuids.count(dstUuid))
                            copy.removeChild(j, nullptr);
                    }
                    modConnections.addChild (copy, -1, nullptr);
                }
            }
        }
    }

    void ConstructionSite::pasteItems()
    {
        if (!clipboard.isValid())
            return;

        auto workVt = clipboard.createCopy();
        auto preps = workVt.getChildWithName (IDs::PREPARATIONS);
        if (!preps.isValid())
            return;

        std::map<juce::String, juce::String> uuidMap;
        std::map<juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID> nodeIdMap;

        juce::Point<int> offset(20, 20);
        if (clipboard.hasProperty(IDs::x_y))
        {
            auto originalCenter = juce::VariantConverter<juce::Point<int>>::fromVar(clipboard.getProperty(IDs::x_y));
            offset = mouse.toInt() - originalCenter;
        }

        // Generate new IDs
        for (int i = 0; i < preps.getNumChildren(); ++i)
        {
            auto prep = preps.getChild(i);
            auto oldUuid = prep.getProperty(IDs::uuid).toString();
            auto newUuid = juce::Uuid().toString();
            uuidMap[oldUuid] = newUuid;
            prep.setProperty (IDs::uuid, newUuid, nullptr);

            auto oldNodeId = juce::AudioProcessorGraph::NodeID (juce::Uuid(oldUuid).getTimeLow());
            auto newNodeId = juce::AudioProcessorGraph::NodeID (juce::Uuid(newUuid).getTimeLow());
            nodeIdMap[oldNodeId] = newNodeId;
            prep.setProperty (IDs::nodeID, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(newNodeId), nullptr);
        
            // Deeply remap children of each preparation (like PORT, modulationproc, etc.)
            for (int j = 0; j < prep.getNumChildren(); ++j)
            {
                auto child = prep.getChild(j);
                bitklavier::deepRemapIDs (child, uuidMap, nodeIdMap);
            }

            // Offset position
            if (prep.hasProperty (IDs::x_y))
            {
                auto p = juce::VariantConverter<juce::Point<int>>::fromVar (prep.getProperty (IDs::x_y));
                p += offset;
                prep.setProperty (IDs::x_y, juce::VariantConverter<juce::Point<int>>::toVar (p), nullptr);
            }
        }

        // Remap Connections
        auto conns = workVt.getChildWithName (IDs::CONNECTIONS);
        if (conns.isValid())
        {
            for (int i = 0; i < conns.getNumChildren(); ++i)
            {
                auto c = conns.getChild(i);
                bitklavier::deepRemapIDs (c, uuidMap, nodeIdMap);
            }
        }

        // Remap ModConnections
        auto mconns = workVt.getChildWithName (IDs::MODCONNECTIONS);
        if (mconns.isValid())
        {
            for (int i = 0; i < mconns.getNumChildren(); ++i)
            {
                auto c = mconns.getChild(i);
                bitklavier::deepRemapIDs (c, uuidMap, nodeIdMap);
            }
        }

        // Apply to current piano
        undo.beginNewTransaction ("Paste items");
        
        auto& lasso = preparationSelector.getLassoSelection();
        lasso.deselectAll();
    
        // items
        for (int i = 0; i < preps.getNumChildren(); ++i)
            prep_list->appendChild (preps.getChild(i).createCopy(), &undo);
    
        // Use callAsync to wait for processors to be created before adding connections
        juce::MessageManager::callAsync([this, conns = conns.createCopy(), mconns = mconns.createCopy(), nodeIdMap] {
            // connections
            if (connection_list != nullptr)
            {
                for (int i = 0; i < conns.getNumChildren(); ++i)
                {
                    auto c = conns.getChild(i).createCopy();
                    connection_list->appendChild (c, &undo);
                }
            }
        
            // mod connections
            if (modulationLineView.connection_list != nullptr)
            {
                for (int i = 0; i < mconns.getNumChildren(); ++i)
                {
                    auto c = mconns.getChild(i).createCopy();
                    modulationLineView.connection_list->appendChild (c, &undo);
                }
            }

            // Select the newly pasted items
            auto& lasso = preparationSelector.getLassoSelection();
            for (auto const& [oldId, newId] : nodeIdMap)
            {
                if (auto* comp = getComponentForPlugin (newId))
                    lasso.addToSelection (comp);
            }
        });
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
    auto s = nodeFactory.CreateObject(wrapper->state.getType(), wrapper->state, interface );
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
    s->resized();

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
    DBG("ConstructionSite::linkedPiano()");
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

    // DBG("moduleRemoved construction site");
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

    // if you don't hit a cable, set all cables to be unselected
    cableView.hitTestCables (e.position);

    auto itemToSelect = e.originalComponent->findParentComponentOfClass<PreparationSection>();
    if (itemToSelect == nullptr)
        itemToSelect = dynamic_cast<PreparationSection*>(e.originalComponent);

    if (e.mods.isLeftButtonDown() && !e.mods.isPopupMenu() && itemToSelect == nullptr) {
        if (!e.mods.isShiftDown())
            preparationSelector.getLassoSelection().deselectAll();

        if (e.originalComponent == this || e.originalComponent->getName() == "cableView")
        {
            addChildComponent(selectorLasso);
            selectorLasso.setVisible(true);
            selectorLasso.toFront(false);

            selectorLasso.endLasso();
            selectorLasso.beginLasso(e, &preparationSelector);

            lassoVisual->setVisible(true);
            lassoVisual->setBounds(selectorLasso.getBounds());
            lassoVisual->redrawImage(true);

            //////Fake drag so the lasso will select anything we click and drag////////
            auto thisPoint = e.getPosition();
            thisPoint.addXY(1, 1);
            selectorLasso.dragLasso(e.withNewPosition(thisPoint));

            lassoVisual->setBounds(selectorLasso.getBounds());
            lassoVisual->redrawImage(true);
        }
    } else if (itemToSelect != nullptr && !e.mods.isPopupMenu()) {
        if (e.mods.isShiftDown())
        {
            if (preparationSelector.getLassoSelection().isSelected(itemToSelect))
                preparationSelector.getLassoSelection().deselect(itemToSelect);
            else
                preparationSelector.getLassoSelection().addToSelection(itemToSelect);
        }
        else
        {
            // If we click on an item that is NOT selected, select ONLY it.
            // If we click on an item that IS already selected, keep the current selection
            // so we can drag the whole group.
            if (!preparationSelector.getLassoSelection().isSelected(itemToSelect))
                preparationSelector.getLassoSelection().selectOnly(itemToSelect);
        }
    }

    held = false;

    lastX = eo.x;
    lastY = eo.y;

    mouse = e.position;

    // // This must happen before the right-click menu or the menu will close
    //grabKeyboardFocus();

    if (e.mods.isPopupMenu()) {
        auto itemToSelectPop = e.originalComponent->findParentComponentOfClass<PreparationSection>();
        if (itemToSelectPop == nullptr) {
            itemToSelectPop = dynamic_cast<PreparationSection *>(e.originalComponent);
        }

        if (itemToSelectPop != nullptr) {
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
                   juce::Point<int>(lastX, lastY)),
               nullptr);
        }

        prep_list->appendChild(t,  &undo);

        if (selection == bitklavier::BKPreparationType::PreparationTypeKeymap)
        {
            juce::ValueTree midiInput(IDs::midiInput);
            midiInput.setProperty(IDs::midiDeviceId, IDs::defaultMidiInput.toString(), nullptr);
            t.appendChild(midiInput, nullptr);
        }
    }
    else {
        DBG("adding VST? " + juce::String(selection) + "");

        const auto idx = static_cast<size_t>(bitklavier::BKPreparationType::PreparationTypeVST);
        jassert (idx < prepSizes.size());
        const auto [baseW, baseH] = prepSizes[idx];

        const float prepWidth  = baseW * prepScale;
        const float prepHeight = baseH * prepScale;
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
                   juce::Point<int>(lastX, lastY)),
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
    selectorLasso.setVisible(false);
    lassoVisual->setVisible(false);
    removeChildComponent(&selectorLasso);
    if (editing_comment_) return;

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
    if (editing_comment_)
        return;

    cableView.mouseDrag(e);

    // Do nothing on right click drag
    if (e.mods.isRightButtonDown())
        return;

    if (selectorLasso.isVisible())
    {
        // DBG("ConstructionSite::mouseDrag - dragging lasso");
        selectorLasso.toFront(false);
        selectorLasso.dragLasso(e);
        selectorLasso.repaint();

        lassoVisual->setBounds(selectorLasso.getBounds());
        lassoVisual->redrawImage(true);
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
    connection_list = interface->getSynth()->getActiveConnectionList();
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
