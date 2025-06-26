//
// Created by Davis Polito on 11/13/24.
//

#ifndef BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
#define BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
#include "../BKComponents/BKSliders.h"
#include "TransposeParams.h"
#include "open_gl_component.h"
#include "valuetree_utils/VariantConverters.h"

#include "juce_data_structures/juce_data_structures.h"

class OpenGL_TranspositionSlider : public OpenGlAutoImageComponent<BKStackedSlider>, BKStackedSlider::Listener {
public:
    OpenGL_TranspositionSlider (TransposeParams *_params,
                              chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKStackedSlider>(
                                                                            "Transpositions", // slider name
                                                                            -12, // min
                                                                            12, // max
                                                                            -12, // default min
                                                                            12, // default max
                                                                            0, // default val
                                                                            0.01,
                                                                            _params->numActive), // increment
                                                                        params(_params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        int i = 0;
        for (auto slider: dataSliders) {
            auto ptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[i++].get(),
                listeners,
                *slider,
                nullptr);
            attachmentVec.emplace_back(std::move(ptr));
        }

        // add slider callbacks to allow the UI to update the number of sliders whenever a modulation changes it
        int j = 0;
        for (auto& param :*params->getFloatParams()) {
            sliderChangedCallback +={ listeners.addParameterListener(
                param,
                chowdsp::ParameterListenerThread::MessageThread,
                    [this,j]() {

                        if ( this->isEditing and j > this->params->numActive - 1) {
                            juce::Array<float> sliderVals = getAllActiveValues();
                            sliderVals.add(dataSliders[j]->getValue());
                            setTo(sliderVals, juce::sendNotification);
                            this->listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
                                getName(),
                                getAllActiveValues());
                        }
                       redoImage();
                    })};
            j++;
        }
    }

    virtual void resized() override {
        OpenGlAutoImageComponent<BKStackedSlider>::resized();
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseDrag(e);
        redoImage();
    }

    void mouseDown(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseDown(e);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorReturnKeyPressed(textEditor);
        redoImage();
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorTextChanged(textEditor);
        redoImage();
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseUp(event);
        redoImage();
    }

    OpenGL_TranspositionSlider*clone() {
        return new OpenGL_TranspositionSlider();
    }

    void addSlider(juce::NotificationType newnotify) override {
        BKStackedSlider::addSlider(newnotify);
        if (params != nullptr) // has no params if its a cloned component
            params->numActive = (params->numActive + 1) % 12;
    }

    void BKStackedSliderValueChanged(juce::String name, juce::Array<float> val) override {
        if (isModulation_) {
            for (int i = 0; i < val.size(); i++) {
                auto str = "t" + juce::String(i);
                modulationState.setProperty(str, val[i], nullptr);
            }
            for (int i = val.size(); i < 11; i++) {
                auto str = "t" + juce::String(i);
                modulationState.removeProperty(str, nullptr);
            }
        }
    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    void syncToValueTree() override {
        juce::Array<float> vals;
        static juce::var nullVar;
        for (int i = 0; i < 11; i++) {
            auto str = "t" + juce::String(i);
            auto val = modulationState.getProperty(str);
            if (val == nullVar)
                break;
            vals.add(val);
        }

        setTo(vals, juce::sendNotification);
    }

    void mouseExit(const juce::MouseEvent &e) override {
        if (getTextEditor()->isVisible())
            return;
        for (auto *listener: listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    TransposeParams *params;

private :
    OpenGL_TranspositionSlider() : OpenGlAutoImageComponent<BKStackedSlider>(
        "Transpositions", // slider name
        -12, // min
        12, // max
        -12, // default min
        12, // default max
        0, // default val
        0.01,1) // increment

    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        isModulation_ = true;
        addMyListener(this);
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
