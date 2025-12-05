//
// Created by Myra Norton on 9/10/25.
//

#ifndef BITKLAVIER2_NOSTALGICPARAMETERSVIEW_H
#define BITKLAVIER2_NOSTALGICPARAMETERSVIEW_H
#include "NostalgicProcessor.h"
#include "OpenGL_VelocityMinMaxSlider.h"
#include "OpenGL_HoldTimeMinMaxSlider.h"
#include "OpenGL_WaveDistUndertowSlider.h"
#include "TranspositionSliderSection.h"
#include "VelocityMinMaxParams.h"
#include "EnvelopeSequenceState.h"
#include "EnvelopeSequenceSection.h"
#include "envelope_section.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"

class NostalgicParametersView : public SynthSection, public juce::Timer
{
public:
    NostalgicParametersView (chowdsp::PluginState& pluginState, NostalgicParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), nparams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("nostalgic");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        setSkinOverride(Skin::kDirect);

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // menus
        if (auto* nostalgicParams = dynamic_cast<NostalgicParams*>(&params)) {
            nostalgicTriggeredBy_combo_box = std::make_unique<OpenGLComboBox>(nostalgicParams->nostalgicTriggeredBy->paramID.toStdString());
            nostalgicTriggeredBy_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*nostalgicParams->nostalgicTriggeredBy.get(), listeners, *nostalgicTriggeredBy_combo_box, nullptr);
            // disable sync key up/down if synchronic isn't connected
            if (!nparams_.synchronicConnected)
            {
                nostalgicTriggeredBy_combo_box->setItemEnabled (2, false);
                nostalgicTriggeredBy_combo_box->setItemEnabled (3, false);
            }
            addAndMakeVisible(nostalgicTriggeredBy_combo_box.get());
            addOpenGlComponent(nostalgicTriggeredBy_combo_box->getImageComponent());
        }

        // transposition slider
        transpositionSlider     = std::make_unique<TranspositionSliderSection>(&params.transpose, listeners,name.toStdString());
        addSubSection (transpositionSlider.get());

        // wave distance undertow slider
        waveSlider = std::make_unique<OpenGL_WaveDistUndertowSlider>(&params.waveDistUndertowParams, listeners);
        addStateModulatedComponent (waveSlider.get());

        // key-on reset button
        keyOnReset = std::make_unique<SynthButton>(params.keyOnReset->paramID);
        keyOnReset_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.keyOnReset, listeners, *keyOnReset, nullptr);
        keyOnReset->setComponentID(params.keyOnReset->paramID);
        addSynthButton(keyOnReset.get(), true);
        keyOnReset->setText("key-on reset?");

        // knobs
        noteLengthMult_knob = std::make_unique<SynthSlider>(params.noteLengthMultParam->getName(20), params.noteLengthMultParam->getModParam());
        addSlider(noteLengthMult_knob.get());
        noteLengthMult_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        noteLengthMult_knob->setPopupPlacement(juce::BubbleComponent::below);
        noteLengthMult_knob->setShowPopupOnHover(true);
        noteLengthMult_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.noteLengthMultParam, listeners, *noteLengthMult_knob, nullptr);
        noteLengthMult_knob->addAttachment(noteLengthMult_knob_attachment.get());

        noteLengthMult_knob_label = std::make_shared<PlainTextComponent>(noteLengthMult_knob->getName(), noteLengthMult_knob->getName());
        addOpenGlComponent(noteLengthMult_knob_label);
        noteLengthMult_knob_label->setJustification(juce::Justification::centred);

        beatsToSkip_knob = std::make_unique<SynthSlider>(params.beatsToSkipParam->getName(20), params.beatsToSkipParam->getModParam());
        addSlider(beatsToSkip_knob.get());
        beatsToSkip_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        beatsToSkip_knob->setPopupPlacement(juce::BubbleComponent::below);
        beatsToSkip_knob->setShowPopupOnHover(true);
        beatsToSkip_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.beatsToSkipParam, listeners, *beatsToSkip_knob, nullptr);
        beatsToSkip_knob->addAttachment(beatsToSkip_knob_attachment.get());

        beatsToSkip_knob_label = std::make_shared<PlainTextComponent>(beatsToSkip_knob->getName(), beatsToSkip_knob->getName());
        addOpenGlComponent(beatsToSkip_knob_label);
        beatsToSkip_knob_label->setJustification(juce::Justification::centred);

        clusterMin_knob = std::make_unique<SynthSlider>(params.clusterMinParam->getName(20), params.clusterMinParam->getModParam());
        addSlider(clusterMin_knob.get());
        clusterMin_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterMin_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterMin_knob->setShowPopupOnHover(true);
        clusterMin_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterMinParam, listeners, *clusterMin_knob, nullptr);
        clusterMin_knob->addAttachment(clusterMin_knob_attachment.get());

        clusterMin_knob_label = std::make_shared<PlainTextComponent>(clusterMin_knob->getName(), clusterMin_knob->getName());
        addOpenGlComponent(clusterMin_knob_label);
        clusterMin_knob_label->setJustification(juce::Justification::centred);

        clusterThreshold_knob = std::make_unique<SynthSlider>(params.clusterThreshParam->getName(20), params.clusterThreshParam->getModParam());
        addSlider(clusterThreshold_knob.get());
        clusterThreshold_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterThreshold_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterThreshold_knob->setShowPopupOnHover(true);
        clusterThreshold_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterThreshParam, listeners, *clusterThreshold_knob, nullptr);
        clusterThreshold_knob->addAttachment(clusterThreshold_knob_attachment.get());

        clusterThreshold_knob_label = std::make_shared<PlainTextComponent>(clusterThreshold_knob->getName(), clusterThreshold_knob->getName());
        addOpenGlComponent(clusterThreshold_knob_label);
        clusterThreshold_knob_label->setJustification(juce::Justification::centred);

        variousControlsBorder = std::make_shared<OpenGL_LabeledBorder>("various controls border", "Various Parameters");
        addBorder(variousControlsBorder.get());

        // hold time min/max slider
        holdTimeMinMaxSlider = std::make_unique<OpenGL_HoldTimeMinMaxSlider>(&params.holdTimeMinMaxParams, listeners);
        holdTimeMinMaxSlider->setComponentID ("holdtime_min_max");
        addStateModulatedComponent (holdTimeMinMaxSlider.get());

        // envelope/ADSR controller UI
        reverseEnvSection = std::make_unique<EnvelopeSection>(params.reverseEnv, listeners, *this);
        reverseEnvSection->setName(params.reverseEnv.label);
        addSubSection (reverseEnvSection.get());
        undertowEnvSection = std::make_unique<EnvelopeSection>(params.undertowEnv, listeners, *this);
        undertowEnvSection->setName(params.undertowEnv.label);
        addSubSection (undertowEnvSection.get());

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Main");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSendGain, listeners, &params.sendLevels);
        sendLevelMeter->setLabel("Send");
        addSubSection(sendLevelMeter.get());

        // draw note length multiplier or beats to skip depending on selected option from combo box
        showAppropriateKnobs();

        // this draws the correct parameter as soon as a new option is selected
        nostalgicTriggerByCallback = {listeners.addParameterListener (params.nostalgicTriggeredBy,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                auto* interface = findParentComponentOfClass<SynthGuiInterface>();
                juce::ScopedLock{*interface->getOpenGlCriticalSection()};
                showAppropriateKnobs();
                auto* prep_popup = findParentComponentOfClass<PreparationPopup>();
                prep_popup->repaintPrepBackground();
            })};

        // for updating the wavedistundertow display sliders
        startTimer(50);
    }

    void timerCallback() override;

    void showAppropriateKnobs()
    {
        if (nparams_.nostalgicTriggeredBy->get() != NostalgicComboBox::Note_Length)
        {
            beatsToSkip_knob->setVisible(true);
            beatsToSkip_knob_label->setVisible(true);
            noteLengthMult_knob->setVisible(false);
            noteLengthMult_knob_label->setVisible(false);
        }
        else
        {
            noteLengthMult_knob->setVisible(true);
            noteLengthMult_knob_label->setVisible(true);
            beatsToSkip_knob->setVisible(false);
            beatsToSkip_knob_label->setVisible(false);
        }
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    std::shared_ptr<PlainTextComponent> prepTitle;

    chowdsp::ScopedCallbackList sliderChangedCallback;

    // complex UI elements in this prep
    std::unique_ptr<TranspositionSliderSection> transpositionSlider;
    std::unique_ptr<OpenGL_WaveDistUndertowSlider> waveSlider;

    // "use tuning" toggle
    std::unique_ptr<SynthButton> useTuning;
    std::unique_ptr<chowdsp::ButtonAttachment> useTuning_attachment;

    // key-on reset toggle
    std::unique_ptr<SynthButton> keyOnReset;
    std::unique_ptr<chowdsp::ButtonAttachment> keyOnReset_attachment;

    // combo box menus
    std::unique_ptr<OpenGLComboBox> nostalgicTriggeredBy_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> nostalgicTriggeredBy_attachment;

    // range slider
    std::unique_ptr<OpenGL_HoldTimeMinMaxSlider> holdTimeMinMaxSlider;

    // knobs
    std::unique_ptr<SynthSlider> noteLengthMult_knob;
    std::unique_ptr<chowdsp::SliderAttachment> noteLengthMult_knob_attachment;
    std::unique_ptr<SynthSlider> beatsToSkip_knob;
    std::unique_ptr<chowdsp::SliderAttachment> beatsToSkip_knob_attachment;
    std::unique_ptr<SynthSlider> clusterMin_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterMin_knob_attachment;
    std::unique_ptr<SynthSlider> clusterThreshold_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThreshold_knob_attachment;

    // border for knobs and combobox and reset toggle
    std::shared_ptr<OpenGL_LabeledBorder> variousControlsBorder;

    // knob labels
    std::shared_ptr<PlainTextComponent> noteLengthMult_knob_label;
    std::shared_ptr<PlainTextComponent> beatsToSkip_knob_label;
    std::shared_ptr<PlainTextComponent> clusterMin_knob_label;
    std::shared_ptr<PlainTextComponent> clusterThreshold_knob_label;

    // ADSR controller: for setting the parameters of each ADSR
    std::unique_ptr<EnvelopeSection> reverseEnvSection;
    std::unique_ptr<EnvelopeSection> undertowEnvSection;

    // ADSRs: for turning on/off particular ADSRs, and for choosing particular ones to edit with envSection
    std::unique_ptr<EnvelopeSequenceSection> reverseEnvSequenceSection;
    std::unique_ptr<EnvelopeSequenceSection> undertowEnvSequenceSection;

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    NostalgicParams& nparams_;

    chowdsp::ScopedCallback nostalgicTriggerByCallback;
    void resized() override;
};

#endif //BITKLAVIER2_NOSTALGICPARAMETERSVIEW_H
