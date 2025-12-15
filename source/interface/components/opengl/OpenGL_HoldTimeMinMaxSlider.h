//
// Created by Dan Trueman on 8/6/25.
//

#ifndef BITKLAVIER0_OPENGL_HOLDTIMEMINMAXSLIDER_H
#define BITKLAVIER0_OPENGL_HOLDTIMEMINMAXSLIDER_H

#include "../BKComponents/BKSliders.h"
#include "HoldTimeMinMaxParams.h"
#include "synth_slider.h"
#include "valuetree_utils/VariantConverters.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_HoldTimeMinMaxSlider : public OpenGlAutoImageComponent<BKRangeSlider>, BKRangeSlider::Listener {
public:
    OpenGL_HoldTimeMinMaxSlider(HoldTimeMinMaxParams *_params,
        chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKRangeSlider>(
                                                      "Accepted Hold Time Range", // slider name
                                                      0.f, // min
                                                      12000.f, // max
                                                      defmin = 0.f, // default min
                                                      defmax = 12000.f, // default max
                                                      1, //increment
                                                      _params->stateChanges.defaultState
                                                      ),
                                                  params(_params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulated_ = true;
        addMyListener(this);

        rangeSliderBorder.setText("Hold Time");
        invisibleSlider.setSkewFactorFromMidPoint(holdTimeMinMax_rangeMid);


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

        /*
         * make sure we have defaultStates for the two params, so the user doesn't have to twiddle with the sliders to get defaults saved
         */
        if (!defaultState.hasProperty("holdtimemin"))
            defaultState.setProperty("holdtimemin", params->holdTimeMinParam->getNormalisableRange().start, nullptr);
        if (!defaultState.hasProperty("holdtimemax"))
            defaultState.setProperty("holdtimemax", params->holdTimeMaxParam->getNormalisableRange().end, nullptr);

        /**
         * this list of callbacks is handled by the chowdsp system.
         * each of these will be called, and the lambda executed, when the parameters are changed anywhere in the code
         */
        sliderChangedCallback += {
            listeners.addParameterListener(_params->holdTimeMinParam,
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
            listeners.addParameterListener(_params->holdTimeMaxParam,
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
                ),
            /**
             * this callback is to notify this slider when the lastVelocityParam is changed.
             * since we're using a legacy bK component for the velocityMinMax slider, we
             * want this callback to trigger the value update and redoImage. We don't
             * need that for the newer OpenGL UI elements, which will just update automatically
             * when the param changes.
             * also, since this is not a slider that the user touches, we don't need a SliderAttachment
             */
            listeners.addParameterListener(_params->lastHoldTimeParam,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    setDisplayValue(this->params->lastHoldTimeParam->getCurrentValue());
                    redoImage();
                }
                )
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
        OpenGlAutoImageComponent<BKRangeSlider>::textEditorReturnKeyPressed(textEditor);
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

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKRangeSlider>::mouseUp(event);
        redoImage();
        mouseInteraction = false;
    }

    OpenGL_HoldTimeMinMaxSlider *clone() {
        return new OpenGL_HoldTimeMinMaxSlider();
    }

    void BKRangeSliderValueChanged(juce::String name, double min, double max) override
    {
        /*
         * we don't want these to be called when a modulation or reset is triggered
         */
        if (!mouseInteraction)
            return;

        if (isModulation_) {
            modulationState.setProperty("holdtimemin", min, nullptr);
            modulationState.setProperty("holdtimemax", max, nullptr);
        }

        else if (isModulated_) {
            defaultState.setProperty("holdtimemin", min, nullptr);
            defaultState.setProperty("holdtimemax", max, nullptr);
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
        if (modulationState.hasProperty ("holdtimemin"))
        {
            auto minval = modulationState.getProperty("holdtimemin");
            setMinValue(minval, juce::sendNotification);
        }
        else
        {
            setMinValue(defmin, juce::sendNotification);
        }

        if (modulationState.hasProperty ("holdtimemax"))
        {
            auto maxval = modulationState.getProperty("holdtimemax");
            setMaxValue(maxval, juce::sendNotification);
        }
        else
        {
            setMaxValue(defmax, juce::sendNotification);
        }
    }

    HoldTimeMinMaxParams *params;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

private :
    OpenGL_HoldTimeMinMaxSlider() : OpenGlAutoImageComponent<BKRangeSlider>(
                                       "HoldTimeRange", // slider name
                                       0.f, // min
                                       12000.f, // max
                                       defmin = 0.f, // default min
                                       defmax = 12000.f, // default max
                                       1.f, // increment
                                       juce::ValueTree{})
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulation_ = true;
        addMyListener(this);

        /*
         * this is so the modulation UI view has a distinctive colored border
         */
        rangeSliderBorder.setColour(juce::GroupComponent::outlineColourId, findColour (Skin::kRotaryArc));
        rangeSliderBorder.setColour(juce::GroupComponent::textColourId, findColour (Skin::kRotaryArc));
        rangeSliderBorder.setText ("MODIFIED");
        rangeSliderBorder.setTextLabelPosition (juce::Justification::centred);

        invisibleSlider.setSkewFactorFromMidPoint(holdTimeMinMax_rangeMid);
        minSlider.setSkewFactorFromMidPoint(holdTimeMinMax_rangeMid);
        maxSlider.setSkewFactorFromMidPoint(holdTimeMinMax_rangeMid);
    }

    int defmin, defmax;

    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER0_OPENGL_HOLDTIMEMINMAXSLIDER_H
