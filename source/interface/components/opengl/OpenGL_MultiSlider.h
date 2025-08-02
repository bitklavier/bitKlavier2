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

    /**
     *
     */
    void updateFromParams()
    {
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
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKMultiSlider>::mouseDown(e);
        redoImage();
        mouseInteraction = false;
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorReturnKeyPressed(textEditor);
        redoImage();
        mouseInteraction = false;
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorFocusLost(textEditor);
        redoImage();
        mouseInteraction = false;
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorEscapeKeyPressed(textEditor);
        redoImage();
        mouseInteraction = false;
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKMultiSlider>::textEditorTextChanged(textEditor);
        redoImage();
        mouseInteraction = false;
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKMultiSlider>::mouseUp(event);
        redoImage();
        mouseInteraction = false;
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

        if (!mouseInteraction)
            return;

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
            defaultState.setProperty(IDs::multislider_vals, valsStr, nullptr);
            defaultState.setProperty(IDs::multislider_size, values.size(), nullptr);
            defaultState.setProperty(IDs::multislider_states, activeStr, nullptr);
            defaultState.setProperty(IDs::multislider_states_size, states.size(), nullptr);
        }
        else if (isModulation_)
        {
            /*
             * since this means we are manipulating the clone of the multislider, we are setting mod values, not the values
             * themselves, so we don't write values to params->sliderVals, etc...
             *
             * we just write strings of the arrays to modulationState properties
             */
            modulationState.setProperty(IDs::multislider_vals, valsStr, nullptr);
            modulationState.setProperty(IDs::multislider_size, values.size(), nullptr);
            modulationState.setProperty(IDs::multislider_states, activeStr, nullptr);
            modulationState.setProperty(IDs::multislider_states_size, states.size(), nullptr);
        }
    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    /*
     * needed for the param-state modulation; is called when state mod popup is opened to set vals there
     */
    void syncToValueTree() override {

        auto msvals = modulationState.getProperty(IDs::multislider_vals);
        auto msvals_size = int(modulationState.getProperty(IDs::multislider_size));
        auto msavals = modulationState.getProperty(IDs::multislider_states);
        auto msavals_size = int(modulationState.getProperty(IDs::multislider_states_size));

        if (!modulationState.hasProperty(IDs::multislider_vals))
        {
            DBG("using defaultState since we don't have a modulationState yet");

            /**
             * todo: not working...
             * we don't seem to have a defaultState either, probably because we are using the clone
             * not a huge deal, since can copy-paste from main slider, so leave for now
             */
             msvals = defaultState.getProperty(IDs::multislider_vals);
             msvals_size = int(defaultState.getProperty(IDs::multislider_size));
             msavals = defaultState.getProperty(IDs::multislider_states);
             msavals_size = int(defaultState.getProperty(IDs::multislider_states_size));

            return;
        }

        std::array<std::atomic<float>, MAXMULTISLIDERLENGTH> dispvals;
        stringToAtomicArray(dispvals, msvals, 0.);

        std::array<std::atomic<bool>, MAXMULTISLIDERLENGTH> dispstates;
        stringToAtomicBoolArray(dispstates, msavals, false);

        setToOnlyActive(
             atomicArrayToJuceArrayLimited(dispvals, msvals_size),
             atomicBoolArrayToJuceArrayLimited(dispstates, msavals_size),
             juce::sendNotification);
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

    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER0_OPENGL_MULTISLIDER_H
