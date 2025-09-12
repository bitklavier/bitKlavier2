//
// Created by Davis Polito on 5/2/24.
//

#pragma once

#include "EnvParams.h"
#include "EnvelopeSequenceParams.h"
#include "Identifiers.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "TransposeParams.h"
#include "TuningProcessor.h"
#include "VelocityMinMaxParams.h"
#include "buffer_debugger.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include "HoldTimeMinMaxParams.h"

// wave distance (0 to 20000), how far back you go, higher wave distance means more gentle wave
// wave section (wrap in an opengl wrapper, transposition slider is done this way)
// line that goes through it means tracking the playback position (synthesizer status in direct)
// undertow (0 to 9320), goes forward, dynamically shorten?

// key on reset checkbox


struct NostalgicParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // using ParamPtrVariant = std::variant<chowdsp::FloatParameter*, chowdsp::ChoiceParameter*, chowdsp::BoolParameter*>;
    // std::vector<ParamPtrVariant> modulatableParams;

    // Adds the appropriate parameters to the Nostalgic Processor
    NostalgicParams() : chowdsp::ParamHolder ("nostalgic")
    {
        add (transpose,
            outputSendGain,
            outputGain,
            noteLengthMultParam,
            clusterMinParam,
            clusterThreshParam,
            holdTimeMinMaxParams,
            reverseEnv,
            // reverseEnvSequence,
            undertowEnv
            // undertowEnvSequence
            );

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        // used in the NostalgicProcessor constructor
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
    // transposition slider
    TransposeParams transpose;

    // Hold time range slider
    HoldTimeMinMaxParams holdTimeMinMaxParams;

    // Reverse and Undertow ADSRs and params
    EnvParams reverseEnv{"Reverse"};
    // EnvelopeSequenceParams reverseEnvSequence;
    EnvParams undertowEnv{"Undertow"};
    // undertowEnv.idPrepend = "1";
    // EnvelopeSequenceParams undertowEnvSequence;



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
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
    };

    // Note Length Multiplier param
    chowdsp::GainDBParameter::Ptr noteLengthMultParam {
        juce::ParameterID { "NoteLengthMultiplier", 100 },
        "Note Length Multiplier",
        juce::NormalisableRange { 0.0f, 10.00f, 0.0f, skewFactor, false },
        1.0f,
        true
    };

    // Cluster Minimum param
    chowdsp::GainDBParameter::Ptr clusterMinParam {
        juce::ParameterID { "ClusterMin", 100 },
        "Cluster Min",
        juce::NormalisableRange { 1.0f, 10.0f, 0.0f, skewFactor, false },
        1.0f,
        true
    };

    // Cluster Threshold param
    chowdsp::GainDBParameter::Ptr clusterThreshParam {
        juce::ParameterID { "ClusterThresh", 100 },
        "Cluster Threshold",
        juce::NormalisableRange { 0.0f, 1000.0f, 0.0f, skewFactor, false },
        150.0f,
        true
    };
    
    /*
     * for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;
    /****************************************************************************************/
};

struct NostalgicNonParameterState : chowdsp::NonParamState
{
    NostalgicNonParameterState() {}
};


// ********************************************************************************************* //
// ************************************** NostalgicProcessor ************************************** //
// ********************************************************************************************* //

class NostalgicProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<NostalgicParams, NostalgicNonParameterState>>,
                        public juce::ValueTree::Listener
{
public:
    NostalgicProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~NostalgicProcessor() {}

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<NostalgicProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    // void setupModulationMappings();

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processContinuousModulations(juce::AudioBuffer<float>& buffer);

    bool acceptsMidi() const override { return true; }
    void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s)
    {
        ptrToSamples = s;
    }

    // void addSoundSet (
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s, // main samples
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h, // hammer samples
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r, // release samples
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) // pedal samples
    // {
    //     mainSynth->addSoundSet (s);
    //     hammerSynth->addSoundSet (h);
    //     releaseResonanceSynth->addSoundSet (r);
    //     pedalSynth->addSoundSet (p);
    // }

    void setTuning (TuningProcessor*) override;

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties nostalgicBusLayout()
    {
        return BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
            .withInput ("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for nostalgic, we have 10:
             *                  - the ADSR params: attackParam, decayParam, sustainParam, releaseParam, and
             *                  - the main params: gainParam, hammerParam, releaseResonanceParam, pedalParam, OutputSendParam, outputGain,
             */
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (10), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false)  // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",juce::AudioChannelSet::stereo(),true);       // Send channel (right outputs)
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    void valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier&)
    {
        // //should add an if check here to make sure its actually the sampleset changing
        // juce::String a = t.getProperty (IDs::mainSampleSet, "");
        // juce::String b = t.getProperty (IDs::hammerSampleSet, "");
        // juce::String c = t.getProperty (IDs::releaseResonanceSampleSet, "");
        // juce::String d = t.getProperty (IDs::pedalSampleSet, "");
        // addSoundSet (&(*ptrToSamples)[a],
        //     &(*ptrToSamples)[b],
        //     &(*ptrToSamples)[c],
        //     &(*ptrToSamples)[d]);
    }

    bool holdCheck(int noteNumber);
    juce::Array<juce::uint64> holdTimers;


private:
    std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* ptrToSamples;
    BKSynthesizerState lastSynthState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NostalgicProcessor)
};
