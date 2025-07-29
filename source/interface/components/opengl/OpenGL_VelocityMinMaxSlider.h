//
// Created by Dan Trueman on 5/7/25.
//

#ifndef BITKLAVIER2_OPENGL_VELOCITYMINMAXSLIDER_H
#define BITKLAVIER2_OPENGL_VELOCITYMINMAXSLIDER_H
#include "../BKComponents/BKSliders.h"
#include "VelocityMinMaxParams.h"
#include "synth_slider.h"
#include "valuetree_utils/VariantConverters.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_VelocityMinMaxSlider : public OpenGlAutoImageComponent<BKRangeSlider>, BKRangeSlider::Listener {
public:
    OpenGL_VelocityMinMaxSlider(VelocityMinMaxParams *_params,
                                chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKRangeSlider>(
                                                                              "Accepted Velocity Range", // slider name
                                                                              0.f, // min
                                                                              128.f, // max
                                                                              0.f, // default min
                                                                              128.f, // default max
                                                                              1 ,//increment
                                                                             _params->stateChanges.defaultState
                                                                          ),
                                                                          params(_params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulated_ = true;
        addMyListener(this);

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

        /**
         * this list of callbacks is handled by the chowdsp system.
         * each of these will be called, and the lambda executed, when the parameters are changed anywhere in the code
         */
        sliderChangedCallback += {
            listeners.addParameterListener(_params->velocityMinParam,
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
            listeners.addParameterListener(_params->velocityMaxParam,
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
            listeners.addParameterListener(_params->lastVelocityParam,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    setDisplayValue(this->params->lastVelocityParam->getCurrentValue());
                    redoImage();
                }
                )
        };
    }

    virtual void resized() override {
        OpenGlAutoImageComponent<BKRangeSlider>::resized();
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
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

    OpenGL_VelocityMinMaxSlider *clone() {
        return new OpenGL_VelocityMinMaxSlider();
    }

    void BKRangeSliderValueChanged(juce::String name, double min, double max) override {
        if (!mouseInteraction)
            return;
        if (isModulation_) {
            DBG("updating modulationState in BKRangeSliderValueChanged");
            modulationState.setProperty("velocitymin", min, nullptr);
            modulationState.setProperty("velocitymax", max, nullptr);
        } else if (isModulated_) {
            DBG("updating defaultState in BKRangeSliderValueChanged");
            defaultState.setProperty("velocitymin", min, nullptr);
            defaultState.setProperty("velocitymax", max, nullptr);
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
        auto minval = modulationState.getProperty("velocitymin");
        setMinValue(minval, juce::sendNotification);

        auto maxval = modulationState.getProperty("velocitymax");
        setMaxValue(maxval, juce::sendNotification);
    }

    VelocityMinMaxParams *params;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

private :
    OpenGL_VelocityMinMaxSlider() : OpenGlAutoImageComponent<BKRangeSlider>(
        "VelocityRange", // slider name
        0, // min
        128, // max
        0, // default min
        128, // default max
        1, juce::ValueTree{}) // increment
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulation_ = true;
        addMyListener(this);
    }
    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER2_OPENGL_VELOCITYMINMAXSLIDER_H
