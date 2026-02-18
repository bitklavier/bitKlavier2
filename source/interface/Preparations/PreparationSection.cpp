//
// Created by Davis Polito on 2/16/24.
//

#include "PreparationSection.h"
#include "synth_gui_interface.h"
#include "ConstructionSite.h"
#include "FullInterface.h"
#include "ModulationPreparation.h"
#include "synth_base.h"
#include "KeymapProcessor.h"

namespace {
// Helper to apply default per-port MIDI Y offsets for a specific preparation type.
// - prepState: ValueTree of the preparation
// - prepType: BKPreparationType this default should apply to
// - inputYOffset: offset (px) applied to MIDI input ports (isIn = true) if not already set on the port
// - outputYOffset: offset (px) applied to MIDI output ports (isIn = false) if not already set on the port
static void applyDefaultMidiPortOffsetsForType(juce::ValueTree& prepState,
                                              bitklavier::BKPreparationType prepType,
                                              float inputYOffset,
                                              float outputYOffset)
{
    const auto currentType = static_cast<int>(prepState.getProperty(IDs::type));
    if (currentType != static_cast<int>(prepType))
        return;

    for (auto child : prepState)
    {
        if (!child.hasType(IDs::PORT))
            continue;

        const int ch = (int) child.getProperty(IDs::chIdx, 0);
        const bool isIn = (bool) child.getProperty(IDs::isIn, false);
        const bool isMidi = (ch == juce::AudioProcessorGraph::midiChannelIndex);

        if (isMidi)
        {
            // Only set if not already set, so manual tweaks can override
            if (!child.hasProperty(IDs::yOffset))
                child.setProperty(IDs::yOffset, isIn ? inputYOffset : outputYOffset, nullptr);
        }
    }
}
}

PreparationSection::PreparationSection(juce::String name, const juce::ValueTree& v, OpenGlWrapper &open_gl,
                                       juce::AudioProcessorGraph::NodeID node, juce::UndoManager &um) :
tracktion::engine::ValueTreeObjectList<BKPort>(v),
    SynthSection(name), state(v), _open_gl(open_gl),
    pluginID(node), undo(um)
{
    //_parent = findParentComponentOfClass<SynthGuiInterface>();
    // setInterceptsMouseClicks(true, false);
    //make this undoable
    curr_point.referTo(state,IDs::x_y,&undo);
    // x.referTo(state, IDs::x, &undo);
    // y.referTo(state, IDs::y, &undo);

    //dont make this undoable
    width.referTo(state, IDs::width, nullptr);
    height.referTo(state, IDs::height, nullptr);
    numIns.referTo(state, IDs::numIns, nullptr);
    numOuts.referTo(state, IDs::numOuts, nullptr);
    uuid.referTo(state, IDs::uuid, nullptr);

    if (state.hasProperty(IDs::width) && state.hasProperty(IDs::height))
        setSize(state.getProperty(IDs::width), state.getProperty(IDs::height));

    if (state.hasProperty (IDs::x_y))
        setCentrePosition (juce::VariantConverter<juce::Point<int>>::fromVar (state.getProperty (IDs::x_y)));

    constrainer.setMinimumOnscreenAmounts(0xffffff, 0xffffff, 0xffffff, 0xffffff);
    rebuildObjects();

    for (auto object: objects) {
        open_gl.context.executeOnGLThread([this,object,&open_gl](juce::OpenGLContext &context) {
            object->getImageComponent()->init(open_gl);
        }, true);
        this->addOpenGlComponent(object->getImageComponent(), false, true);
        object->addListener(this);
        addAndMakeVisible(object);
        object->redoImage();
    }

    // give default name to this prep, numbered by how many of this prep exist in the full gallery
    // - will need to check to see if the name exists already, so we don't overwrite
    if (!state.hasProperty (IDs::name))
    {
        int numThisPrep = howManyOfThisPrepTypeInVT(state.getRoot(), state.getType());
        state.setProperty(IDs::name, state.getType().toString() + " " + juce::String(numThisPrep), nullptr);
    }
}

juce::AudioProcessor *PreparationSection::getProcessor() const {
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>()) {
        if (auto node = parent->getSynth()->getNodeForId(pluginID))
            return node->getProcessor();

        return {};
    }
}

void PreparationSection::changeListenerCallback(juce::ChangeBroadcaster *source) {
    if (selectedSet->isSelected(this)) {
        item->setColor(juce::Colours::white);
        isSelected = true;

        // If this is a Keymap preparation, display its keyStates on the footer keyboard
        if (auto* keymapProc = dynamic_cast<KeymapProcessor*>(getProcessor()))
        {
            if (auto* iface = findParentComponentOfClass<SynthGuiInterface>())
            {
                if (auto* gui = iface->getGui())
                {
                    if (gui->footer_)
                    {
                        auto keys = keymapProc->getState().params.keyboard_state.keyStates.load();
                        gui->footer_->displayKeymapState(keys);
                    }
                }
            }
        }
    } else {
        item->setColor(findColour(Skin::kWidgetPrimary1, true));
        isSelected = false;

        // If this is a Keymap preparation being deselected, clear the footer keyboard display
        // only if no other Keymap is currently selected
        if (dynamic_cast<KeymapProcessor*>(getProcessor()) != nullptr)
        {
            bool anotherKeymapSelected = false;
            if (selectedSet != nullptr)
            {
                for (int i = 0; i < selectedSet->getNumSelected(); ++i)
                {
                    auto* other = selectedSet->getSelectedItem(i);
                    if (other != this && dynamic_cast<KeymapProcessor*>(other->getProcessor()) != nullptr)
                    {
                        anotherKeymapSelected = true;
                        break;
                    }
                }
            }
            if (!anotherKeymapSelected)
            {
                if (auto* iface = findParentComponentOfClass<SynthGuiInterface>())
                {
                    if (auto* gui = iface->getGui())
                    {
                        if (gui->footer_)
                            gui->footer_->clearKeymapDisplay();
                    }
                }
            }
        }
    }
    item->redoImage();
}

void PreparationSection::paintBackground(juce::Graphics &g) {
    if (item) {
        //item->setColor(findColour (Skin::kWidgetPrimary1, true));
        item->redoImage();
    }
    for (auto object: objects) {
        object->redoImage();
    }
    //    g.restoreState();
}

void PreparationSection::setNodeInfo() {
    if (auto interface = findParentComponentOfClass<SynthGuiInterface>()) {
        if (auto *node = interface->getSynth()->getNodeForId(pluginID)) {
            this->state.setProperty(IDs::nodeID,
                                    juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                                    nullptr);
            int i = 0;
            auto &processor = *node->getProcessor();
            if (objects.isEmpty()) {
                juce::MessageManager::callAsync([safeComp = juce::Component::SafePointer<PreparationSection>(this)] {
                    safeComp->setPortInfo();
                });
            }
        }
    }
}

void PreparationSection::setPortInfo() {
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
    {
        if (auto *node = parent->getSynth()->getNodeForId(pluginID))
        {
            auto processor = node->getProcessor();

            //check if main audio input bus/ is enabled
            if (processor->getBus(true, 0) != nullptr && processor->getBus(true, 0)->isEnabled()) {
                for (int i = 0; i < processor->getMainBusNumInputChannels(); i+=2) {
                    juce::ValueTree v{IDs::PORT};
                    v.setProperty(IDs::nodeID,
                                  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                                  nullptr);
                    v.setProperty(IDs::chIdx, i, nullptr);
                    v.setProperty(IDs::isIn, true, nullptr);
                    bool add = true;
                    for (auto vt: state) {
                        if (vt.isEquivalentTo(v)) {
                            add = false;
                        }
                    }
                    if (add) {
                        state.addChild(v, -1, nullptr);
                    }
                    numIns = numIns + 1;
                }
            }

            if (processor->acceptsMidi()) {
                juce::ValueTree v{IDs::PORT};
                v.setProperty(IDs::nodeID,
                              juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                              nullptr);
                v.setProperty(IDs::chIdx, juce::AudioProcessorGraph::midiChannelIndex, nullptr);
                v.setProperty(IDs::isIn, true, nullptr);
                bool add = true;
                for (auto vt: state) {
                    if (vt.isEquivalentTo(v)) {
                        add = false;
                    }
                }
                if (add) {
                    state.addChild(v, -1, nullptr);
                }
                numIns = numIns + 1;
            }

            //check if main audio output bus is enabled
            if (processor->getBus(false, 0) != nullptr && getProcessor()->getBus(false, 0)->isEnabled()) {
                for (int i = 0; i < processor->getMainBusNumOutputChannels(); i+=2) {
                    auto chIdx = processor->getBus(false,0)->getChannelIndexInProcessBlockBuffer(i);
                    juce::ValueTree v{IDs::PORT};
                    v.setProperty(IDs::nodeID,
                                  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                                  nullptr);
                    v.setProperty(IDs::chIdx, chIdx, nullptr);
                    v.setProperty(IDs::isIn, false, nullptr);
                    bool add = true;
                    for (auto vt: state) {
                        if (vt.isEquivalentTo(v)) {
                            add = false;
                        }
                    }
                    if (add) {
                        state.addChild(v, -1, nullptr);
                    }
                    numOuts = numOuts + 1;
                }
            }

            //check if send audio output bus is enabled
            if (processor->getBus(false, 2) != nullptr && getProcessor()->getBus(false, 2)->isEnabled()) {
                for (int i = 0; i < processor->getBus(false,2)->getNumberOfChannels(); i+=2) {
                    auto chIdx = processor->getBus(false,2)->getChannelIndexInProcessBlockBuffer(i);
                    juce::ValueTree v{IDs::PORT};
                    v.setProperty(IDs::nodeID,
                                  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                                  nullptr);
                    v.setProperty(IDs::chIdx, chIdx, nullptr);
                    v.setProperty(IDs::isIn, false, nullptr);
                    bool add = true;
                    for (auto vt: state) {
                        if (vt.isEquivalentTo(v)) {
                            add = false;
                        }
                    }
                    if (add) {
                        state.addChild(v, -1, nullptr);
                    }
                    numOuts = numOuts + 1;
                }
            }

            if (processor->producesMidi()) {
                juce::ValueTree v{IDs::PORT};
                v.setProperty(IDs::nodeID,
                              juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                              nullptr);
                v.setProperty(IDs::chIdx, juce::AudioProcessorGraph::midiChannelIndex, nullptr);
                v.setProperty(IDs::isIn, false, nullptr);
                bool add = true;
                for (auto vt: state) {
                    if (vt.isEquivalentTo(v)) {
                        add = false;
                    }
                }
                if (add) {
                    state.addChild(v, -1, nullptr);
                }
                numOuts = numOuts + 1;
            }

            // Apply default per-port MIDI yOffsets for specific preparation types via helper
            // MidiTarget: Top (output) +6 px, Bottom (input) -4 px (only if not already set on the port)
            applyDefaultMidiPortOffsetsForType(state,
                                              bitklavier::BKPreparationType::PreparationTypeMidiTarget,
                                              -10.0f,  // inputYOffset (bottom)
                                              +7.0f); // outputYOffset (top)

            applyDefaultMidiPortOffsetsForType(state,
                                              bitklavier::BKPreparationType::PreparationTypeMidiFilter,
                                              -7.0f,  // inputYOffset (bottom)
                                              +3.0f); // outputYOffset (top)

            applyDefaultMidiPortOffsetsForType(state,
                                              bitklavier::BKPreparationType::PreparationTypeModulation,
                                              +1.0f,  // inputYOffset (bottom)
                                              +0.0f); // outputYOffset (top), NA for Mod
        }
    }
}

void PreparationSection::mouseDown(const juce::MouseEvent &e) {
    pointBeforDrag = this->getPosition();
    juce::Logger::writeToLog ("prep mousedown");

    if (auto* site = dynamic_cast<ConstructionSite *>(getParentComponent()))
    {
        site->drag_offset_ = (e.getEventRelativeTo(this).getPosition() - getLocalBounds().getCentre()).toInt();
    }

    if (e.mods.isCtrlDown() || e.mods.isRightButtonDown())
    {
        FullInterface* fullInterface = findParentComponentOfClass<FullInterface>();
        if (fullInterface && fullInterface->header_)
        {
            std::vector<std::string> pianoNames = fullInterface->header_->getAllPianoNames();
            PopupItems menu("add linked prep to...");
            int id = 1;
            for (const auto& name : pianoNames)
            {
                menu.addItem(id++, name);
            }

            showPopupSelector(this, e.getPosition(), menu, [this, pianoNames](int id, int selectedIndex) {
                if (selectedIndex >= 0 && selectedIndex < pianoNames.size())
                {
                    DBG("Selected piano to add linked prep to: " << pianoNames[selectedIndex]);
                    juce::ValueTree linked_piano{IDs::linkedPrep};
                    auto gallery = state.getParent().getParent().getParent();
                    auto new_piano = gallery.getChildWithProperty(IDs::name, juce::String(pianoNames[selectedIndex]));
                    auto p  = state.getProperty(IDs::nodeID);
                    auto type = state.getProperty(IDs::type);
                    auto currPianoName = state.getParent().getParent().getProperty(IDs::name);

                    linked_piano.setProperty(IDs::nodeID, p,nullptr);
                    linked_piano.setProperty(IDs::linkedType,type,nullptr);
                    linked_piano.setProperty(IDs::linkedPianoName, currPianoName, nullptr);

                    new_piano.getChildWithName(IDs::PREPARATIONS).appendChild(linked_piano,nullptr);


                }
            });
        }
    }
}

void PreparationSection::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {

    if (auto* site = dynamic_cast<ConstructionSite*>(getParentComponent()))
    {
        site->mouse_drag_position_ = site->getLocalPoint(this, dragSourceDetails.localPosition);
        site->item_dropped_on_prep_ = true;
    }

    auto dropped_tree = juce::ValueTree::fromXml(dragSourceDetails.description);

    //should switch to strings for type names
    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeModulation) {
        for (auto listener: listeners_)
            listener->modulationDropped(dropped_tree, state);
    }

    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeTuning)
        for (auto listener: listeners_)
            listener->tuningDropped(dropped_tree, state);

    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeReset) {
        for (auto listener: listeners_)
            listener->resetDropped(dropped_tree, state);
    }
    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeTempo) {
        for (auto listener: listeners_)
            listener->tempoDropped(dropped_tree, state);
    }
    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeSynchronic) {
        for (auto listener: listeners_)
            listener->synchronicDropped(dropped_tree, state);
    }
}

void PreparationSection::itemDragMove(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (auto* site = dynamic_cast<ConstructionSite*>(getParentComponent()))
    {
        site->mouse_drag_position_ = site->getLocalPoint(this, dragSourceDetails.localPosition);
        site->item_dropped_on_prep_ = true;
    }
}

void PreparationSection::itemDragEnter(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (auto* site = dynamic_cast<ConstructionSite*>(getParentComponent()))
        site->item_dropped_on_prep_ = true;
}

void PreparationSection::itemDragExit(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    if (auto* site = dynamic_cast<ConstructionSite*>(getParentComponent()))
        site->item_dropped_on_prep_ = false;
}

void PreparationSection::resized() {
    juce::Rectangle<float> bounds = getLocalBounds().toFloat();
    int item_padding_y = kItemPaddingY * size_ratio_;
    int item_height = getHeight() - 2 * item_padding_y;
    int item_padding_x = kItemPaddingX * size_ratio_;
    int item_width = getWidth() - 2 * item_padding_x;
    auto newBounds = getBoundsInParent();

    if (item)
        item->setBounds(item_padding_x, item_padding_y, item_width, item_height);

    if (auto *processor = getProcessor()) {
        for (auto *port: objects) {
            const bool isInput = port->isInput;

            auto channelIndex = port->pin.channelIndex;

            int busIdx = 0;
            processor->getOffsetInBusBufferForAbsoluteChannelIndex(isInput, channelIndex, busIdx);

            int total = isInput ? numIns : numOuts;
            int index = port->pin.isMIDI() ? (total - 1) : channelIndex;
            auto totalSpaces = static_cast<float>(total) +
                               (static_cast<float>(juce::jmax(0, processor->getBusCount(isInput) - 1)) * 0.5f);
            auto indexPos = static_cast<float>(index/2) + (static_cast<float>(busIdx) * 0.5f);
            if (item == nullptr)
                continue;

            // Determine bounds reference for port placement.
            // For MidiTarget, MidiFilter, and Modulation, items may draw their icon in paintButton()
            // instead of populating layer_1_, so use BKItem::getVisualBounds() for those.
            auto transformedBounds = item->layer_1_.getBounds();
            const auto prepType = static_cast<int>(state.getProperty(IDs::type));
            const bool useVisualBounds = (prepType == bitklavier::BKPreparationType::PreparationTypeMidiTarget) ||
                                          (prepType == bitklavier::BKPreparationType::PreparationTypeMidiFilter) ||
                                          (prepType == bitklavier::BKPreparationType::PreparationTypeModulation);
            if (useVisualBounds)
                transformedBounds = item->getVisualBounds();
            // Translate icon-local bounds to this component's coordinate space
            transformedBounds = transformedBounds.withPosition(
                transformedBounds.getX() + item->getX(),
                transformedBounds.getY() + item->getY());
            if (port->pin.isMIDI()) {
                // For MIDI ports, space them independently across the icon width,
                // ignoring audio channel/bus counts so that a single MIDI port is centered.
                int midiTotal = 0;
                int midiIndex = 0;
                int seen = 0;
                for (auto* p : objects)
                {
                    if (p->isInput == isInput && p->pin.isMIDI())
                    {
                        if (p == port) midiIndex = seen;
                        ++seen;
                    }
                }
                midiTotal = seen;
                // Fallback safety
                if (midiTotal <= 0) midiTotal = 1;
                const float midiTotalSpaces = static_cast<float>(midiTotal);
                const float midiIndexPos = static_cast<float>(midiIndex);
                const float x = transformedBounds.getX() + transformedBounds.getWidth() * (
                        (1.0f + midiIndexPos) / (midiTotalSpaces + 1.0f)) - portSize / 2.0f;
                float yBase = isInput
                        ? transformedBounds.getBottom() - portSize / 2 + BKItem::kMeterPixel
                        : transformedBounds.getY() - portSize / 2;
                // Apply optional per-port/per-prep Y offset only for MidiTarget/MidiFilter/Modulation
                float yOffset = 0.0f;
                if (useVisualBounds)
                {
                    const float perPortOffset = (float) port->state.getProperty(IDs::yOffset, 0.0f);
                    const float perPrepDefault = (float) state.getProperty(isInput ? IDs::inputYOffset : IDs::outputYOffset, 0.0f);
                    yOffset = (perPortOffset != 0.0f) ? perPortOffset : perPrepDefault;
                }
                const float y = yBase + yOffset;
                port->setBounds(x, y, portSize, portSize);
            } else {
                const float x = isInput
                      ? transformedBounds.getX() - portSize / 2 - BKItem::kMeterPixel
                      : transformedBounds.getRight() - portSize / 2 + BKItem::kMeterPixel;
                float yBase = transformedBounds.getY() + transformedBounds.getHeight() * (
                      1.0f - ((1.0f + indexPos) / (totalSpaces + 1.0f))) - portSize / 2;
                float yOffset = 0.0f;
                if (useVisualBounds)
                {
                    const float perPortOffset = (float) port->state.getProperty(IDs::yOffset, 0.0f);
                    const float perPrepDefault = (float) state.getProperty(isInput ? IDs::inputYOffset : IDs::outputYOffset, 0.0f);
                    yOffset = (perPortOffset != 0.0f) ? perPortOffset : perPrepDefault;
                }
                const float y = yBase + yOffset;
                port->setBounds(x, y, portSize, portSize);
            }
        }
    }

    SynthSection::resized();
}

PreparationSection::~PreparationSection() {
    freeObjects();
}

BKPort *PreparationSection::createNewObject(const juce::ValueTree &v) {
    return new BKPort(v);
}

void PreparationSection::deleteObject(BKPort *at) {
    if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {
        SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
        //this might cause bugs when adding deletion of a prepartion and dynamic port adding and delting
        at->setVisible(false);
        _parent->getOpenGlWrapper()->context.executeOnGLThread([this, &at](juce::OpenGLContext &openGLContext) {
                                                                   //this->destroyOpenGlComponent()
                                                               },
                                                               false);
    } else delete at;
}

void PreparationSection::reset() {
    SynthSection::reset();
}

void PreparationSection::newObjectAdded(BKPort *object) {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    parent->getGui()->open_gl_.context.executeOnGLThread([this,object](juce::OpenGLContext &context) {
        SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
        object->getImageComponent()->init(_parent->getGui()->open_gl_);
        juce::MessageManagerLock mm;
        this->addOpenGlComponent(object->getImageComponent(), false, true);
        this->addAndMakeVisible(object);
        object->addListener(this);
        this->resized();
    }, false);
}

void PreparationSection::valueTreeRedirected(juce::ValueTree &) {}

void PreparationSection::mouseDrag(const juce::MouseEvent &e) {
    for (auto listener: listeners_) {
        listener->_update();
        listener->preparationDragged(this, e);
    }
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    auto* site = dynamic_cast<ConstructionSite *>(getParentComponent());
    site->startDragging(state.toXmlString(), this, juce::ScaledImage(), true);
    //    if (e.mods.isLeftButtonDown()) {
    //        auto newPos = this->getPosition() + e.getDistanceFromDragStart();
    //        this->setPosition(newPos);
    //    }

}

void PreparationSection::moved()
{
    // //undo.beg
    // x = this->getX();
    // y = this->getY();
}
