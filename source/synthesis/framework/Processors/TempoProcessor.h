//
// Created by Myra Norton on 8/20/2025.
//

#pragma once

#include "EnvParams.h"
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

struct TempoParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Direct Processor
    TempoParams() : chowdsp::ParamHolder ("tempo")
    {
        add (tempoParam,
            subdivisionsParam);

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

    // Tempo param
    chowdsp::FloatParameter::Ptr tempoParam {
        juce::ParameterID { "tempo", 100 },
        "Tempo",
        chowdsp::ParamUtils::createNormalisableRange (40.0f, 208.0f, 124.0f),
        120.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // Subdivision param
    chowdsp::FloatParameter::Ptr subdivisionsParam {
        juce::ParameterID { "subdivisions", 100 },
        "Subdivisions",
        chowdsp::ParamUtils::createNormalisableRange (0.01f, 32.0f, 16.0f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };
    /****************************************************************************************/
};

struct TempoNonParameterState : chowdsp::NonParamState
{
    TempoNonParameterState() {}
};


// ********************************************************************************************* //
// ************************************** TempoProcessor ************************************** //
// ********************************************************************************************* //

class TempoProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<TempoParams, TempoNonParameterState>>,
                        public juce::ValueTree::Listener
{
public:
    TempoProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~TempoProcessor() {}

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<TempoProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    // void processContinuousModulations(juce::AudioBuffer<float>& buffer);

    bool acceptsMidi() const override { return true; }
    // void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s)
    // {
    //     ptrToSamples = s;
    // }
    //
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

    // void setTuning (TuningProcessor*) override;

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties tempoBusLayout()
    {
        return BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
            .withInput ("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (10), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false);  // Modulation send channel; disabled for all but Modulation preps!
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }


    // void valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier&)
    // {
    //     //should add an if check here to make sure its actually the sampleset changing
    //     juce::String a = t.getProperty (IDs::mainSampleSet, "");
    //     juce::String b = t.getProperty (IDs::hammerSampleSet, "");
    //     juce::String c = t.getProperty (IDs::releaseResonanceSampleSet, "");
    //     juce::String d = t.getProperty (IDs::pedalSampleSet, "");
    //     addSoundSet (&(*ptrToSamples)[a],
    //         &(*ptrToSamples)[b],
    //         &(*ptrToSamples)[c],
    //         &(*ptrToSamples)[d]);
    // }

    /**
     * todo: do we need these?
     * DAVIS: this just explicitly defines the other valuetree listener functions to be doing nothing
     * we only care about the treepropertychanged valuetree
     */
//    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) {}
//    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) {}
//    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) {}
//    void valueTreeParentChanged (juce::ValueTree&) {}
//    void valueTreeRedirected (juce::ValueTree&) {}

//    bool getTranspositionUsesTuning() { return state.params.transpose.transpositionUsesTuning->get();}


private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TempoProcessor)
};


