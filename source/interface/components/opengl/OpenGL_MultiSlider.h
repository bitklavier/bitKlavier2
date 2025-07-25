//
// Created by Dan Trueman on 7/24/25.
//

#ifndef BITKLAVIER0_OPENGL_MULTISLIDER_H
#define BITKLAVIER0_OPENGL_MULTISLIDER_H

#include "../BKComponents/BKSliders.h"
#include "MultiSliderState.h"
#include "open_gl_component.h"
#include "synth_slider.h"
#include "valuetree_utils/VariantConverters.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_MultiSlider: public OpenGlAutoImageComponent<BKMultiSlider>, BKMultiSlider::Listener
{
public:
    OpenGL_MultiSlider(MultiSliderState *sliderstate, chowdsp::ParameterListeners &listeners) :
            OpenGlAutoImageComponent<BKMultiSlider>(sliderstate->paramDefault), params(sliderstate)
    {
        isModulated = true;
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        addMyListener(this);

    }

    virtual void resized() override {
        OpenGlAutoImageComponent<BKMultiSlider>::resized();
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKMultiSlider>::mouseDrag(e);
        redoImage();
    }

    void mouseDown(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKMultiSlider>::mouseDown(e);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorReturnKeyPressed(textEditor);
        redoImage();
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorTextChanged(textEditor);
        redoImage();
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKMultiSlider>::mouseUp(event);
        redoImage();
    }

    OpenGL_MultiSlider* clone() {
        return new OpenGL_MultiSlider();
    }

    void multiSliderValueChanged(juce::String name, int whichSlider, juce::Array<float> values) override
    {
        DBG("multiSliderValueChanged");
    }
    void multiSliderAllValuesChanged(juce::String name, juce::Array<juce::Array<float>> values, juce::Array<bool> states) override
    {
        DBG("multiSliderAllValuesChanged");
    }

//    void addSlider(juce::NotificationType newnotify) override {
////        BKMultiSlider::addSlider(newnotify);
////        if (params != nullptr) // has no params if its a cloned component
////        {
////            params->numActive = (params->numActive + 1) % 12;
////            params->numActiveSliders->setParameterValue(params->numActive);
////        }
//    }

//    void BKMultiSliderValueChanged(juce::String name, juce::Array<float> val) override {
//        if (isModulation_) {
////            for (int i = val.size(); i < 11; i++) {
////                auto str = "t" + juce::String(i);
////                modulationState.removeProperty(str, nullptr);
////            }
////            for (int i = 0; i < val.size(); i++) {
////                auto str = "t" + juce::String(i);
////                modulationState.setProperty(str, val[i], nullptr);
////            }
//        } else if (isModulated) {
////            for (int i = val.size(); i < 11; i++) {
////                auto str = "t" + juce::String(i);
////                defaultState.removeProperty(str, nullptr);
////            }
////            for (int i = 0; i < val.size(); i++) {
////                auto str = "t" + juce::String(i);
////                defaultState.setProperty(str, val[i], nullptr);
////            }
////            this->params->numActive = val.size();
//        }
//    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    /*
     * needed for the param-state modulation
     */
    void syncToValueTree() override {
//        juce::Array<float> vals;
//        static juce::var nullVar;
//        for (int i = 0; i < 11; i++) {
//            auto str = "t" + juce::String(i);
//            auto val = modulationState.getProperty(str);
//            if (val == nullVar)
//                break;
//            vals.add(val);
//        }
//
//        setTo(vals, juce::sendNotification);
    }

    void mouseExit(const juce::MouseEvent &e) override {
        if (getTextEditor()->isVisible())
            return;
        for (auto *listener: listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    MultiSliderState *params;

private :
    OpenGL_MultiSlider() : OpenGlAutoImageComponent<BKMultiSlider>()
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        isModulation_ = true;
        addMyListener(this);
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER0_OPENGL_MULTISLIDER_H
