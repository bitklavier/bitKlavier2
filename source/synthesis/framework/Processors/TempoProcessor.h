//
// Created by Myra Norton on 8/20/2025.
//

#pragma once

#include "EnvParams.h"
#include "HoldTimeMinMaxParams.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "TransposeParams.h"
#include "TuningProcessor.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>

enum TempoModeType
{
    Constant_Tempo = 1 << 0,
    Adaptive2Time_Between_Notes = 1 << 1, // 2 => -, "Adaptive-Time Between Notes
    Adaptive2Sustain_Time = 1 << 2,
};

struct TempoParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Direct Processor
    TempoParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("tempo")
    {
        add (tempoParam,
            subdivisionsParam,
            historyParam,
            tempoModeOptions,
            timeWindowMinMaxParams);

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        // used in the DirectProcessor constructor
        doForAllParameters ([this] (auto& param, size_t) {
            // if (auto* sliderParam = dynamic_cast<chowdsp::ChoiceParameter*> (&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.push_back ( sliderParam);
            //
            // if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.push_back ( sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back ( sliderParam);
   });
    }

    // Tempo param
    chowdsp::FloatParameter::Ptr tempoParam {
        juce::ParameterID { "tempo", 100 },
        "TEMPO",
        chowdsp::ParamUtils::createNormalisableRange (40.0f, 208.0f, 124.0f),
        120.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // Subdivision param
    chowdsp::FloatParameter::Ptr subdivisionsParam {
        juce::ParameterID { "subdivisions", 100 },
        "SUBDIVISIONS",
        chowdsp::ParamUtils::createNormalisableRange (0.01f, 32.0f, 16.0f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // Subdivision param
    chowdsp::FloatParameter::Ptr historyParam {
        juce::ParameterID { "history", 100 },
        "HISTORY",
        chowdsp::ParamUtils::createNormalisableRange (1.f, 10.f, 5.f, 1.f),
        1.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    chowdsp::EnumChoiceParameter<TempoModeType>::Ptr tempoModeOptions{
        juce::ParameterID{"determinesCluster", 100},
        "determines cluster",
        TempoModeType::Constant_Tempo,
        std::initializer_list<std::pair<char, char>>{{'_', ' '}, {'1', '/'}, {'2', '-'}, {'3', '\''}, {'4', '#'}, {'5', 'b'}}};

    HoldTimeMinMaxParams timeWindowMinMaxParams;

    void processStateChanges() override
    {
        timeWindowMinMaxParams.processStateChanges();
    }
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

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    // void processContinuousModulations(juce::AudioBuffer<float>& buffer);

    bool acceptsMidi() const override { return true; }

    juce::AudioProcessor::BusesProperties tempoBusLayout()
    {
        return BusesProperties()
            .withOutput("Output", juce::AudioChannelSet::stereo(), false) // Main Output
            .withInput ("Input", juce::AudioChannelSet::stereo(), false)  // Main Input (not used here)
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (10), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false);  // Modulation send channel; disabled for all but Modulation preps!
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    void setGlobalTempoMultiplier(float gTM)
    {
        globalTempoMultiplier = gTM;
        DBG("global tempo multiplier updated " << globalTempoMultiplier);
    }
    float getGlobalTempoMultiplier() { return globalTempoMultiplier; }
    double globalTempoMultiplier = 1.;

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


