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

        paintChildrenBackgrounds (g);
    }

    chowdsp::ScopedCallbackList sliderChangedCallback;

    std::unique_ptr<OpenGL_MultiSlider> transpositionsSlider;
    std::unique_ptr<OpenGL_MultiSlider> accentsSlider;
    std::unique_ptr<OpenGL_MultiSlider> sustainLengthMultipliersSlider;
    std::unique_ptr<OpenGL_MultiSlider> beatLengthMultipliersSlider;

    std::unique_ptr<OpenGL_ClusterMinMaxSlider> clusterMinMaxSlider;
    std::unique_ptr<OpenGL_HoldTimeMinMaxSlider> holdTimeMinMaxSlider;


    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    SynchronicParams& sparams_;

    void resized() override;
};

#endif //BITKLAVIER0_SYNCHRONICPARAMETERSVIEW_H
