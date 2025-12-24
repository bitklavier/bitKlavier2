//
// Created by Dan Trueman on 8/5/25.
//

#ifndef BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H
#define BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H

#include "OpenGL_ClusterMinMaxSlider.h"
#include "OpenGL_HoldTimeMinMaxSlider.h"
#include "OpenGL_MultiSlider.h"
#include "OpenGL_2DMultiSlider.h"
#include "SynchronicProcessor.h"
#include "EnvelopeSequenceState.h"
#include "EnvelopeSequenceSection.h"
#include "peak_meter_section.h"
#include "envelope_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"

/**
 * todo:
 *  - when Key Off "determines cluster", grey out Note-On options in "pulse triggered by"
 *  - grey out hold min/max slider when it's not relevant (most of the note-on options)
 */

class SynchronicParametersView : public SynthSection, public juce::Timer
{
public:
    SynchronicParametersView (chowdsp::PluginState& pluginState, SynchronicParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), sparams_(params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("synchronic");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        setSkinOverride(Skin::kDirect);
        //setSkinOverride(Skin::kSynchronic);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // menus
        if (auto* synchronicParams = dynamic_cast<SynchronicParams*>(&params)) {
            pulseTriggeredBy_combo_box = std::make_unique<OpenGLComboBox>(synchronicParams->pulseTriggeredBy->paramID.toStdString());
            pulseTriggeredBy_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*synchronicParams->pulseTriggeredBy.get(), listeners, *pulseTriggeredBy_combo_box, nullptr);
            addAndMakeVisible(pulseTriggeredBy_combo_box.get());
            addOpenGlComponent(pulseTriggeredBy_combo_box->getImageComponent());

            determinesCluster_combo_box = std::make_unique<OpenGLComboBox>(synchronicParams->determinesCluster->paramID.toStdString());
            determinesCluster_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*synchronicParams->determinesCluster.get(), listeners, *determinesCluster_combo_box, nullptr);
            addAndMakeVisible(determinesCluster_combo_box.get());
            addOpenGlComponent(determinesCluster_combo_box->getImageComponent());
        }

        // menu labels
        pulseTriggeredBy_label = std::make_shared<PlainTextComponent>("ptb", "trigger:");
        addOpenGlComponent(pulseTriggeredBy_label);
        pulseTriggeredBy_label->setJustification(juce::Justification::right);

        determinesCluster_label = std::make_shared<PlainTextComponent>("dtl", "cluster:");
        addOpenGlComponent(determinesCluster_label);
        determinesCluster_label->setJustification(juce::Justification::right);

        // knobs
        numPulses_knob = std::make_unique<SynthSlider>(params.numPulses->paramID, params.numPulses->getModParam());
        addSlider(numPulses_knob.get());
        numPulses_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        numPulses_knob->setPopupPlacement(juce::BubbleComponent::below);
        numPulses_knob->setShowPopupOnHover(true);
        numPulses_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.numPulses, listeners, *numPulses_knob, nullptr);
        numPulses_knob->addAttachment(numPulses_knob_attachment.get()); // needed for mods!!

        numLayers_knob = std::make_unique<SynthSlider>(params.numLayers->paramID,params.numLayers->getModParam());
        addSlider(numLayers_knob.get());
        numLayers_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        numLayers_knob->setPopupPlacement(juce::BubbleComponent::below);
        numLayers_knob->setShowPopupOnHover(true);
        numLayers_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.numLayers, listeners, *numLayers_knob, nullptr);
        numLayers_knob->addAttachment(numLayers_knob_attachment.get());

        clusterThickness_knob = std::make_unique<SynthSlider>(params.clusterThickness->paramID,params.clusterThickness->getModParam());
        addSlider(clusterThickness_knob.get());
        clusterThickness_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterThickness_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterThickness_knob->setShowPopupOnHover(true);
        clusterThickness_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterThickness, listeners, *clusterThickness_knob, nullptr);
        clusterThickness_knob->addAttachment(clusterThickness_knob_attachment.get());

        clusterThreshold_knob = std::make_unique<SynthSlider>(params.clusterThreshold->paramID,params.clusterThreshold->getModParam());
        addSlider(clusterThreshold_knob.get());
        clusterThreshold_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterThreshold_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterThreshold_knob->setShowPopupOnHover(true);
        clusterThreshold_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterThreshold, listeners, *clusterThreshold_knob, nullptr);
        clusterThreshold_knob->addAttachment(clusterThreshold_knob_attachment.get());

        numPulses_knob_label = std::make_shared<PlainTextComponent>(numPulses_knob->getName(), params.numPulses->getName(20));
        addOpenGlComponent(numPulses_knob_label);
        numPulses_knob_label->setJustification(juce::Justification::centred);

        numLayers_knob_label = std::make_shared<PlainTextComponent>(numLayers_knob->getName(), params.numLayers->getName(20));
        addOpenGlComponent(numLayers_knob_label);
        numLayers_knob_label->setJustification(juce::Justification::centred);

        clusterThickness_knob_label = std::make_shared<PlainTextComponent>(clusterThickness_knob->getName(), params.clusterThickness->getName(20));
        addOpenGlComponent(clusterThickness_knob_label);
        clusterThickness_knob_label->setJustification(juce::Justification::centred);

        clusterThreshold_knob_label = std::make_shared<PlainTextComponent>(clusterThreshold_knob->getName(), params.clusterThreshold->getName(20));
        addOpenGlComponent(clusterThreshold_knob_label);
        clusterThreshold_knob_label->setJustification(juce::Justification::centred);

        variousControlsBorder = std::make_shared<OpenGL_LabeledBorder>("various controls border", "General Stuff");
        addBorder(variousControlsBorder.get());

        // multisliders
        transpositionsSlider = std::make_unique<OpenGL_2DMultiSlider>("transpositions", &params.transpositions, listeners);
        transpositionsSlider->setComponentID ("transpositions");
        transpositionsSlider->setMinMaxDefaultInc({-12., 12, 0., 0.001});
        transpositionsSlider->setName("Transpositions");
        addStateModulatedComponent (transpositionsSlider.get());
        transpositionsSlider->updateFromParams(juce::dontSendNotification);

        accentsSlider = std::make_unique<OpenGL_MultiSlider>("accents", &params.accents, listeners);
        accentsSlider->setComponentID ("accents");
        accentsSlider->setMinMaxDefaultInc({0., 2, 1., 0.1});
        accentsSlider->setName("Accents");
        addStateModulatedComponent (accentsSlider.get());
        accentsSlider->updateFromParams(juce::dontSendNotification);

        sustainLengthMultipliersSlider = std::make_unique<OpenGL_MultiSlider>("sustainlength_multipliers", &params.sustainLengthMultipliers, listeners);
        sustainLengthMultipliersSlider->setComponentID ("sustainlength_multipliers");
        sustainLengthMultipliersSlider->setMinMaxDefaultInc({-2., 2., 1., 0.01});
        sustainLengthMultipliersSlider->setName("Sustain Length Multipliers");
        addStateModulatedComponent (sustainLengthMultipliersSlider.get());
        sustainLengthMultipliersSlider->updateFromParams(juce::dontSendNotification);

        beatLengthMultipliersSlider = std::make_unique<OpenGL_MultiSlider>("beatlength_multipliers", &params.beatLengthMultipliers, listeners);
        beatLengthMultipliersSlider->setComponentID ("beatlength_multipliers");
        beatLengthMultipliersSlider->setMinMaxDefaultInc({0., 2., 1., 0.01});
        beatLengthMultipliersSlider->setName("Beat Length Multipliers");
        addStateModulatedComponent (beatLengthMultipliersSlider.get());
        beatLengthMultipliersSlider->updateFromParams(juce::dontSendNotification);

        // min/max sliders
        clusterMinMaxSlider = std::make_unique<OpenGL_ClusterMinMaxSlider>(&params.clusterMinMaxParams, listeners);
        clusterMinMaxSlider->setComponentID ("cluster_min_max");
        addStateModulatedComponent (clusterMinMaxSlider.get());

        holdTimeMinMaxSlider = std::make_unique<OpenGL_HoldTimeMinMaxSlider>(&params.holdTimeMinMaxParams, listeners);
        holdTimeMinMaxSlider->setComponentID ("holdtime_min_max");
        addStateModulatedComponent (holdTimeMinMaxSlider.get());

        // envelope/ADSR controller UI
        envSection = std::make_unique<EnvelopeSection>(params.env, listeners, *this);
        addSubSection (envSection.get());

        // sequence of ADSRs
        envSequenceSection = std::make_unique<EnvelopeSequenceSection>(name, params.envelopeSequence, listeners, *this);
        addSubSection(envSequenceSection.get());

        // toggles
        useTuning = std::make_unique<SynthButton>(params.transpositionUsesTuning->paramID);
        useTuning_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.transpositionUsesTuning, listeners, *useTuning, nullptr);
        useTuning->setComponentID(params.transpositionUsesTuning->paramID);
        addSynthButton(useTuning.get(), true);
        useTuning->setText("use Tuning?");

        skipFirst = std::make_unique<SynthButton>(params.skipFirst->paramID);
        skipFirst_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.skipFirst, listeners, *skipFirst, nullptr);
        skipFirst->setComponentID(params.skipFirst->paramID);
        addSynthButton(skipFirst.get(), true);
        skipFirst->setText("skip first?");

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Main");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSendGain, listeners, &params.sendLevels);
        sendLevelMeter->setLabel("Send");
        addSubSection(sendLevelMeter.get());

        /*
         * listen for changes from mods/resets, redraw as needed
         */
        sliderChangedCallback += {
            listeners.addParameterListener
            (
                params.updateUIState, // this value will be changed whenever a mod or reset is called
                chowdsp::ParameterListenerThread::MessageThread,
                [this]
                {
                    transpositionsSlider->updateFromParams();
                    accentsSlider->updateFromParams();
                    sustainLengthMultipliersSlider->updateFromParams();
                    beatLengthMultipliersSlider->updateFromParams();
                }
            )
        };

        // this will get called when a different envelope of the 12 is selected for editing (bottom row of buttons)
        sliderChangedCallback += {
            listeners.addParameterListener
            (
                params.envelopeSequence.currentlyEditing,
                chowdsp::ParameterListenerThread::MessageThread,
                [this]
                {
                    displayEnvSequenceValsInEnvEditor();
                }
            )
        };

        // this will get called when any of the params in the envelope editor Env are changed
        // triggered on mouseUp in the UI
        sliderChangedCallback += {
            listeners.addParameterListener
            (
                params.env.notify,
                chowdsp::ParameterListenerThread::MessageThread,
                [this]
                {
                    copyEnvEditorValsToEnvSequence();
                }
            )
        };

        // draw them...
        transpositionsSlider->drawSliders(juce::dontSendNotification);
        accentsSlider->drawSliders(juce::dontSendNotification);
        sustainLengthMultipliersSlider->drawSliders(juce::dontSendNotification);
        beatLengthMultipliersSlider->drawSliders(juce::dontSendNotification);

        // update UI for envelope editor
        displayEnvSequenceValsInEnvEditor();

        // for updating the current sliders in multisliders
        startTimer(50);

    }

    void timerCallback() override;

    void copyEnvEditorValsToEnvSequence()
    {
        int currentEnv = *sparams_.envelopeSequence.currentlyEditing;
        sparams_.envelopeSequence.envStates.attacks[currentEnv].store(*sparams_.env.attackParam);
        sparams_.envelopeSequence.envStates.decays[currentEnv].store(*sparams_.env.decayParam);
        sparams_.envelopeSequence.envStates.sustains[currentEnv].store(*sparams_.env.sustainParam);
        sparams_.envelopeSequence.envStates.releases[currentEnv].store(*sparams_.env.releaseParam);

        sparams_.envelopeSequence.envStates.attackPowers[currentEnv].store(*sparams_.env.attackPowerParam);
        sparams_.envelopeSequence.envStates.decayPowers[currentEnv].store(*sparams_.env.decayPowerParam);
        sparams_.envelopeSequence.envStates.releasePowers[currentEnv].store(*sparams_.env.releasePowerParam);
    }

    void displayEnvSequenceValsInEnvEditor()
    {
        int currentEnv = *sparams_.envelopeSequence.currentlyEditing;
        envSection->setADSRVals(
            sparams_.envelopeSequence.envStates.attacks[currentEnv],
            sparams_.envelopeSequence.envStates.decays[currentEnv],
            sparams_.envelopeSequence.envStates.sustains[currentEnv],
            sparams_.envelopeSequence.envStates.releases[currentEnv],
            sparams_.envelopeSequence.envStates.attackPowers[currentEnv],
            sparams_.envelopeSequence.envStates.decayPowers[currentEnv],
            sparams_.envelopeSequence.envStates.releasePowers[currentEnv]
        );
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;

    std::shared_ptr<PlainTextComponent> prepTitle;

    // combo box menus
    std::unique_ptr<OpenGLComboBox> pulseTriggeredBy_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> pulseTriggeredBy_attachment;
    std::unique_ptr<OpenGLComboBox> determinesCluster_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> determinesCluster_attachment;
    std::shared_ptr<PlainTextComponent> pulseTriggeredBy_label;
    std::shared_ptr<PlainTextComponent> determinesCluster_label;

    // multisliders
    std::unique_ptr<OpenGL_2DMultiSlider> transpositionsSlider;
    std::unique_ptr<OpenGL_MultiSlider> accentsSlider;
    std::unique_ptr<OpenGL_MultiSlider> sustainLengthMultipliersSlider;
    std::unique_ptr<OpenGL_MultiSlider> beatLengthMultipliersSlider;

    // "use tuning" and "skip first" toggles
    std::unique_ptr<SynthButton> useTuning;
    std::unique_ptr<chowdsp::ButtonAttachment> useTuning_attachment;
    std::unique_ptr<SynthButton> skipFirst;
    std::unique_ptr<chowdsp::ButtonAttachment> skipFirst_attachment;

    // range sliders
    std::unique_ptr<OpenGL_ClusterMinMaxSlider> clusterMinMaxSlider;
    std::unique_ptr<OpenGL_HoldTimeMinMaxSlider> holdTimeMinMaxSlider;

    // knobs
    std::unique_ptr<SynthSlider> numPulses_knob;
    std::unique_ptr<chowdsp::SliderAttachment> numPulses_knob_attachment;
    std::unique_ptr<SynthSlider> numLayers_knob;
    std::unique_ptr<chowdsp::SliderAttachment> numLayers_knob_attachment;
    std::unique_ptr<SynthSlider> clusterThickness_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThickness_knob_attachment;
    std::unique_ptr<SynthSlider> clusterThreshold_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThreshold_knob_attachment;

    // knob labels
    std::shared_ptr<PlainTextComponent> numPulses_knob_label;
    std::shared_ptr<PlainTextComponent> numLayers_knob_label;
    std::shared_ptr<PlainTextComponent> clusterThickness_knob_label;
    std::shared_ptr<PlainTextComponent> clusterThreshold_knob_label;

    // for the mixer knobs
    std::shared_ptr<OpenGL_LabeledBorder> variousControlsBorder;

    // ADSR controller: for setting the parameters of each ADSR
    std::unique_ptr<EnvelopeSection> envSection;

    // ADSRs: for turning on/off particular ADSRs, and for choosing particular ones to edit with envSection
    std::unique_ptr<EnvelopeSequenceSection> envSequenceSection;

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    SynchronicParams& sparams_;

    void resized() override;
    void stopAllTimers()override {
        stopTimer();
    }
};

#endif //BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H
