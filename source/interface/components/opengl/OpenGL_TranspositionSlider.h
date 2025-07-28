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

class OpenGL_TranspositionSlider : public OpenGlAutoImageComponent<BKStackedSlider>, BKStackedSlider::Listener
{
public:
    OpenGL_TranspositionSlider (TransposeParams *_params, chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKStackedSlider>(
                                                                            "Transpositions", // slider name
                                                                            -12, // min
                                                                            12, // max
                                                                            -12, // default min
                                                                            12, // default max
                                                                            0, // default val
                                                                            0.01,
//                                                                            _params->numActive,
                                                                _params->numActiveSliders->getCurrentValue(),
                                                                    _params->paramDefault), // increment
                                                                        params(_params)
    {
        isModulated_ = true;
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

        /**
         * these callbacks are for each of the 12 transposition values in the slider
         * they will be called when a modulation is triggered, changing any/all of the transp values
         * so that the UI can update accordingly.
         */
        int j = 0;
        for (auto& param :*params->getFloatParams()) {
            if ((*params->getFloatParams())[j].get()->paramID == "numActiveSliders") continue;
            sliderChangedCallback +={ listeners.addParameterListener(
                param,
                chowdsp::ParameterListenerThread::MessageThread,[this,j]()
                    {
                        auto sliderVals= getAllActiveValues();
                        auto activeSliders_ = params->numActiveSliders->getCurrentValue();

                        sliderVals.clearQuick();
                        for (int k = 0; k < activeSliders_; k++)
                        {
                            sliderVals.add(dataSliders[k]->getValue());
                        }
                        setTo(sliderVals, juce::sendNotification);
                        redoImage();
                    }
                )
            };
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

    virtual void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseMove(e);
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

    OpenGL_TranspositionSlider* clone() {
        return new OpenGL_TranspositionSlider();
    }

    /*
     * this is called whenever the transposition slider "isModulated_" is edited by the user
     * OR when the modulation transposition slider "isModulation_" is edited by the user
     * this is NOT called when an actual state modulation is executed
     */
    void BKStackedSliderValueChanged(juce::String name, juce::Array<float> val) override {
        if (isModulation_) {
            /*
             * the modulation editor case: we are editing the modulator version of the stacked slider
             * we blank out all the transp values above the last one we have (in case there were ones there previously)
             * and then set the value for the ones we do have
             * so, if we have 4 transpositions, we blank out 4 and above and then set 0 through 3
             */
            for (int i = val.size(); i < 11; i++) {
                auto str = "t" + juce::String(i);
                modulationState.removeProperty(str, nullptr);
            }
            for (int i = 0; i < val.size(); i++) {
                auto str = "t" + juce::String(i);
                modulationState.setProperty(str, val[i], nullptr);
            }
        } else if (isModulated_) {
            /*
             * the normal case: the transposition slider being used by the user
             * we do a similar thing as before, blanking out any pre-existing ones above our last one
             * and then setting the ones we do have.
             * again, if we have 4 transpositions, we blank out all those above that and then set our 4
             *
             * 'isModulated_' is a bit of a misnomer, in that this applies to ANY transposition slider,
             * regardless of whether it has a modulator attached
             */
            for (int i = val.size(); i < 11; i++) {
                auto str = "t" + juce::String(i);
                defaultState.removeProperty(str, nullptr);
            }
            for (int i = 0; i < val.size(); i++) {
                auto str = "t" + juce::String(i);
                defaultState.setProperty(str, val[i], nullptr);
            }

            params->numActiveSliders->setParameterValue(val.size());
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
    /**
     * we use this constructor to create a generic clone for the mods
     * so "isModulation" is set to true here,
     * as this will be used to set the modulation target values
     */
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
