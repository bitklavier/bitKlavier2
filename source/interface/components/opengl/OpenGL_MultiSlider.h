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

        updateFromParams(juce::dontSendNotification);
    }

    /*
     * updates the sliders from the current parameter values: useful after a mod or reset
     */
    void updateFromParams(juce::NotificationType notify = juce::sendNotification)
    {
        setToOnlyActive(
            atomicArrayToJuceArrayLimited(params->sliderVals, params->sliderVals_size),
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

    OpenGL_MultiSlider* clone() {
        // auto msvals = modulationState.getProperty(name_ + "_vals");
        // auto msvals_size = int(modulationState.getProperty(name_  + "_size"));
        // auto msavals = modulationState.getProperty(name_ + "_states");
        // auto msavals_size = int(modulationState.getProperty(name_ + "_states_size"));
        // defaultState.setProperty( name_ + "_vals", msvals, nullptr);
        // defaultState.setProperty( name_ + "_size", msvals_size, nullptr);
        // defaultState.setProperty(name_ + "_states", msavals, nullptr);
        // defaultState.setProperty(name_ + "_states_size", msavals_size, nullptr);
        auto sliderClone = new OpenGL_MultiSlider();
        sliderClone->setMinMaxDefaultInc (getMinMaxDefaultInc());
        sliderClone->name_ = name_;
        // sliderClone->setName(name_);
        return sliderClone;;
    }

    void multiSliderValueChanged(juce::String name, int whichSlider, juce::Array<float> values) override
    {
        DBG("multiSliderValueChanged");
        //seems to not ever be called
    }

    void multiSliderAllValuesChanged(juce::String name, juce::Array<juce::Array<float>> values, juce::Array<bool> states) override
    {
        if (!mouseInteraction) return;

        /*
         * create string representations of the multislider vals/states
         */
        juce::String valsStr = getFirstValueFromSubarrays(values);
        juce::String activeStr = arrayBoolToString(states);

        if (isModulated_)
        {
            /*
             * we are manipulating the actual multislider: need to update param vals and defaultState vals
             */

            // just copy the direct UI representations to the param arrays; the prep should know how to use them
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

            DBG("resetting defaultState in Multislider");
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

        auto msvals = modulationState.getProperty(name_ + "_vals");
        auto msvals_size = int(modulationState.getProperty(name_  + "_size"));
        auto msavals = modulationState.getProperty(name_ + "_states");
        auto msavals_size = int(modulationState.getProperty(name_ + "_states_size"));

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
        image_component_->setUseAlpha(false);

        isModulation_ = true;
        addMyListener(this);

        /*
         * this is so the modulation UI view has a distinctive colored border
         */
        sliderBorder.setColour(juce::GroupComponent::outlineColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setColour(juce::GroupComponent::textColourId, findColour (Skin::kRotaryArc));
        sliderBorder.setText ("MODIFIED");
        sliderBorder.setTextLabelPosition (juce::Justification::centred);
    }

    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //BITKLAVIER0_OPENGL_MULTISLIDER_H
