//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_SYNCHRONICPROCESSOR_H
#define BITKLAVIER2_SYNCHRONICPROCESSOR_H

#pragma once

#include "EnvParams.h"
#include "Identifiers.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "Synthesiser/Sample.h"
#include "TransposeParams.h"
#include "TuningProcessor.h"
#include "VelocityMinMaxParams.h"
#include "buffer_debugger.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

struct SynchronicParams : chowdsp::ParamHolder
{

    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    using ParamPtrVariant = std::variant<chowdsp::FloatParameter*, chowdsp::ChoiceParameter*, chowdsp::BoolParameter*>;
    std::vector<ParamPtrVariant> modulatableParams;

    // Adds the appropriate parameters to the Synchronic Processor
    SynchronicParams()
    {
        add (outputSendGain,
            outputGain,
            backwardsEnvParams,
            forwardsEnvParams);

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        // used in the DirectProcessor constructor
        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::ChoiceParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);
        });
    }

//    // Velocity param
//    chowdsp::FloatParameter::Ptr velocityParam {
//            juce::ParameterID { "Velocity", 100 },
//            "Velocity",
//            chowdsp::ParamUtils::createNormalisableRange (20.0f, 20000.0f, 2000.0f), // FIX
//            1000.0f,
//            &chowdsp::ParamUtils::floatValToString,
//            &chowdsp::ParamUtils::stringToFloatVal
//    };
//
//    // Attack param
//    chowdsp::TimeMsParameter::Ptr attackParam {
//            juce::ParameterID { "attack", 100 },
//            "attack",
//            chowdsp::ParamUtils::createNormalisableRange (2.01f, 10.0f, 4.0f),
//            3.5f,
//    };

    // Gain for output send (for blendronic, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSendGain {
        juce::ParameterID { "Send", 100 },
        "Send",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    // for the output gain slider, final gain stage for this prep (meter slider on right side of prep)
    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID { "OutputGain", 100 },
        "Output Gain",
        juce::NormalisableRange { -80.0f, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    /*
     * serializers are used for more complex params, called on save and load
     *  - here we need these for all the multisliders
     */
    /* Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const SynchronicParams& paramHolder);

    /* Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, SynchronicParams& paramHolder);

    /*
     * processStateChanges() is used to handle state change modifications, including resets. should be called every block
     *
     * state changes are NOT audio-rate/continuous; those are handled by "modulatableParams" in the parameter definitions
     * examples: TranspParams or velocityMinMaxParams
     *
     * 'stateChanges.change' state will have size() != 0 after a mod or reset is triggered. each change
     * is handled here, and then it is cleared until another mod or reset is triggered
     *
     * in this specific case, if there is a changeState in stateChanges (which will only happen when
     * a mod is triggered) we read through the valueTree and update all the modded transpositions (t0, t1, etc...)
     */
    void processStateChanges() override
    {
//
//        beatLengths.processStateChanges();
//        delayLengths.processStateChanges();
//        smoothingTimes.processStateChanges();
//        feedbackCoeffs.processStateChanges();
//
//        // signal the UI to redraw the sliders
//        if( beatLengths.updateUI == true || delayLengths.updateUI == true || smoothingTimes.updateUI == true || feedbackCoeffs.updateUI == true)
//        {
//            DBG("updateUIState for Multisliders");
//
//            /*
//             * need to actually change the value for the listener to get the message
//             * we're just using updateUIState as a way to notify the UI, and its actual value doesn't matter
//             * so we switch it everything we one of the sliders gets modded.
//             */
//            if(updateUIState->get())
//                updateUIState->setValueNotifyingHost(false);
//            else
//                updateUIState->setValueNotifyingHost(true);
//        }
    }

    /*
     * for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;

    EnvParams forwardsEnvParams;    // for the forward playing Synchronic pulses
    EnvParams backwardsEnvParams;   // for the reverse playing Synchronic pulses (sustain length multipliers < 0)


};

struct SynchronicNonParameterState : chowdsp::NonParamState
{
    SynchronicNonParameterState()
    {
        addStateValues ({ &prepPoint });
    }

    chowdsp::StateValue<juce::Point<int>> prepPoint { "prep_point", { 300, 500 } };
};


// ********************************************************************************************* //
// ************************************ SynchronicProcessor ************************************ //
// ********************************************************************************************* //

class SynchronicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<SynchronicParams, SynchronicNonParameterState>>,
                            public juce::ValueTree::Listener
{
public:
    SynchronicProcessor(SynthBase& parent, const juce::ValueTree& v);
    ~SynchronicProcessor(){}

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<SynchronicProcessor> (parent, v);
    }

    void setupModulationMappings();
    void processContinuousModulations(juce::AudioBuffer<float>& buffer);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    bool acceptsMidi() const override { return true; }
    void addSoundSet (juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s)
    {
        forwardsSynth->addSoundSet (s);
        backwardsSynth->addSoundSet (s);
    }

    void setTuning (TuningProcessor*) override;

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties synchronicBusLayout()
    {
        return BusesProperties()
            .withOutput("Output",       juce::AudioChannelSet::stereo(), true) // Main Output
            .withInput ("Input",        juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for direct, we have 10:
             *                  - the ADSR params: attackParam, decayParam, sustainParam, releaseParam, and
             *                  - the main params: gainParam, hammerParam, releaseResonanceParam, pedalParam, OutputSendParam, outputGain,
             */
            .withInput ("Modulation",   juce::AudioChannelSet::discreteChannels (10), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation",   juce::AudioChannelSet::mono(),false)  // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",         juce::AudioChannelSet::stereo(),true);       // Send channel (right outputs)
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    juce::ScopedPointer<BufferDebugger> bufferDebugger;

    std::unique_ptr<BKSynthesiser> forwardsSynth;
    std::unique_ptr<BKSynthesiser> backwardsSynth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynchronicProcessor)
};



#endif //BITKLAVIER2_SYNCHRONICPROCESSOR_H
