//
// Created by Davis Polito on 2/25/25.
//

#ifndef BITKLAVIER2_AGGREGATECOMPONENT_H
#define BITKLAVIER2_AGGREGATECOMPONENT_H
#include "juce_gui_basics/juce_gui_basics.h"
#include "open_gl_image_component.h"
class SynthSection;
class OpenGlImageComponent;

class LegacyComponentWrapper : public juce::Component{
public:
    LegacyComponentWrapper(){}
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void hoverStarted(LegacyComponentWrapper* button) { }
        virtual void hoverEnded(LegacyComponentWrapper* button) { }
    };

    void addListener(LegacyComponentWrapper::Listener* listener)
    {
        listeners_.push_back(listener);
    }

    void mouseEnter(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    virtual std::shared_ptr<OpenGlImageComponent> getImageComponent() = 0;
    virtual std::unique_ptr<LegacyComponentWrapper> clone() const = 0;
    virtual void redoImage() = 0;
    std::vector<Listener*> listeners_;
    bool hovering_;


};

template <typename ComponentType>
class OpenGlAutoImageComponentWrapper : public LegacyComponentWrapper {
public:
    explicit OpenGlAutoImageComponentWrapper(std::function<std::shared_ptr<OpenGlAutoImageComponent<ComponentType>>()> factory)
   :componentFactory_(factory), component_(componentFactory_())


  {}

//    // Initialize the OpenGL component
//    void initComponent(OpenGlWrapper& open_gl) override {
//        component_->init(open_gl);
//    }
//
//    // Render the OpenGL component
//    void renderComponent(OpenGlWrapper& open_gl) override {
//        component_->render(open_gl);
//    }

    // Handle resize callbacks
//    void resized() override {
//        if (auto c = component_.get()) {
//            c->resized();
//        }
//    }
//
//    // Handle visibility change callbacks
//    void visibilityChanged() override {
//        if (auto c = component_.get()) {
//            c->visibilityChanged();
//        }
//    }

    // Clone the wrapper itself
    std::unique_ptr<LegacyComponentWrapper> clone() const override {
        // Create a new wrapper and copy the current component's state
        auto clonedWrapper = std::make_unique<OpenGlAutoImageComponentWrapper<ComponentType>>(componentFactory_);
        //clonedWrapper->component_ = std::make_shared<OpenGlAutoImageComponent<ComponentType>>(*component_);
        return clonedWrapper;
    }

    // Clone and return the underlying auto image component
//    std::shared_ptr<OpenGlImageComponent> getClonedComponent() const override {
//        // Note: Deep copy
//        return std::make_shared<OpenGlAutoImageComponent<ComponentType>>(*component_);
//    }
//
    // Access the stored templated component directly
    std::shared_ptr<OpenGlAutoImageComponent<ComponentType>> getActualComponent() const {
        return component_;
    }
    std::shared_ptr<OpenGlImageComponent> getImageComponent() override {return component_->getImageComponent();}
    std::function<std::shared_ptr<OpenGlAutoImageComponent<ComponentType>>()> componentFactory_;
    std::shared_ptr<OpenGlAutoImageComponent<ComponentType>> component_;
    void redoImage() override
    {
        component_->redoImage();
    }
private:

};

#endif //BITKLAVIER2_AGGREGATECOMPONENT_H
