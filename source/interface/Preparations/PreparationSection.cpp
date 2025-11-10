//
// Created by Davis Polito on 2/16/24.
//

#include "PreparationSection.h"
#include "synth_gui_interface.h"

#include "ConstructionSite.h"
#include "FullInterface.h"
#include "ModulationPreparation.h"
#include "synth_base.h"

PreparationSection::PreparationSection(juce::String name, const juce::ValueTree& v, OpenGlWrapper &open_gl,
                                       juce::AudioProcessorGraph::NodeID node, juce::UndoManager &um) :
tracktion::engine::ValueTreeObjectList<BKPort>(v),
    SynthSection(name), state(v), _open_gl(open_gl),
    pluginID(node), undo(um)
{
    //_parent = findParentComponentOfClass<SynthGuiInterface>();
    setInterceptsMouseClicks(true, false);
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



}

juce::AudioProcessor *PreparationSection::getProcessor() const {
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>()) {
        if (auto node = parent->getSynth()->getNodeForId(pluginID))
            return node->getProcessor();

        return {};
    }
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
    {
        if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
            if (auto *node = parent->getSynth()->getNodeForId(pluginID)) {
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
            }
    }
}

void PreparationSection::mouseDown(const juce::MouseEvent &e) {
    pointBeforDrag = this->getPosition();
    //todo investigate need for this only in release build
    if (e.getNumberOfClicks() == 2) {
        mouseDoubleClick(e);
    }
}

void PreparationSection::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {

    dynamic_cast<ConstructionSite *>(getParentComponent())->item_dropped_on_prep_ = true;
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
}

void PreparationSection::resized() {
    juce::Rectangle<float> bounds = getLocalBounds().toFloat();
    int item_padding_y = kItemPaddingY * size_ratio_;
    int item_height = getHeight() - 2 * item_padding_y;
    int item_padding_x = kItemPaddingX * size_ratio_;
    int item_width = getWidth() - 2 * item_padding_x;
    auto newBounds = getBoundsInParent();

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
            auto transformedBounds = item->layer_1_.getBounds();
            if (port->pin.isMIDI()) {
                port->setBounds(
                    transformedBounds.getX() + transformedBounds.getWidth() * (
                        (1.0f + indexPos) / (totalSpaces + 1.0f)),
                    isInput
                        ? transformedBounds.getBottom() - portSize / 2 + BKItem::kMeterPixel
                        : transformedBounds.getY() - portSize / 2,
                    portSize, portSize);
            } else {
                port->setBounds(isInput
                                    ? transformedBounds.getX() - portSize / 2 - BKItem::kMeterPixel
                                    : transformedBounds.getRight() - portSize / 2 + BKItem::kMeterPixel,
                                transformedBounds.getY() + transformedBounds.getHeight() * (
                                    (1.0f + indexPos) / (totalSpaces + 1.0f)) - portSize / 2,
                                portSize, portSize);
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
#include"FullInterface.h"
void PreparationSection::deleteObject(BKPort *at) {
    if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {
        SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
        //this might cause bugs when adding deletion of a prepartion and dynamic port adding and delting
        at->setVisible(false);
        _parent->getOpenGlWrapper()->context.executeOnGLThread([this, &at](juce::OpenGLContext &openGLContext) {
                                                                   //this->destroyOpenGlComponent()
                                                               },
                                                               false);
    } else
        delete at;
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

void PreparationSection::valueTreeRedirected(juce::ValueTree &) {
}


void PreparationSection::mouseDrag(const juce::MouseEvent &e) {
    for (auto listener: listeners_) {
        listener->_update();
        listener->preparationDragged(this, e);
    }
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    dynamic_cast<ConstructionSite *>(getParentComponent())->startDragging(
        state.toXmlString(), this, juce::ScaledImage(), true);
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