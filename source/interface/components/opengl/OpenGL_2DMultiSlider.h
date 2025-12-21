//
// Created by Dan Trueman on 11/13/25.
//

#ifndef BITKLAVIER0_OPENGL_2DMULTISLIDER_H
#define BITKLAVIER0_OPENGL_2DMULTISLIDER_H

#include "../BKComponents/BKSliders.h"
#include "MultiSlider2DState.h"
#include "open_gl_component.h"
#include "synth_slider.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_2DMultiSlider: public OpenGlAutoImageComponent<BKMultiSlider>, BKMultiSlider::Listener
{
public:
    OpenGL_2DMultiSlider(juce::String sname, MultiSlider2DState *sliderstate, chowdsp::ParameterListeners &listeners) :
                         OpenGlAutoImageComponent<BKMultiSlider>(sliderstate->stateChanges.defaultState), params(sliderstate), name_(sname)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        allowSubSliders = true; //2D

        isModulated_ = true;
        addMyListener(this);

        updateFromParams(juce::dontSendNotification);
    }

    juce::Array<juce::Array<float>> getUIValuesFromSliderVals(
        const std::array<std::array<std::atomic<float>, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH>& sliderVals,
        const std::atomic<int>& sliderVals_size,
        const std::array<std::atomic<int>, MAXMULTISLIDER2DLENGTH>& sliderDepths)
    {
        juce::Array<juce::Array<float>> result;

        // 1. Atomically read the overall size LAST.
        // This provides a consistent boundary. We rely on the fact that the
        // writing function updates sliderVals *before* updating sliderVals_size.
        const int N_used = sliderVals_size.load();

        // Loop through the active rows
        for (int i = 0; i < N_used; ++i)
        {
            // 2. Atomically read the depth for this row.
            const int M_used_i = sliderDepths[i].load();

            juce::Array<float> innerArray;

            // Loop through the active values in the current row
            for (int j = 0; j < M_used_i; ++j)
            {
                // 3. Atomically load the float value and add it to the inner array.
                innerArray.add(sliderVals[i][j].load());
            }

            // Add the populated inner array (row) to the result.
            // We only add the row if it had active values, though M_used_i > 0
            // should be guaranteed by the setting logic (unless M_used_i was set to 0
            // while N_used still included the index 'i').
            if (M_used_i > 0)
            {
                result.add(innerArray);
            }
        }

        return result;
    }

    /*
     * updates the sliders from the current parameter values: useful after a mod or reset
     */
    void updateFromParams(juce::NotificationType notify = juce::sendNotification)
    {
        setToOnlyActive(
            getUIValuesFromSliderVals(params->sliderVals, params->sliderVals_size, params->sliderDepths),
            atomicBoolArrayToJuceArrayLimited(params->activeSliders, params->activeVals_size),
            notify);
    }

    //setBackgroundColor(findColour(Skin::kWidgetBackground, true));

    virtual void resized() override {
        OpenGlAutoImageComponent<BKMultiSlider>::resized();
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        mouseInteraction = true;
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

    OpenGL_2DMultiSlider* clone() {
        // auto msvals = modulationState.getProperty(name_ + "_vals");
        // auto msvals_size = int(modulationState.getProperty(name_  + "_size"));
        // auto msavals = modulationState.getProperty(name_ + "_states");
        // auto msavals_size = int(modulationState.getProperty(name_ + "_states_size"));
        // defaultState.setProperty( name_ + "_vals", msvals, nullptr);
        // defaultState.setProperty( name_ + "_size", msvals_size, nullptr);
        // defaultState.setProperty(name_ + "_states", msavals, nullptr);
        // defaultState.setProperty(name_ + "_states_size", msavals_size, nullptr);
        auto sliderClone = new OpenGL_2DMultiSlider();
        sliderClone->setMinMaxDefaultInc (getMinMaxDefaultInc());
        sliderClone->name_ = name_;
        // sliderClone->setName(name_);
        return sliderClone;
    }

    void updateSliderValsFromUI(
        const juce::Array<juce::Array<float>>& userMultiSliderValues,
        std::array<std::array<std::atomic<float>, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH>& sliderVals,
        std::atomic<int>& sliderVals_size,
        std::array<std::atomic<int>, MAXMULTISLIDER2DLENGTH>& sliderDepths)
    {
        // A. Calculate the new dimensions and transfer data to temporary structures.

        // 1. Determine the new overall size (N_used).
        const int new_sliderVals_size = std::min(userMultiSliderValues.size(), (int)MAXMULTISLIDER2DLENGTH);

        // 2. Use a temporary, non-atomic structure for safe staging.
        std::array<std::array<float, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH> temp_sliderVals = {};
        std::array<int, MAXMULTISLIDER2DLENGTH> temp_sliderDepths = {};

        // 3. Populate the temporary structures, respecting the max bounds.
        for (int i = 0; i < new_sliderVals_size; ++i)
        {
            const auto& innerArray = userMultiSliderValues.getUnchecked(i);

            // Determine the new depth (M_used(i)) for this row.
            const int new_depth = std::min(innerArray.size(), (int)MAXMULTISLIDER2DVALS);
            temp_sliderDepths[i] = new_depth;

            // Copy the float values.
            for (int j = 0; j < new_depth; ++j)
            {
                temp_sliderVals[i][j] = innerArray.getUnchecked(j);
            }
        }

        // B. Update the atomics (Critical Section).

        // 1. Update the actual slider values (`sliderVals`).
        // Iterate over the dimensions we actually need to update.
        for (int i = 0; i < new_sliderVals_size; ++i)
        {
            for (int j = 0; j < temp_sliderDepths[i]; ++j)
            {
                // Atomically update the float value.
                sliderVals[i][j].store(temp_sliderVals[i][j]);
            }
        }

        // 2. Update the depths (`sliderDepths`).
        // First, update the active depths.
        for (int i = 0; i < new_sliderVals_size; ++i)
        {
            sliderDepths[i].store(temp_sliderDepths[i]);
        }
        // Second, zero out any rows that were previously active but are now unused.
        for (int i = new_sliderVals_size; i < sliderVals_size.load(); ++i)
        {
            sliderDepths[i].store(0);
        }

        // 3. Update the overall size (`sliderVals_size`) LAST.
        // This is the "switch" that tells any processing threads the new data
        // and new dimensions are ready to be read.
        sliderVals_size.store(new_sliderVals_size);
    }

    void multiSliderValueChanged(juce::String name, int whichSlider, juce::Array<float> values) override
    {
        DBG("multiSliderValueChanged");
        //seems to not ever be called
    }

    void multiSliderAllValuesChanged(juce::String name, juce::Array<juce::Array<float>> values, juce::Array<bool> states) override
    {
        if (!mouseInteraction) return;

        juce::String valsStr;
        juce::String activeStr;

        if (isModulated_)
        {
            /*
             * we are manipulating the actual multislider: need to update param vals and defaultState vals
             */

            /*
             * first, update the parameter values from the slider values
             * - since these parameters are serialized and not handled internally like the simpler chowdsp params,
             *      we have to update them manually here
             */
            updateSliderValsFromUI(values, params->sliderVals, params->sliderVals_size, params->sliderDepths);

            int valCtr = 0;
            for (auto sval : states)
            {
                params->activeSliders[valCtr].store(states[valCtr++]);
            }
            params->activeVals_size.store (states.size()); // full array of slider states, including inactive ones (false)

            /*
             * then, create the strings for storing as defaultState properties
             *  and store
             */
            valsStr = sliderValsToString(params->sliderVals, params->sliderVals_size, params->sliderDepths);
            activeStr = arrayBoolToString(states);

            // then update defaultState, with string representation of the multislider arrays
            defaultState.setProperty( name_ + "_vals", valsStr, nullptr);
            defaultState.setProperty( name_ + "_size", values.size(), nullptr);
            defaultState.setProperty(name_ + "_states", activeStr, nullptr);
            defaultState.setProperty(name_ + "_states_size", states.size(), nullptr);
        }
        else if (isModulation_)
        {
            /*
             * since this means we are manipulating the clone of the multislider, we are setting mod values, not the values
             * themselves, so we don't write values to params->sliderVals, etc...
             *
             * we just write strings of the arrays to modulationState properties
             */

            // create some dummy param holders, so we can update them from the UI and then convert them to strings for property storage
            std::array<std::array<std::atomic<float>, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH> sliderVals_temp;
            std::atomic<int> sliderVals_size_temp;
            std::array<std::atomic<int>, MAXMULTISLIDER2DLENGTH> sliderDepths_temp;

            // update the dummy values from the actual modulator slider values
            updateSliderValsFromUI(values, sliderVals_temp, sliderVals_size_temp, sliderDepths_temp);

            // convert those values to strings
            valsStr = sliderValsToString(sliderVals_temp, sliderVals_size_temp, sliderDepths_temp);
            activeStr = arrayBoolToString(states);

            // store those strings to the appropriate properties
            modulationState.setProperty(name_ + "_vals", valsStr, nullptr);
            modulationState.setProperty(name_ + "_size", values.size(), nullptr);
            modulationState.setProperty(name_ +"_states", activeStr, nullptr);
            modulationState.setProperty(name_ + "_states_size", states.size(), nullptr);
        }
    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    /*
     * needed for the param-state modulation; is called when state mod popup is opened to set vals there
     */
    void syncToValueTree() override {

        // get the property strings for this mod
        auto msvals = modulationState.getProperty(name_ + "_vals");
        auto msvals_size = int(modulationState.getProperty(name_  + "_size"));
        auto msavals = modulationState.getProperty(name_ + "_states");
        auto msavals_size = int(modulationState.getProperty(name_ + "_states_size"));

        // make some dummy param holders, and fill them based on the mod strings above
        std::array<std::array<std::atomic<float>, MAXMULTISLIDER2DVALS>, MAXMULTISLIDER2DLENGTH> dispvals;
        std::atomic<int> sliderVals_size_temp;
        std::array<std::atomic<int>, MAXMULTISLIDER2DLENGTH> sliderDepths_temp;
        stringToSliderVals (msvals, dispvals, sliderVals_size_temp, sliderDepths_temp);

        std::array<std::atomic<bool>, MAXMULTISLIDER2DLENGTH> dispstates;
        stringToAtomicBoolArray(dispstates, msavals, false);

        // send to the multislider
        setToOnlyActive(
            getUIValuesFromSliderVals(dispvals, sliderVals_size_temp, sliderDepths_temp),
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

    /*
    * this is for making the modulation UI view opaque
    */
    void paint(juce::Graphics& g) override {
        if (isModulation_)
        {
            g.fillAll(juce::Colours::black); // choose your opaque BG
            BKMultiSlider::paint(g);
        }
    }

    juce::String name_;
    MultiSlider2DState *params;

private :
    /**
     * constructor for cloning UI in state-change mods
     */
    OpenGL_2DMultiSlider() : OpenGlAutoImageComponent<BKMultiSlider>()
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        image_component_->setUseAlpha(false);

        allowSubSliders = true; //2D

        isModulation_ = true;
        addMyListener(this);

        sliderBorder.setColour(juce::GroupComponent::outlineColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setColour(juce::GroupComponent::textColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setText ("MODIFIED");
        sliderBorder.setTextLabelPosition (juce::Justification::centred);
    }

    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER0_OPENGL_2DMULTISLIDER_H
