//
// Created by Davis Polito on 2/13/24.
//

#ifndef BITKLAVIER2_PREPARATIONSECTION_H
#define BITKLAVIER2_PREPARATIONSECTION_H

#include "tracktion_ValueTreeUtilities.h"
#include "synth_section.h"
#include "Identifiers.h"
#include "BKItem.h"
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
#include "open_gl_image_component.h"

class PreparationSection
        : public SynthSection, public BKItem::Listener, public BKPort::Listener, public juce::ChangeListener,
          public tracktion::engine::ValueTreeObjectList<BKPort>, public juce::DragAndDropTarget {
public:
    static constexpr float kItemPaddingY = 2.0f;
    static constexpr float kItemPaddingX = 2.0f;

    // Constructor Declaration
    PreparationSection(juce::String name,const juce::ValueTree& v, OpenGlWrapper &um, juce::AudioProcessorGraph::NodeID node, juce::UndoManager& undo);

    // Destructor Declaration
    ~PreparationSection();
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

        virtual void preparationDeleted(const juce::ValueTree&) {};

        virtual void preparationDragged(juce::Component *prep, const juce::MouseEvent &e) {}

        virtual void preparationDropped(const juce::MouseEvent &e, juce::Point<int> originalComponentPoint) {}

        virtual void modulationDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}
        virtual void resetDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}

        virtual void tuningDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}
        virtual void tempoDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}
        virtual void synchronicDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}

        virtual void midifilterDropped(const juce::ValueTree &source, const juce::ValueTree &dest) {}

        virtual void createWindow(juce::AudioProcessorGraph::Node* node, PluginWindow::Type type){}
    };

    void beginConnectorDrag(juce::AudioProcessorGraph::NodeAndChannel source,
                            juce::AudioProcessorGraph::NodeAndChannel dest,
                            const juce::MouseEvent &e) override{
        for (auto listener: listeners_) {
            listener->beginConnectorDrag(source,
                                         dest,
                                         e);
        }
    }

    void dragConnector(const juce::MouseEvent &e)override {
        for (auto listener: listeners_) {
            listener->dragConnector(e);
        }

    }

    void endDraggingConnector(const juce::MouseEvent &e)override {
        for (auto listener: listeners_) {
            listener->endDraggingConnector(
                    e);
        }
    }

    void addListener(Listener *listener) { listeners_.push_back(listener); }
    std::vector<Listener *> listeners_;

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
        if (i == IDs::x_y)
        {
            this->setCentrePosition (juce::VariantConverter<juce::Point<int>>::fromVar(v.getProperty (i)));
        }
        else if (i == IDs::width || i == IDs::height)
        {
            if (v.hasProperty(IDs::width) && v.hasProperty(IDs::height))
            {
                auto oldCenter = getBounds().getCentre();
                setSize(v.getProperty(IDs::width), v.getProperty(IDs::height));
                setCentrePosition(oldCenter);
            }
        }
    }

    bool isSuitableType(const juce::ValueTree &v) const override {
        return v.hasType(IDs::PORT);
    }

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    // Public function declarations, which override base class (SynthSection) virtual functions
    void paintBackground(juce::Graphics &g) override;

    void resized() override;

    void setDefaultColor() {
        item->setColor(findColour(Skin::kWidgetPrimary1, true));
        item->redoImage();
    }

    void moved() override;

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
        // auto popup = getPrepPopup();
        // showPrepPopup(std::move(popup),state,bitklavier::BKPreparationTypeNil);
        auto popup = getPrepPopup();
        auto safeThis = juce::Component::SafePointer<SynthSection> (this);
        juce::Logger::writeToLog ("prepsectiin double click");
        juce::MessageManager::callAsync ([safeThis, popup = std::move(popup), thisState = state] () mutable
        {
            if (safeThis == nullptr) return;
            safeThis->showPrepPopup (std::move (popup), thisState, bitklavier::BKPreparationTypeNil);
        });
    }

    ///drraganddrop for line view and modulator
    //should be using this function probably instead of if in itemdropped. but i want all preps to go back to
    // original pos if dropped over another prep and this is my current solution
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) override {
        return true;
    }

    void itemDropped(const SourceDetails &dragSourceDetails) override;
    void itemDragMove(const SourceDetails &dragSourceDetails) {}
    void itemDragEnter(const SourceDetails &dragSourceDetails) {}
    void itemDragExit(const SourceDetails &dragSourceDetails) {}

    juce::Point<float> getPinPos(int index, bool isInput) const {
        for (auto *port: objects)
            if (port->pin.channelIndex == index && isInput == port->isInput)
                return getPosition().toFloat() + port->getBounds().getCentre().toFloat();

        return {};
    }

    void setPortInfo();
    virtual std::unique_ptr<SynthSection> getPrepPopup() {}

    void setNodeInfo();
    juce::AudioProcessor* getProcessor() const;
    juce::CachedValue<juce::Uuid> uuid;


    // Public member variables for a PreparationSection object
    juce::ValueTree state;
    OpenGlWrapper &_open_gl;
    juce::SelectedItemSet<PreparationSection *> *selectedSet;
    std::unique_ptr<BKItem> item;
    juce::CachedValue<int>  width, height, numIns, numOuts;
    juce::CachedValue<juce::Point<int>> curr_point;

    juce::ComponentBoundsConstrainer constrainer;
    const juce::AudioProcessorGraph::NodeID pluginID;
    juce::UndoManager& undo;

protected:
    int portSize = 16;

private:
    juce::Point<int> pointBeforDrag; // e.getEventRelativeTo (componentToDrag).getMouseDownPosition();
    bool isSelected = true;

};


#endif // BITKLAVIER2_PREPARATIONSECTION_H
