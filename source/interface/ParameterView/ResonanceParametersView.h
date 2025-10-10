//
// Created by Dan Trueman on 10/8/25.
//

#ifndef BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
#define BITKLAVIER0_RESONANCEPARAMETERSVIEW_H

#include "ResonanceProcessor.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"
#include "OpenGL_AbsoluteKeyboardSlider.h"
#include "OpenGL_KeymapKeyboard.h"

class ResonanceParametersView : public SynthSection
{
public:
    ResonanceParametersView (chowdsp::PluginState& pluginState, ResonanceParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), sparams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("resonance");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

//        fundamentalKeyboard = std::make_unique<OpenGLKeymapKeyboardComponent>(params);
//        addStateModulatedComponent(fundamentalKeyboard.get());
//        addAndMakeVisible(fundamentalKeyboard.get());

        offsetsKeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<ResonanceParams*>(&params)->offsetsKeyboardState);
        addStateModulatedComponent(offsetsKeyboard.get());
        offsetsKeyboard->setName("offsets");
        offsetsKeyboard->setAvailableRange(0, numKeys);
        offsetsKeyboard->setOctaveForMiddleC(5);

        gainsKeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<ResonanceParams*>(&params)->gainsKeyboardState);
        addStateModulatedComponent(gainsKeyboard.get());
        gainsKeyboard->setName("gains");
        gainsKeyboard->setAvailableRange(0, numKeys);
        gainsKeyboard->setMinMidMaxValues(0.1, 1., 10., 2); // min, mid, max, display resolution
        gainsKeyboard->setOctaveForMiddleC(5);

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Main");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSendGain, listeners, &params.sendLevels);
        sendLevelMeter->setLabel("Send");
        addSubSection(sendLevelMeter.get());
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);

//        drawLabelForComponent(g, TRANS("pulses"), numPulses_knob.get());
//        drawLabelForComponent(g, TRANS("layers"), numLayers_knob.get());
//        drawLabelForComponent(g, TRANS("cluster thickness"), clusterThickness_knob.get());
//        drawLabelForComponent(g, TRANS("cluster threshold"), clusterThreshold_knob.get());

        paintChildrenBackgrounds (g);
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;

    std::unique_ptr<OpenGLKeymapKeyboardComponent> fundamentalKeyboard;
    std::unique_ptr<OpenGLKeymapKeyboardComponent> closestKeyboard;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> offsetsKeyboard;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> gainsKeyboard;
    int numKeys = 52;

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    ResonanceParams& sparams_;

    void resized() override;
};

#endif //BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
