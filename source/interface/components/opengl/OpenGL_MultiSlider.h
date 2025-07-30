//
// Created by Dan Trueman on 7/24/25.
//

#ifndef BITKLAVIER0_OPENGL_MULTISLIDER_H
#define BITKLAVIER0_OPENGL_MULTISLIDER_H

#include "../BKComponents/BKSliders.h"
#include "MultiSliderState.h"
#include "open_gl_component.h"
#include "synth_slider.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_MultiSlider: public OpenGlAutoImageComponent<BKMultiSlider>, BKMultiSlider::Listener
{
public:
    OpenGL_MultiSlider(MultiSliderState *sliderstate, chowdsp::ParameterListeners &listeners) :
            OpenGlAutoImageComponent<BKMultiSlider>(sliderstate->stateChanges.defaultState), params(sliderstate)
    {
        isModulated_ = true;
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
        //seems to not ever be called
    }

    void multiSliderAllValuesChanged(juce::String name, juce::Array<juce::Array<float>> values, juce::Array<bool> states) override
    {
        /**
         * todo: allow for multiple values in each slider, for transpositions
         * todo: figure out how to deal with hard cap in the std::arrays for the params
         * todo: figure out the isModulation_/isModulated_ part
         */

        /*
         * this takes the arrays of values and states (whether a particular slider is active or not) from BKMultSlider
         * and maps them to the params sliderVals and activeSliders, whose values should mirror what we see in the UI
         * so, if the user has set sliders 0, 1, and 3 to values, and left slider 2 inactive, then
         *  sliderVals = {val, val, 0, val, 0....}
         *  activeSilders = {true, true, false, true, false, false....}
         */
        /**
         * todo: yeah, don't need this. on the back end, we need the original values array. don't even need the states array, except for storage
         * might want something like this for modulationState setting and serializer
         */
        if (isModulated_)
        {
//            int stateCtr = 0;
//            int valueCounter = 0;
//            for (auto bval : states)
//            {
//                if (bval)
//                {
//                    params->sliderVals[stateCtr].store (values[valueCounter++][0]); // 1d for now....
//                    params->activeSliders[stateCtr].store (true);
//                }
//                else
//                {
//                    params->sliderVals[stateCtr].store (0.);
//                    params->activeSliders[stateCtr].store (false);
//                }
//                stateCtr++;
//            }

            /*
             * just copy the direct UI representations to the param arrays; the prep should know how to use them
             */
            int valCtr = 0;
            for (auto sval : values)
            {
                params->sliderVals[valCtr].store(values[valCtr++][0]); // 1d for now
            }
            params->sliderVals_size.store (values.size()); // how many active slider values do we have

            valCtr = 0;
            for (auto sval : values)
            {
                params->activeSliders[valCtr].store(states[valCtr++]);
            }
            params->activeVals_size.store (states.size()); // full array of slider states, including inactive ones (false)

            /*
             * write string representations of these arrays to default state for this property?
             */
            // defaultState.setProperty(str, val[i], nullptr);
        }
        else if (isModulation_)
        {
            /*
             * write string representations to modulationState.setProperty for reach fo these arrays?
             */
            //modulationState.setProperty(str, val[i], nullptr);
        }
    }

//    void addSlider(juce::NotificationType newnotify) override {
////        BKMultiSlider::addSlider(newnotify);
////        if (params != nullptr) // has no params if its a cloned component
////        {
////            params->numActive = (params->numActive + 1) % 12;
////            params->sliderVals_size->setParameterValue(params->numActive);
////        }
//    }

//    void multiSliderValueChanged(juce::String name, juce::Array<float> val) override {
//        if (isModulation_) {
////            for (int i = val.size(); i < 11; i++) {
////                auto str = "t" + juce::String(i);
////                modulationState.removeProperty(str, nullptr);
////            }
////            for (int i = 0; i < val.size(); i++) {
////                auto str = "t" + juce::String(i);
////                modulationState.setProperty(str, val[i], nullptr);
////            }
//        } else if (isModulated_) {
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
     * needed for the param-state modulation; is called when state mod popup is opened to set vals there
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
