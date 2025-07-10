//
// Created by Davis Polito on 2/25/25.
//

#ifndef BITKLAVIER2_STATEMODULATEDCOMPONET_H
#define BITKLAVIER2_STATEMODULATEDCOMPONET_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "open_gl_image_component.h"
#include "Identifiers.h"
struct StateModulatedComponent : juce::Component

{
    StateModulatedComponent(const juce::ValueTree& stateDefault) : defaultState(stateDefault){}
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void hoverStarted(StateModulatedComponent* button) { }
        virtual void hoverEnded(StateModulatedComponent* button) { }
    };

    void addListener(StateModulatedComponent::Listener* listener)
    {
        listeners_.push_back(listener);
    }

    void mouseEnter(const juce::MouseEvent &event) override
    {
        for (auto* listener : listeners_)
            listener->hoverStarted(this);
        hovering_ = true;
    }

    void mouseExit(const juce::MouseEvent &event) override
    {
        for (auto* listener : listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    virtual std::shared_ptr<OpenGlImageComponent> getImageComponent() {}
    std::vector<Listener*> listeners_;
    bool hovering_ = false;
    bool isModulation_ = false;
    bool isModulated  = false;
    virtual StateModulatedComponent* clone() = 0;
    //should be replaced by valuetree wrapper or something along those lines
    virtual void syncToValueTree() = 0;
    juce::ValueTree modulationState;
    juce::ValueTree defaultState;// {IDs::PARAM_DEFAULT};

};
#endif //BITKLAVIER2_STATEMODULATEDCOMPONET_H
