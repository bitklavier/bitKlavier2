//
// Created by Davis Polito on 5/2/24.
//

#pragma once

#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "Synthesiser/Sample.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include "EnvParams.h"
#include "TransposeParams.h"
#include "buffer_debugger.h"
#include "Identifiers.h"
#include "VelocityMinMaxParams.h"
#include "TuningProcessor.h"

struct DirectParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -60.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    using ParamPtrVariant = std::variant<chowdsp::FloatParameter*, chowdsp::ChoiceParameter*, chowdsp::BoolParameter*>;
    std::unordered_map<std::string,ParamPtrVariant> modulatableParams;

    // Adds the appropriate parameters to the Direct Processor
    DirectParams() : chowdsp::ParamHolder ("direct")
    {
        //add (gainParam, hammerParam, releaseResonanceParam, pedalParam, velocityParam, attackParam, decayParam, sustainParam, releaseParam, transpositionsParam);
        //add (gainParam, hammerParam, releaseResonanceParam, pedalParam, velocityParam, attackParam, decayParam, sustainParam, releaseParam);
        add (gainParam, hammerParam, releaseResonanceParam, pedalParam, blendronicSend,  env, transpose, velocityMinMax);
        doForAllParameters ([this] (auto& param, size_t) {
            if (auto *sliderParam = dynamic_cast<chowdsp::ChoiceParameter *> (&param))
                if(sliderParam->supportsMonophonicModulation())
                    modulatableParams.insert({ sliderParam->paramID.toStdString(),  sliderParam});
//
            if (auto *sliderParam = dynamic_cast<chowdsp::BoolParameter *> (&param))
                if(sliderParam->supportsMonophonicModulation())
                    modulatableParams.insert({ sliderParam->paramID.toStdString(),  sliderParam});

            if (auto *sliderParam = dynamic_cast<chowdsp::FloatParameter *> (&param))
                if(sliderParam->supportsMonophonicModulation())
                    modulatableParams.insert({ sliderParam->paramID.toStdString(),  sliderParam});
        });
    }

    // Gain param
    chowdsp::GainDBParameter::Ptr gainParam {
        juce::ParameterID { "Main", 100 },
        "Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,true
    };

    // Hammer param
    chowdsp::GainDBParameter::Ptr hammerParam {
        juce::ParameterID { "Hammers", 100 },
        "Hammer",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        -6.0f
    };

    // Resonance param
    chowdsp::GainDBParameter::Ptr releaseResonanceParam {
        juce::ParameterID { "Resonance", 100 },
        "Release Resonance",
        juce::NormalisableRange { rangeStart, rangeEnd + 24, 0.0f, skewFactor, false },
        6.0f,true
    };

    // Pedal param
    chowdsp::GainDBParameter::Ptr pedalParam {
        juce::ParameterID { "Pedal", 100 },
        "Pedal",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        -6.0f,true
    };

    // Gain param
    chowdsp::GainDBParameter::Ptr blendronicSend {
        juce::ParameterID { "Send", 100 },
        "Send",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,true
    };

    // Velocity param
    chowdsp::FloatParameter::Ptr velocityParam {
        juce::ParameterID { "Velocity", 100 },
        "Velocity",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 127.0f, 63.f), // FIX
        127.0f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // ADSR params
    EnvParams env;

    // Transposition slider (holds up to 12 transposition values)
    TransposeParams transpose;
    VelocityMinMaxParams velocityMinMax;

    // for storing outputLevels of this preparation for display
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;

    //
    //
    //    // Blendronic Send param
    //    chowdsp::GainDBParameter::Ptr blendronicSendParam {
    //            juce::ParameterID { "blendronicSend", 100 },
    //            "BlendronicSend",
    //            juce::NormalisableRange { -30.0f, 0.0f }, // FIX
    //            -24.0f
    //    };

    /****************************************************************************************/
};

struct DirectNonParameterState : chowdsp::NonParamState
{
    DirectNonParameterState()
    {
        //addStateValues ({ /*,&isSelected*/});
    }

    //chowdsp::StateValue<juce::Point<int>> prepPoint { "prep_point", { 300, 500 } };
    //chowdsp::StateValue<bool> isSelected { "selected", true };
};

class SynthBase;
class DirectProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<DirectParams, DirectNonParameterState>>,
public juce::ValueTree::Listener
{
public:
    DirectProcessor (SynthBase* parent,const juce::ValueTree& v);
    ~DirectProcessor()
    {

    }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    bool acceptsMidi() const override
    {
        return true;
    }

    void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s)
    {
        ptrToSamples = s;
    }

    void addSoundSet (
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s, // main samples
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h, // hammer samples
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r, // release samples
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) // pedal samples
    {
        DBG ("adding main, hammer and releaseResonance synths");
        mainSynth->addSoundSet (s);
        hammerSynth->addSoundSet (h);
        releaseResonanceSynth->addSoundSet (r);
        pedalSynth->addSoundSet (p);
    }

    juce::AudioProcessor::BusesProperties directBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output1", juce::AudioChannelSet::stereo(), true)
            .withInput("input",juce::AudioChannelSet::stereo(),false)
            .withInput ("Modulation",juce::AudioChannelSet::discreteChannels(9) ,true);
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    void addToVT (juce::ValueTree& vt)
    {
        state.params.doForAllParameters ([this, &vt] (auto& param, size_t) {
            vt.setProperty (param.paramID, chowdsp::ParameterTypeHelpers::getValue (param), nullptr);
        });
    }

    void valueTreePropertyChanged   (juce::ValueTree& t, const juce::Identifier&) {
        juce::String a  = t.getProperty(IDs::mainSampleSet, "");
        juce::String b  = t.getProperty(IDs::hammerSampleSet, "");
        juce::String c  = t.getProperty(IDs::releaseResonanceSampleSet, "");
        juce::String d  = t.getProperty(IDs::pedalSampleSet, "");
        addSoundSet (&(*ptrToSamples)[a],
                &(*ptrToSamples)[b],
                &(*ptrToSamples)[c],
                &(*ptrToSamples)[d]);

    }

    void valueTreeChildAdded        (juce::ValueTree&, juce::ValueTree&)        {}
    void valueTreeChildRemoved      (juce::ValueTree&, juce::ValueTree&, int)   {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int)          {}
    void valueTreeParentChanged     (juce::ValueTree&)                    {}
    void valueTreeRedirected        (juce::ValueTree&)                    {}

    double getLevelL()
    {
        if(levelBuf.getNumSamples() > 0) return levelBuf.getRMSLevel(0, 0, levelBuf.getNumSamples());
        else return 0.;
    }

    double getLevelR()
    {
        if(levelBuf.getNumChannels() == 2) {
            if(levelBuf.getNumSamples()) return levelBuf.getRMSLevel(1, 0, levelBuf.getNumSamples());
            else return 0.;
        }
        else return getLevelL();
    }

private:
    //chowdsp::experimental::Directillator<float> oscillator;
    chowdsp::Gain<float> gain;
    //juce::ADSR::Parameters adsrParams;
    juce::ScopedPointer<BufferDebugger> bufferDebugger;
    std::unique_ptr<BKSynthesiser> mainSynth;
    std::unique_ptr<BKSynthesiser> hammerSynth;
    std::unique_ptr<BKSynthesiser> releaseResonanceSynth;
    std::unique_ptr<BKSynthesiser> pedalSynth;

    float releaseResonanceSynthGainMultiplier = 10.; // because these are very soft
    //juce::HashMap<int, juce::Array<float>> transpositionsByNoteOnNumber; // indexed by noteNumber
    juce::Array<float> midiNoteTranspositions;
    juce::Array<float> getMidiNoteTranspositions();

    //juce::HashMap<int, juce::Array<float>> transpositionsByNoteOnNumber; // indexed by noteNumber
    std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* ptrToSamples;

    chowdsp::ScopedCallbackList adsrCallbacks;
    chowdsp::ScopedCallbackList vtCallbacks;

    juce::AudioSampleBuffer levelBuf; //for storing samples for metering/RMS calculation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectProcessor)
};
