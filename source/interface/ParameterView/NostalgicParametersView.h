//
// Created by Myra Norton on 9/10/25.
//

#ifndef BITKLAVIER2_NOSTALGICPARAMETERSVIEW_H
#define BITKLAVIER2_NOSTALGICPARAMETERSVIEW_H
#include "NostalgicProcessor.h"
#include "OpenGL_VelocityMinMaxSlider.h"
#include "OpenGL_HoldTimeMinMaxSlider.h"
#include "TranspositionSliderSection.h"
#include "VelocityMinMaxParams.h"
#include "EnvelopeSequenceState.h"
#include "EnvelopeSequenceSection.h"
#include "envelope_section.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"

class NostalgicParametersView : public SynthSection
{
public:
    NostalgicParametersView (chowdsp::PluginState& pluginState, NostalgicParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection ("")
    {
        // the name that will appear in the UI as the name of the section
        setName ("nostalgic");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // transposition slider
        transpositionSlider     = std::make_unique<TranspositionSliderSection>(&params.transpose, listeners,name.toStdString());
        addSubSection (transpositionSlider.get());

        // knobs
        noteLengthMult_knob = std::make_unique<SynthSlider>(params.noteLengthMultParam->paramID);
        addSlider(noteLengthMult_knob.get());
        noteLengthMult_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        noteLengthMult_knob->setPopupPlacement(juce::BubbleComponent::below);
        noteLengthMult_knob->setShowPopupOnHover(true);
        noteLengthMult_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.noteLengthMultParam, listeners, *noteLengthMult_knob, nullptr);
        noteLengthMult_knob->addAttachment(noteLengthMult_knob_attachment.get());

        clusterMin_knob = std::make_unique<SynthSlider>(params.clusterMinParam->paramID);
        addSlider(clusterMin_knob.get());
        clusterMin_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterMin_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterMin_knob->setShowPopupOnHover(true);
        clusterMin_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterMinParam, listeners, *clusterMin_knob, nullptr);
        clusterMin_knob->addAttachment(clusterMin_knob_attachment.get());

        clusterThreshold_knob = std::make_unique<SynthSlider>(params.clusterThreshParam->paramID);
        addSlider(clusterThreshold_knob.get());
        clusterThreshold_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterThreshold_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterThreshold_knob->setShowPopupOnHover(true);
        clusterThreshold_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterThreshParam, listeners, *clusterThreshold_knob, nullptr);
        clusterThreshold_knob->addAttachment(clusterThreshold_knob_attachment.get());

        // hold time min/max slider
        holdTimeMinMaxSlider = std::make_unique<OpenGL_HoldTimeMinMaxSlider>(&params.holdTimeMinMaxParams, listeners);
        holdTimeMinMaxSlider->setComponentID ("holdtime_min_max");
        addStateModulatedComponent (holdTimeMinMaxSlider.get());

        // envelope/ADSR controller UI
        reverseEnvSection = std::make_unique<EnvelopeSection>(params.reverseEnv, listeners, *this);
        addSubSection (reverseEnvSection.get());
        undertowEnvSection = std::make_unique<EnvelopeSection>(params.undertowEnv, listeners, *this);
        addSubSection (undertowEnvSection.get());

        // sequence of ADSRs
        // envSequenceSection = std::make_unique<EnvelopeSequenceSection>(name, params.reverseEnvSequence, listeners, *this);
        // addSubSection(envSequenceSection.get());

        // // toggles
        // useTuning = std::make_unique<SynthButton>(params.transpositionUsesTuning->paramID);
        // useTuning_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.transpositionUsesTuning, listeners, *useTuning, nullptr);
        // useTuning->setComponentID(params.transpositionUsesTuning->paramID);
        // addSynthButton(useTuning.get(), true);
        // useTuning->setText("use Tuning?");

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

        drawLabelForComponent(g, TRANS("note length multiplier"), noteLengthMult_knob.get());
        drawLabelForComponent(g, TRANS("cluster min"), clusterMin_knob.get());
        drawLabelForComponent(g, TRANS("cluster threshold"), clusterThreshold_knob.get());

        paintChildrenBackgrounds (g);
    }


    chowdsp::ScopedCallbackList sliderChangedCallback;

    // complex UI elements in this prep
    std::unique_ptr<TranspositionSliderSection> transpositionSlider;

    // "use tuning" toggle
    std::unique_ptr<SynthButton> useTuning;
    std::unique_ptr<chowdsp::ButtonAttachment> useTuning_attachment;

    // range slider
    std::unique_ptr<OpenGL_HoldTimeMinMaxSlider> holdTimeMinMaxSlider;

    // knobs
    std::unique_ptr<SynthSlider> noteLengthMult_knob;
    std::unique_ptr<chowdsp::SliderAttachment> noteLengthMult_knob_attachment;
    std::unique_ptr<SynthSlider> clusterMin_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterMin_knob_attachment;
    std::unique_ptr<SynthSlider> clusterThreshold_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThreshold_knob_attachment;

    // ADSR controller: for setting the parameters of each ADSR
    std::unique_ptr<EnvelopeSection> reverseEnvSection;
    std::unique_ptr<EnvelopeSection> undertowEnvSection;

    // ADSRs: for turning on/off particular ADSRs, and for choosing particular ones to edit with envSection
    std::unique_ptr<EnvelopeSequenceSection> reverseEnvSequenceSection;
    std::unique_ptr<EnvelopeSequenceSection> undertowEnvSequenceSection;


    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    void resized() override;
};

#endif //BITKLAVIER2_NOSTALGICPARAMETERSVIEW_H
