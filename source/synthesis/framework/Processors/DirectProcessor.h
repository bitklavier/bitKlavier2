//
// Created by Davis Polito on 5/2/24.
//

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

struct DirectParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -60.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    using ParamPtrVariant = std::variant<chowdsp::FloatParameter*, chowdsp::ChoiceParameter*, chowdsp::BoolParameter*>;
    std::unordered_map<std::string, ParamPtrVariant> modulatableParams;

    // Adds the appropriate parameters to the Direct Processor
    DirectParams() : chowdsp::ParamHolder ("direct")
    {
        add (gainParam,
            hammerParam,
            releaseResonanceParam,
            pedalParam,
            outputSendParam,
            outputGain,
            env,
            transpose,
            velocityMinMax);

        doForAllParameters ([this] (auto& param, size_t) {
            if (auto* sliderParam = dynamic_cast<chowdsp::ChoiceParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.insert ({ sliderParam->paramID.toStdString(), sliderParam });

            if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.insert ({ sliderParam->paramID.toStdString(), sliderParam });

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.insert ({ sliderParam->paramID.toStdString(), sliderParam });
        });
    }

    // Gain param
    chowdsp::GainDBParameter::Ptr gainParam {
        juce::ParameterID { "Main", 100 },
        "Main",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
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
        6.0f,
        true
    };

    // Pedal param
    chowdsp::GainDBParameter::Ptr pedalParam {
        juce::ParameterID { "Pedal", 100 },
        "Pedal",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        -6.0f,
        true
    };

    // Gain for output send (for blendronic, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSendParam {
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

    // ADSR params
    EnvParams env;

    // Transposition slider (holds up to 12 transposition values)
    TransposeParams transpose;

    /**
     * for storing min/max values for the velocityMinMax slider
     * and also keeping track of the lastVelocity, which we'll get
     * from the lastSynthState in processBlock()
     * the code for OpenGL_VelocityMinMaxSlider has further comments about
     * how the chowdsp system works with params, callbacks, and so on.
     */
    VelocityMinMaxParams velocityMinMax;

    /** for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;

    /****************************************************************************************/
};

struct DirectNonParameterState : chowdsp::NonParamState
{
    DirectNonParameterState() {}
};

class DirectProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<DirectParams, DirectNonParameterState>>,
                        public juce::ValueTree::Listener
{
public:
    DirectProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~DirectProcessor()
    {
    }
    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<DirectProcessor> (parent, v);
    }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    bool acceptsMidi() const override { return true; }
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
        mainSynth->addSoundSet (s);
        hammerSynth->addSoundSet (h);
        releaseResonanceSynth->addSoundSet (r);
        pedalSynth->addSoundSet (p);
    }

    void setTuning (TuningProcessor*) override;
    juce::AudioProcessor::BusesProperties directBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output1", juce::AudioChannelSet::stereo(), true)
            .withInput ("input", juce::AudioChannelSet::stereo(), false)
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (9), true);
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

    void valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier&)
    {
        juce::String a = t.getProperty (IDs::mainSampleSet, "");
        juce::String b = t.getProperty (IDs::hammerSampleSet, "");
        juce::String c = t.getProperty (IDs::releaseResonanceSampleSet, "");
        juce::String d = t.getProperty (IDs::pedalSampleSet, "");
        addSoundSet (&(*ptrToSamples)[a],
            &(*ptrToSamples)[b],
            &(*ptrToSamples)[c],
            &(*ptrToSamples)[d]);
    }

    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) {}
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) {}
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) {}
    void valueTreeParentChanged (juce::ValueTree&) {}
    void valueTreeRedirected (juce::ValueTree&) {}



private:
    chowdsp::Gain<float> gain;
    juce::ScopedPointer<BufferDebugger> bufferDebugger;
    std::unique_ptr<BKSynthesiser> mainSynth;
    std::unique_ptr<BKSynthesiser> hammerSynth;
    std::unique_ptr<BKSynthesiser> releaseResonanceSynth;
    std::unique_ptr<BKSynthesiser> pedalSynth;

    juce::Array<float> midiNoteTranspositions;
    juce::Array<float> getMidiNoteTranspositions();

    std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* ptrToSamples;

    chowdsp::ScopedCallbackList adsrCallbacks;
    chowdsp::ScopedCallbackList vtCallbacks;

    BKSynthesizerState lastSynthState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectProcessor)
};
