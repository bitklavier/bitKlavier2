//
// Created by Dan Trueman on 5/7/25.
//

#ifndef BITKLAVIER2_OPENGL_VELOCITYMINMAXSLIDER_H
#define BITKLAVIER2_OPENGL_VELOCITYMINMAXSLIDER_H
#include "../BKComponents/BKSliders.h"
#include "VelocityMinMaxParams.h"
#include "open_gl_component.h"
#include "synth_slider.h"
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
                                                                              1 //increment
                                                                          ),
                                                                          params(_params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        auto minsliderptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[0].get(),
                                                                        listeners,
                                                                        minSlider, nullptr);
        attachmentVec.emplace_back(std::move(minsliderptr));

        auto maxsliderptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[1].get(),
                                                                        listeners,
                                                                        maxSlider, nullptr);
        attachmentVec.emplace_back(std::move(maxsliderptr));

//        auto displaysliderptr = std::make_unique<chowdsp::SliderAttachment>(*(params->lastVelocityParam.get()),
//            listeners,
//            invisibleSlider, nullptr);
//        attachmentVec.emplace_back(std::move(displaysliderptr));

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
    }

    OpenGL_VelocityMinMaxSlider *clone() {
        return new OpenGL_VelocityMinMaxSlider();
    }

    void BKRangeSliderValueChanged(juce::String name, double min, double max) override {
        if (isModulation_) {
            modulationState.setProperty("velocitymin", min, nullptr);
            modulationState.setProperty("velocitymax", max, nullptr);
        }
    }

    void mouseExit(const juce::MouseEvent &e) override {
        //call hoverEnded for listeners like the modulation manager
        for (auto *listener: listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    void syncToValueTree() override {
        auto minval = modulationState.getProperty("velocitymin");
        auto maxval = modulationState.getProperty("velocitymax");

        setMinValue(minval, juce::sendNotification);
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
        1) // increment
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        isModulation_ = true;
        addMyListener(this);
    }

    // chowdsp::ScopedCallbackList minMaxSliderCallbacks;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER2_OPENGL_VELOCITYMINMAXSLIDER_H
