// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Dan Trueman on 8/6/25.
//

#ifndef BITKLAVIER0_CLUSTEROPENGL_MIINMAXSLIDER_H
#define BITKLAVIER0_CLUSTEROPENGL_MIINMAXSLIDER_H

#include "../BKComponents/BKSliders.h"
#include "ClusterMinMaxParams.h"
#include "synth_slider.h"
#include "valuetree_utils/VariantConverters.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_ClusterMinMaxSlider : public OpenGlAutoImageComponent<BKRangeSlider>, BKRangeSlider::Listener {
public:
    OpenGL_ClusterMinMaxSlider(ClusterMinMaxParams *_params,
        chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKRangeSlider>(
                                                      "Accepted Cluster Range", // slider name
                                                      1.f, // min
                                                      12.f, // max
                                                      defmin = 1.f, // default min
                                                      defmax = 12.f, // default max
                                                      1, //increment
                                                      _params->stateChanges.defaultState
                                                      ),
                                                  params(_params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        setMinValue (params->clusterMinParam->getNormalisableRange().start, juce::dontSendNotification);
        setMaxValue (params->clusterMinParam->getNormalisableRange().end, juce::dontSendNotification);
        defmin = params->clusterMinParam->getNormalisableRange().start;
        defmax = params->clusterMaxParam->getNormalisableRange().end;

        isModulated_ = true;
        addMyListener(this);

        rangeSliderBorder.setText("Cluster Min/Max");
        invisibleSlider.setSkewFactorFromMidPoint(clusterMinMax_rangeMid);
        displaySlider->setSkewFactorFromMidPoint(clusterMinMax_rangeMid);

        /**
         * these SliderAttachments contain the listeners for each slider, and so will be notified when the slider is manipulated
         * the attachmentVec is just a place to put them, since some objects have might have many or variable number of them.
         * these attach the sliders changes to the chowdsp params, so they can be managed appropriately, with the callbacks
         * in the "sliderChangedCallback" list called
         *
         * we don't need one for the displaySlider, since that one will not be changed by the user, so we don't need to listen to it
         * but, we do need a parameter listener for displaySlider, which is added to the sliderChangeCallback list.
         */
        auto minsliderptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[0].get(),
            listeners,
            minSlider, nullptr);
        attachmentVec.emplace_back(std::move(minsliderptr));

        auto maxsliderptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[1].get(),
            listeners,
            maxSlider, nullptr);
        attachmentVec.emplace_back(std::move(maxsliderptr));

        minValueTF.setText(juce::String(minSlider.getValue()),juce::dontSendNotification);
        maxValueTF.setText(juce::String(maxSlider.getValue()),juce::dontSendNotification);

        // If the parameter range was expanded before this constructor ran (e.g., a gallery
        // was loaded and SynchronicProcessor::deserialize widened range.end), the SliderAttachment
        // above already set maxSlider's NormalisableRange to the expanded bounds. Sync displaySlider
        // to match; checkValue won't have been called yet so it can't do this for us.
        {
            auto loadedMin = (float) minSlider.getMinimum();
            auto loadedMax = (float) maxSlider.getMaximum();
            if (loadedMax > clusterMinMax_rangeMax || loadedMin < clusterMinMax_rangeMin)
                displaySlider->setRange(loadedMin, loadedMax, sliderIncrement);
        }

        /*
         * make sure we have defaultStates for the two params, so the user doesn't have to twiddle with the sliders to get defaults saved
         */
        if (!defaultState.hasProperty("clustermin"))
            defaultState.setProperty("clustermin", params->clusterMinParam->getNormalisableRange().start, nullptr);
        if (!defaultState.hasProperty("clustermax"))
            defaultState.setProperty("clustermax", params->clusterMaxParam->getNormalisableRange().end, nullptr);

        /**
         * this list of callbacks is handled by the chowdsp system.
         * each of these will be called, and the lambda executed, when the parameters are changed anywhere in the code
         */
        sliderChangedCallback += {
            listeners.addParameterListener(_params->clusterMinParam,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    minValueTF.setText(juce::String(minSlider.getValue()),
                        juce::dontSendNotification);
                    this->listeners.call(&BKRangeSlider::Listener::BKRangeSliderValueChanged,
                        getName(),
                        minSlider.getValue(),
                        maxSlider.getValue());
                    redoImage();
                }
                ),
            listeners.addParameterListener(_params->clusterMaxParam,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    maxValueTF.setText(juce::String(maxSlider.getValue()),
                        juce::dontSendNotification);
                    this->listeners.call(&BKRangeSlider::Listener::BKRangeSliderValueChanged,
                        getName(),
                        minSlider.getValue(),
                        maxSlider.getValue());
                    redoImage();
                }
                )
            // Display value is no longer driven by a parameter listener — the owning
            // ParametersView's Timer polls ClusterMinMaxParams::lastClusterParam (atomic)
            // and calls setDisplayValue() directly. See VelocityMinMaxSlider / KeymapParameterView
            // for the same pattern.
        };
    }

    virtual void resized() override {
        OpenGlAutoImageComponent<BKRangeSlider>::resized();
        redoImage();
    }

    /*
    * this is for making the modulation UI view opaque
    */
    void paint(juce::Graphics& g) override {
        if (isModulation_)
        {
            g.fillAll(juce::Colours::black); // choose your opaque BG
            BKRangeSlider::paint(g);
        }
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKRangeSlider>::mouseDrag(e);
        redoImage();
    }

    void mouseDown(const juce::MouseEvent &e) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKRangeSlider>::mouseDown(e);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        double newval = textEditor.getText().getDoubleValue();

        // Expand the chowdsp parameter ranges BEFORE the parent calls setValue (via SliderAttachment).
        // Without this, the attachment normalizes the new slider value against the old (unexpanded)
        // parameter range and clamps it, losing the typed out-of-range value.
        if (textEditor.getName() == "minvalue" && newval < params->clusterMinParam->range.start)
            params->clusterMinParam->range.start = (float)newval;
        if (textEditor.getName() == "maxvalue" && newval > params->clusterMaxParam->range.end)
            params->clusterMaxParam->range.end = (float)newval;

        mouseInteraction = true;
        OpenGlAutoImageComponent<BKRangeSlider>::textEditorReturnKeyPressed(textEditor);
        mouseInteraction = false;
        redoImage();
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKRangeSlider>::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKRangeSlider>::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKRangeSlider>::textEditorTextChanged(textEditor);
        redoImage();
    }

    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override
    {
        if (key.isKeyCode (juce::KeyPress::leftKey)
            || key.isKeyCode (juce::KeyPress::rightKey)
            || key.isKeyCode (juce::KeyPress::upKey)
            || key.isKeyCode (juce::KeyPress::downKey)
            || key.isKeyCode (juce::KeyPress::homeKey)
            || key.isKeyCode (juce::KeyPress::endKey))
        {
            redoImage();
        }

        return false;
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKRangeSlider>::mouseUp(event);
        redoImage();
        mouseInteraction = false;
    }

    OpenGL_ClusterMinMaxSlider *clone() {
        return new OpenGL_ClusterMinMaxSlider();
    }

    void BKRangeSliderValueChanged(juce::String name, double min, double max) override
    {
        /*
         * we don't want these to be called when a modulation or reset is triggered
         */
        if (!mouseInteraction)
            return;

        if (isModulation_) {
            modulationState.setProperty("clustermin", min, nullptr);
            modulationState.setProperty("clustermax", max, nullptr);
        }

        else if (isModulated_) {
            if (min < params->clusterMinParam->range.start)
            {
                params->clusterMinParam->range.start = min;
            }

            if (max > params->clusterMaxParam->range.end)
            {
                params->clusterMaxParam->range.end = max;
            }

            defaultState.setProperty("clustermin", min, nullptr);
            defaultState.setProperty("clustermax", max, nullptr);
        }
    }

    void mouseExit(const juce::MouseEvent &e) override
    {
        //call hoverEnded for listeners like the modulation manager
        for (auto *listener: listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    /**
     * syncToValueTree() is called in ModulationManager::modulationClicked and
     * is used to set the mod view of the parameter to the most recent mod values
     *
     * todo: have this update to the current actual slider values if mod values haven't been created yet
     *          tried, but ran into some difficulties, leaving for now
     */
    void syncToValueTree() override
    {
        if (modulationState.hasProperty ("clustermin"))
        {
            auto minval = modulationState.getProperty("clustermin");
            setMinValue(minval, juce::sendNotification);
        }
        else
        {
            setMinValue(defmin, juce::sendNotification);
        }

        if (modulationState.hasProperty ("clustermax"))
        {
            auto maxval = modulationState.getProperty("clustermax");
            setMaxValue(maxval, juce::sendNotification);
        }
        else
        {
            setMaxValue(defmax, juce::sendNotification);
        }
    }

    ClusterMinMaxParams *params;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

private :
    OpenGL_ClusterMinMaxSlider() : OpenGlAutoImageComponent<BKRangeSlider>(
                                        "ClusterRange", // slider name
                                        1.f, // min
                                        12.f, // max
                                        defmin = 1.f, // default min
                                        defmax = 12.f, // default max
                                        1, // increment
                                        juce::ValueTree{})
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        // NOTE: `params` is null in the clone — do not dereference it here.
        // Range/defaults are already supplied by the parent BKRangeSlider initializer above.

        isModulation_ = true;
        addMyListener(this);

        /*
         * this is so the modulation UI view has a distinctive colored border
         */
        rangeSliderBorder.setColour(juce::GroupComponent::outlineColourId, findColour (Skin::kRotaryArc));
        rangeSliderBorder.setColour(juce::GroupComponent::textColourId, findColour (Skin::kRotaryArc));
        rangeSliderBorder.setText ("MODIFIED");
        rangeSliderBorder.setTextLabelPosition (juce::Justification::centred);

        invisibleSlider.setSkewFactorFromMidPoint(clusterMinMax_rangeMid);
        minSlider.setSkewFactorFromMidPoint(clusterMinMax_rangeMid);
        maxSlider.setSkewFactorFromMidPoint(clusterMinMax_rangeMid);
        displaySlider->setSkewFactorFromMidPoint(clusterMinMax_rangeMid);
    }

    int defmin, defmax;

    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER0_OPENGL_CLUSTERMIINMAXSLIDER_H
