//
// Created by Davis Polito on 2/13/24.
//

#ifndef BITKLAVIER2_PREPARATIONSECTION_H
#define BITKLAVIER2_PREPARATIONSECTION_H

#include "valuetree_utils/VariantConverters.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "tracktion_ValueTreeUtilities.h"
#include "synth_section.h"
//#include "draggable_component.h"
#include "common.h"
#include "Identifiers.h"
#include "BKItem.h"
//#include "opengl_ComponentDragger.h"
#include "BKPort.h"
#include "Sample.h" //need this for samplersound... should find a way not to need this
#include "PluginWindow.h"
/************************************************************************************/
/*                            CLASS: SynthGuiInterface                              */
/************************************************************************************/

class SynthGuiInterface;

/************************************************************************************/
/*     CLASS: PreparationSection, inherits from SynthSection and Listener           */
/************************************************************************************/
class PreparationSection
        : public SynthSection, public BKItem::Listener, public BKPort::Listener, public juce::ChangeListener,
          public tracktion::engine::ValueTreeObjectList<BKPort>, public juce::DragAndDropTarget {
public:
    static constexpr float kItemPaddingY = 2.0f;
    static constexpr float kItemPaddingX = 2.0f;

    class Listener {
    public:
        virtual ~Listener() = default;

//        virtual void portClicked(const juce::Point<int>& pos, juce::AudioProcessorGraph::Node::Ptr) = 0;
        virtual void beginConnectorDrag(juce::AudioProcessorGraph::NodeAndChannel source,
                                        juce::AudioProcessorGraph::NodeAndChannel dest,
                                        const juce::MouseEvent &e) {};

        virtual void dragConnector(const juce::MouseEvent &e) {};

        virtual void endDraggingConnector(const juce::MouseEvent &e) {};

        virtual void _update() {};

        virtual void preparationDragged(juce::Component *prep, const juce::MouseEvent &e) {}

        virtual void preparationDropped(const juce::MouseEvent &e, juce::Point<int> originalComponentPoint) {}

        virtual void modulationDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}

        virtual void tuningDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}

        virtual void createWindow(juce::AudioProcessorGraph::Node* node, PluginWindow::Type type){}
    };

    void beginConnectorDrag(juce::AudioProcessorGraph::NodeAndChannel source,
                            juce::AudioProcessorGraph::NodeAndChannel dest,
                            const juce::MouseEvent &e) {
        for (auto listener: listeners_) {
            listener->beginConnectorDrag(source,
                                         dest,
                                         e);
        }
    }

    void dragConnector(const juce::MouseEvent &e) {
        for (auto listener: listeners_) {
            listener->dragConnector(e);
        }

    }

    void endDraggingConnector(const juce::MouseEvent &e) {
        for (auto listener: listeners_) {
            listener->endDraggingConnector(
                    e);
        }
    }

    void addListener(Listener *listener) { listeners_.push_back(listener); }

    std::vector<Listener *> listeners_;

    // Constructor Declaration
    PreparationSection(juce::String name, juce::ValueTree v, OpenGlWrapper &um, juce::AudioProcessor *proc);

    // Destructor Declaration
    ~PreparationSection();

    virtual void
    addSoundSet(std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>> *s) {}

    virtual void addSoundSet(
            juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>> *s,
            juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>> *h,
            juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>> *r,
            juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>> *p) {}

    // Public member variables for a PreparationSection object
    juce::ValueTree state;
    OpenGlWrapper &_open_gl;
    juce::SelectedItemSet<PreparationSection *> *selectedSet;
    std::unique_ptr<BKItem> item;
    juce::CachedValue<int> x, y, width, height, numIns, numOuts;
    juce::ComponentBoundsConstrainer constrainer;
    BKPort *createNewObject(const juce::ValueTree &v) override;

    void deleteObject(BKPort *at) override;


    void reset() override;

    void newObjectAdded(BKPort *) override;

    void objectRemoved(BKPort *) override { resized(); }//resized(); }
    void objectOrderChanged() override { resized(); }//resized(); }
    // void valueTreeParentChanged (juce::ValueTree&) override;
    void valueTreeRedirected(juce::ValueTree &) override;

    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override {
        tracktion::engine::ValueTreeObjectList<BKPort>::valueTreePropertyChanged(v, i);
    }

    bool isSuitableType(const juce::ValueTree &v) const override {
        return v.hasType(IDs::PORT);
    }


    void changeListenerCallback(juce::ChangeBroadcaster *source) {
        if (selectedSet->isSelected(this)) {
            item->setColor(juce::Colours::white);
            isSelected = true;
        } else {
            item->setColor(findColour(Skin::kWidgetPrimary1, true));
            isSelected = false;
        }
        item->redoImage();

    }

    // Public function declarations, which override base class (SynthSection) virtual functions
    void paintBackground(juce::Graphics &g) override;

    void resized() override;

    void setDefaultColor() {
        item->setColor(findColour(Skin::kWidgetPrimary1, true));
        item->redoImage();
    }

    // Public function definitions, which override base class (SynthSection) virtual functions
    void setSizeRatio(float ratio) override {
        size_ratio_ = ratio;
        item->size_ratio = ratio;
    }

    void mouseDown(const juce::MouseEvent &e) override;

    void mouseEnter(const juce::MouseEvent &e) override {
        setMouseCursor(juce::MouseCursor::ParentCursor);
    }

    void mouseDrag(const juce::MouseEvent &e) override;

    void mouseUp(const juce::MouseEvent &e) override {
        setMouseCursor(juce::MouseCursor::ParentCursor);
        for (auto listener: listeners_) {
            listener->_update();
            listener->preparationDropped(e, pointBeforDrag);
        }
    }

    void mouseDoubleClick(const juce::MouseEvent &event) override {
        showPrepPopup(this);
    }

    ///drraganddrop for line view and modulator
//should be using this function probably instead of if in itemdropped. but i want all preps to go back to
// original pos if dropped over another prep and this is my current solution
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        return true;
    }

    void itemDropped(const SourceDetails &dragSourceDetails) override;


    void itemDragMove(const SourceDetails &dragSourceDetails) {

    }

    void itemDragEnter(const SourceDetails &dragSourceDetails) {
    }

    void itemDragExit(const SourceDetails &dragSourceDetails) {

    }

//    void 	itemDragMove (const SourceDetails &dragSourceDetails)
//    void itemDragEnter (const SourceDetails &dragSourceDetails)
//    void 	itemDragExit (const SourceDetails &dragSourceDetails)
//    bool 	shouldDrawDragImageWhenOver ()
    /************************************************************************************/
    /*            NESTED CLASS: PreparationPopup, inherits from SynthSection            */
    /************************************************************************************/



    juce::Point<float> getPinPos(int index, bool isInput) const {
        for (auto *port: objects)
            if (port->pin.channelIndex == index && isInput == port->isInput)
                return getPosition().toFloat() + port->getBounds().getCentre().toFloat();

        return {};
    }

    void setPortInfo() {

        auto &processor = *this->node->getProcessor();
        //check if main audio input bus is enabled
        if (getProcessor()->getBus(true, 0) != nullptr && getProcessor()->getBus(true, 0)->isEnabled()) {
            for (int i = 0; i < processor.getMainBusNumInputChannels(); ++i) {

                juce::ValueTree v{IDs::PORT};
                v.setProperty(IDs::nodeID,
                              juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                              nullptr);
                v.setProperty(IDs::chIdx, i, nullptr);
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


        if (processor.acceptsMidi()) {
            juce::ValueTree v{IDs::PORT};
            v.setProperty(IDs::nodeID, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
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
        if (getProcessor()->getBus(false, 0) != nullptr && getProcessor()->getBus(false, 0)->isEnabled()) {
            for (int i = 0; i < processor.getTotalNumOutputChannels(); ++i) {

                    juce::ValueTree v{IDs::PORT};
                v.setProperty(IDs::nodeID,
                              juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                              nullptr);
                v.setProperty(IDs::chIdx, i, nullptr);
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
        if (processor.producesMidi()) {
            juce::ValueTree v{IDs::PORT};
            v.setProperty(IDs::nodeID, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
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
        numOuts = numOuts  + 1;
        }
        //this->resized();
    }

    virtual std::unique_ptr<SynthSection> getPrepPopup() {}

    void setNodeInfo(juce::AudioProcessorGraph::Node::Ptr _node) {
        node = _node;
        pluginID = node->nodeID;
        this->state.setProperty(IDs::nodeID,
                                juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(this->pluginID),
                                nullptr);
        int i = 0;
        auto &processor = *_node->getProcessor();
        if (objects.isEmpty()) {
            juce::MessageManager::callAsync([safeComp = juce::Component::SafePointer<PreparationSection>(this)] {
                safeComp->setPortInfo();
            });
        }


    }

    juce::AudioProcessorGraph::Node::Ptr node;
    juce::AudioProcessorGraph::NodeID pluginID;
    juce::CachedValue<juce::Uuid> uuid;

    virtual juce::AudioProcessor *getProcessor() {}

    virtual std::unique_ptr<juce::AudioProcessor> getProcessorPtr() {}

protected:


    int portSize = 16;
private:
    juce::Point<int> pointBeforDrag; // e.getEventRelativeTo (componentToDrag).getMouseDownPosition();
    bool isSelected = true;
    juce::AudioProcessor *_proc;

};


#endif // BITKLAVIER2_PREPARATIONSECTION_H
