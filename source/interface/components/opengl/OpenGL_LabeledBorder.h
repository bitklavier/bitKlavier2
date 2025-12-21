//
// Created by Dan Trueman on 11/29/25.
//

#ifndef BITKLAVIER0_OPENGL_LABELEDBORDER_H
#define BITKLAVIER0_OPENGL_LABELEDBORDER_H

#pragma once

#include "BKLookAndFeel.h"
#include "FullInterface.h"
#include "open_gl_component.h"

class BKGroupComponent : public juce::Component
{
public:
    BKGroupComponent(juce::String name, juce::String label)
    {
        groupBorder.setName(name);
        groupBorder.setText(label);
        groupBorder.setTextLabelPosition(juce::Justification::centred);
        addAndMakeVisible(groupBorder);
    }

    virtual ~BKGroupComponent() = default;

    void resized() override
    {
        juce::Rectangle<int> area (getLocalBounds());
        groupBorder.setBounds(area);
    }

private:
    juce::GroupComponent groupBorder;
};

class OpenGL_LabeledBorder : public OpenGlAutoImageComponent<BKGroupComponent>
{
public:
    OpenGL_LabeledBorder(juce::String name, juce::String label) : OpenGlAutoImageComponent<BKGroupComponent>(name, label)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        setInterceptsMouseClicks(false, false);
    }

    virtual void resized() override {
        OpenGlAutoImageComponent<BKGroupComponent>::resized();
        redoImage();
    }
};


#endif //BITKLAVIER0_OPENGL_LABELEDBORDER_H
