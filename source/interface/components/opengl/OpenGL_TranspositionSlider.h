//
// Created by Davis Polito on 11/13/24.
//

#ifndef BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
#define BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
#include "../BKComponents/BKSliders.h"
#include "TransposeParams.h"
#include "open_gl_component.h"
#include "synth_slider.h"
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
                                                                            _params->numActive, _params->paramDefault), // increment
                                                                        params(_params) {
        isModulated = true;
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        addMyListener(this);
        int i = 0;
        for (auto slider: dataSliders) {
            if ((*params->getFloatParams())[i].get()->paramID == "numActiveSliders") continue;
            auto ptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[i++].get(),
                listeners,
                *slider,
                nullptr);
            // (*params->getFloatParams())[i++].get()->getCurrentValue() > 0.f;
            attachmentVec.emplace_back(std::move(ptr));
        }
        // juce::Array<float> sliderVals = getAllActiveValues();
        // setTo(sliderVals, juce::sendNotification);
        // add slider callbacks to allow the UI to update the number of sliders whenever a modulation changes it
        sliderChangedCallback += {listeners.addParameterListener(params->numActiveSliders, chowdsp::ParameterListenerThread::MessageThread,
            [this] {
                auto sliderVals   = getAllActiveValues();
                sliderVals.removeRange(params->numActiveSliders->getCurrentValue(),sliderVals.size());
                setTo(sliderVals, juce::sendNotification);
                redoImage();
            })};

        int j = 0;
        for (auto& param :*params->getFloatParams()) {
            if ((*params->getFloatParams())[j].get()->paramID == "numActiveSliders") continue;
            sliderChangedCallback +={ listeners.addParameterListener(
                param,
                chowdsp::ParameterListenerThread::MessageThread,
                    [this,j]() {
                        if ( j > this->params->numActive - 1) {
                                                    juce::Array<float> sliderVals = getAllActiveValues();
                                                    sliderVals.add(dataSliders[j]->getValue());
                                                    setTo(sliderVals, juce::sendNotification);
                            if (isModulation_) {
                                this->listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
                                    getName(),
                                    getAllActiveValues());
                            }
                                                }
                                               redoImage();
                       //   if (!this->isEditing and j > this->params->numActive - 1) return;
                       //      juce::Array<float> sliderVals = getAllActiveValues();
                       //      if (this->params->numActive < j) {
                       //          this->params->numActive = j +1;
                       //      }
                       //      if (sliderVals.size() >= j+1) {
                       //          sliderVals.set(j,dataSliders[j]->getValue());
                       //          sliderVals.removeRange(j+1,sliderVals.size());
                       //          setTo(sliderVals, juce::sendNotification);
                       //      }else {
                       //          sliderVals.set(j,dataSliders[j]->getValue());
                       //          setTo(sliderVals, juce::sendNotification);
                       //      }
                       //      // this->listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
                       //      //     getName(),
                       //      //     getAllActiveValues());
                       //  // }
                       //
                       // redoImage();
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
        {
            params->numActive = (params->numActive + 1) % 12;
            params->numActiveSliders->setParameterValue(params->numActive);
        }
    }

    void BKStackedSliderValueChanged(juce::String name, juce::Array<float> val) override {
        if (isModulation_) {

            for (int i = val.size(); i < 11; i++) {
                auto str = "t" + juce::String(i);
                modulationState.removeProperty(str, nullptr);
            }
            for (int i = 0; i < val.size(); i++) {
                auto str = "t" + juce::String(i);
                modulationState.setProperty(str, val[i], nullptr);
            }
        } else if (isModulated) {
            for (int i = val.size(); i < 11; i++) {
                auto str = "t" + juce::String(i);
                defaultState.removeProperty(str, nullptr);
            }
            for (int i = 0; i < val.size(); i++) {
                auto str = "t" + juce::String(i);
                defaultState.setProperty(str, val[i], nullptr);
            }
            this->params->numActive = val.size();
        }
    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    /*
     * needed for the param-state modulation
     */
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
