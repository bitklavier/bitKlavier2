//
// Created by Dan Trueman on 8/5/25.
//

#ifndef BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H
#define BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H

#include "OpenGL_ClusterMinMaxSlider.h"
#include "OpenGL_HoldTimeMinMaxSlider.h"
#include "OpenGL_MultiSlider.h"
#include "SynchronicProcessor.h"
#include "peak_meter_section.h"
#include "envelope_section.h"
#include "synth_section.h"
#include "synth_slider.h"

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

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

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

        pulseTriggeredBy_label = std::make_shared<PlainTextComponent>("ptb", "pulse triggered by:");
        addOpenGlComponent(pulseTriggeredBy_label);
        pulseTriggeredBy_label->setTextSize (12.0f);
        pulseTriggeredBy_label->setJustification(juce::Justification::right);

        determinesCluster_label = std::make_shared<PlainTextComponent>("dtl", "determines cluster:");
        addOpenGlComponent(determinesCluster_label);
        determinesCluster_label->setTextSize (12.0f);
        determinesCluster_label->setJustification(juce::Justification::right);

        numPulses_knob = std::make_unique<SynthSlider>(params.numPulses->paramID);
        addSlider(numPulses_knob.get());
        numPulses_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        numPulses_knob->setPopupPlacement(juce::BubbleComponent::below);
        numPulses_knob->setShowPopupOnHover(true);
        numPulses_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.numPulses, listeners, *numPulses_knob, nullptr);

        numLayers_knob = std::make_unique<SynthSlider>(params.numLayers->paramID);
        addSlider(numLayers_knob.get());
        numLayers_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        numLayers_knob->setPopupPlacement(juce::BubbleComponent::below);
        numLayers_knob->setShowPopupOnHover(true);
        numLayers_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.numLayers, listeners, *numLayers_knob, nullptr);

        clusterThickness_knob = std::make_unique<SynthSlider>(params.clusterThickness->paramID);
        addSlider(clusterThickness_knob.get());
        clusterThickness_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterThickness_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterThickness_knob->setShowPopupOnHover(true);
        clusterThickness_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterThickness, listeners, *clusterThickness_knob, nullptr);

        clusterThreshold_knob = std::make_unique<SynthSlider>(params.clusterThreshold->paramID);
        addSlider(clusterThreshold_knob.get());
        clusterThreshold_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        clusterThreshold_knob->setPopupPlacement(juce::BubbleComponent::below);
        clusterThreshold_knob->setShowPopupOnHover(true);
        clusterThreshold_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.clusterThreshold, listeners, *clusterThreshold_knob, nullptr);

        transpositionsSlider = std::make_unique<OpenGL_MultiSlider>("transpositions_", &params.transpositions, listeners);
        transpositionsSlider->setComponentID ("transpositions_");
        transpositionsSlider->setMinMaxDefaultInc({-12., 12, 0., 0.001});
        transpositionsSlider->setName("Transpositions");
        addStateModulatedComponent (transpositionsSlider.get());

        accentsSlider = std::make_unique<OpenGL_MultiSlider>("accents_", &params.accents, listeners);
        accentsSlider->setComponentID ("accents_");
        accentsSlider->setMinMaxDefaultInc({0., 2, 1., 0.1});
        accentsSlider->setName("Accents");
        addStateModulatedComponent (accentsSlider.get());

        sustainLengthMultipliersSlider = std::make_unique<OpenGL_MultiSlider>("sustainlength_multipliers", &params.sustainLengthMultipliers, listeners);
        sustainLengthMultipliersSlider->setComponentID ("sustainlength_multipliers");
        sustainLengthMultipliersSlider->setMinMaxDefaultInc({-2., 2., 1., 0.01});
        sustainLengthMultipliersSlider->setName("Sustain Length Multipliers");
        addStateModulatedComponent (sustainLengthMultipliersSlider.get());

        beatLengthMultipliersSlider = std::make_unique<OpenGL_MultiSlider>("beatlength_multipliers", &params.beatLengthMultipliers, listeners);
        beatLengthMultipliersSlider->setComponentID ("beatlength_multipliers");
        beatLengthMultipliersSlider->setMinMaxDefaultInc({0., 2., 1., 0.01});
        beatLengthMultipliersSlider->setName("Beat Length Multipliers");
        addStateModulatedComponent (beatLengthMultipliersSlider.get());

        clusterMinMaxSlider = std::make_unique<OpenGL_ClusterMinMaxSlider>(&params.clusterMinMaxParams, listeners);
        clusterMinMaxSlider->setComponentID ("cluster_min_max");
        addStateModulatedComponent (clusterMinMaxSlider.get());

        holdTimeMinMaxSlider = std::make_unique<OpenGL_HoldTimeMinMaxSlider>(&params.holdTimeMinMaxParams, listeners);
        holdTimeMinMaxSlider->setComponentID ("holdtime_min_max");
        addStateModulatedComponent (holdTimeMinMaxSlider.get());

        envSection = std::make_unique<EnvelopeSection>( params.env ,listeners, *this);
        addSubSection (envSection.get());

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

        /*
         * not sure why we need to redo this here, but they don't draw without these calls
         */
        transpositionsSlider->drawSliders(juce::dontSendNotification);
        accentsSlider->drawSliders(juce::dontSendNotification);
        sustainLengthMultipliersSlider->drawSliders(juce::dontSendNotification);
        beatLengthMultipliersSlider->drawSliders(juce::dontSendNotification);

        // for updating the current sliders in multisliders
        startTimer(50);

    }

    void timerCallback() override;

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);

        drawLabelForComponent(g, TRANS("num pulses"), numPulses_knob.get());
        drawLabelForComponent(g, TRANS("num layers"), numLayers_knob.get());
        drawLabelForComponent(g, TRANS("cluster thickness"), clusterThickness_knob.get());
        drawLabelForComponent(g, TRANS("cluster threshold"), clusterThreshold_knob.get());

        paintChildrenBackgrounds (g);
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;

    // combo box menus
    std::unique_ptr<OpenGLComboBox> pulseTriggeredBy_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> pulseTriggeredBy_attachment;
    std::unique_ptr<OpenGLComboBox> determinesCluster_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> determinesCluster_attachment;
    std::shared_ptr<PlainTextComponent> pulseTriggeredBy_label;
    std::shared_ptr<PlainTextComponent> determinesCluster_label;

    // multisliders
    std::unique_ptr<OpenGL_MultiSlider> transpositionsSlider;
    std::unique_ptr<OpenGL_MultiSlider> accentsSlider;
    std::unique_ptr<OpenGL_MultiSlider> sustainLengthMultipliersSlider;
    std::unique_ptr<OpenGL_MultiSlider> beatLengthMultipliersSlider;

    // range sliders
    std::unique_ptr<OpenGL_ClusterMinMaxSlider> clusterMinMaxSlider;
    std::unique_ptr<OpenGL_HoldTimeMinMaxSlider> holdTimeMinMaxSlider;

    // ADSR controller
    std::unique_ptr<EnvelopeSection> envSection;

    // knobs
    std::unique_ptr<SynthSlider> numPulses_knob;
    std::unique_ptr<chowdsp::SliderAttachment> numPulses_knob_attachment;
    std::unique_ptr<SynthSlider> numLayers_knob;
    std::unique_ptr<chowdsp::SliderAttachment> numLayers_knob_attachment;
    std::unique_ptr<SynthSlider> clusterThickness_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThickness_knob_attachment;
    std::unique_ptr<SynthSlider> clusterThreshold_knob;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThreshold_knob_attachment;

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    SynchronicParams& sparams_;

    void resized() override;
};

#endif //BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H
