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
    OpenGL_MultiSlider(juce::String sname, MultiSliderState *sliderstate, chowdsp::ParameterListeners &listeners) :
            OpenGlAutoImageComponent<BKMultiSlider>(sliderstate->stateChanges.defaultState), params(sliderstate), name_(sname)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulated_ = true;
        addMyListener(this);

        /*
         * call setTo here, so the slider displays the passed param values in sliderstate
         */
        setToOnlyActive(
            atomicArrayToJuceArrayLimited(params->sliderVals, params->sliderVals_size),
            atomicBoolArrayToJuceArrayLimited(params->activeSliders, params->activeVals_size),
            juce::sendNotification);

    }

    //setBackgroundColor(findColour(Skin::kWidgetBackground, true));

    virtual void resized() override {
        OpenGlAutoImageComponent<BKMultiSlider>::resized();
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKMultiSlider>::mouseDrag(e);
        redoImage();
    }

    virtual void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKMultiSlider>::mouseMove(e);
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
         * todo: allow for multiple values in each slider, for transpositions: perhaps make OpenGL_MultiSlider2d
         */
//        if (isModulation_)
//        {
//            modulationState.setProperty(IDs::absoluteTuning, s, nullptr);
//        }
//        else if (isModulated_)
//        {
//            defaultState.setProperty(IDs::absoluteTuning, s, nullptr);
//        }

        /*
         * create string representations of the multislider vals/states
         */
        juce::String valsStr = getFirstValueFromSubarrays(values);
        juce::String activeStr = arrayBoolToString(states);

        if (isModulated_)
        {
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
            for (auto sval : states)
            {
                params->activeSliders[valCtr].store(states[valCtr++]);
            }
            params->activeVals_size.store (states.size()); // full array of slider states, including inactive ones (false)

            // then update defaultState, with string representation of the multislider arrays

//            defaultState.setProperty(name_ + "_sliderVals", valsStr, nullptr);
//            defaultState.setProperty(name_ + "_sliderVals_size", values.size(), nullptr);
//            defaultState.setProperty(name_ + "_activeVals", activeStr, nullptr);
//            defaultState.setProperty(name_ + "_activeVals_size", states.size(), nullptr);

            defaultState.setProperty("sliderVals", valsStr, nullptr);
            defaultState.setProperty("sliderVals_size", values.size(), nullptr);
            defaultState.setProperty("activeVals", activeStr, nullptr);
            defaultState.setProperty("activeVals_size", states.size(), nullptr);
        }
        else if (isModulation_)
        {
            /*
             * since this means we are manipulating the clone of the multislider, we are setting mod values, not the values
             * themselves, so we don't write values to params->sliderVals, etc...
             *
             * we just write strings of the arrays to modulationState properties
             */
//            modulationState.setProperty(name_ + "_sliderVals", valsStr, nullptr);
//            modulationState.setProperty(name_ + "_sliderVals_size", values.size(), nullptr);
//            modulationState.setProperty(name_ + "_activeVals", activeStr, nullptr);
//            modulationState.setProperty(name_ + "_activeVals_size", states.size(), nullptr);

            modulationState.setProperty("sliderVals", valsStr, nullptr);
            modulationState.setProperty("sliderVals_size", values.size(), nullptr);
            modulationState.setProperty("activeVals", activeStr, nullptr);
            modulationState.setProperty("activeVals_size", states.size(), nullptr);
        }
    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    /*
     * needed for the param-state modulation; is called when state mod popup is opened to set vals there
     */
    void syncToValueTree() override {

        /**
         * todo: redo this with modulationState vals?
         */
//        setToOnlyActive(
//            atomicArrayToJuceArrayLimited(params->sliderVals, params->sliderVals_size),
//            atomicBoolArrayToJuceArrayLimited(params->activeSliders, params->activeVals_size),
//            juce::sendNotification);


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

    juce::String name_;
    MultiSliderState *params;

private :
    /**
     * constructor for cloning UI in state-change mods
     */
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
