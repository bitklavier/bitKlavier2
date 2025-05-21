//
// Created by Joshua Warner on 6/27/24.
//

#ifndef BITKLAVIER2_TUNINGPROCESSOR_H
#define BITKLAVIER2_TUNINGPROCESSOR_H

#pragma once

#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

#include "PluginBase.h"
// TODO change params

typedef enum TuningSystem {
    Equal_Temperament = 1 << 0,
    Partial= 1 << 1,
    Just = 1 << 2,
    Duodene = 1 << 3,
    Otonal= 1 << 4,
    Utonal = 1 << 5,
    Custom = 1 << 6,
    Pythagorean = 1 << 9,
    Grammateus = 1 << 10,
    Kirnberger_II = 1 << 11,
    Kirnberger_III = 1 << 12,
    Werkmeister_III = 1 << 13,
    Quarter2Comma_Meantone = 1 << 14,
    Split2Wolf_QC_Meantone = 1 << 15,
    Transposing_QC_Meantone = 1 << 16,
    Corrette = 1 << 17,
    Rameau = 1 << 18,
    Marpurg = 1 << 19,
    Eggars_English_Ord = 1 << 20,
    Third2Comma_Meantone = 1 << 21,
    D_Alembert_Rousseau = 1 << 22,
    Kellner = 1 << 23,
    Vallotti = 1 << 24,
    Young_II = 1 << 25,
    Sixth_Comma_Meantone = 1 << 26,
    Bach1Barnes = 1 << 27,
    Neidhardt = 1 << 28,
    Bach1Lehman = 1 << 29,
    Bach1O3Donnell = 1 << 30,
    Bach1Hill = 1 << 31,
    Bach1Swich = 1LL << 32,
    Lambert = 1LL << 33,
    Eighth2Comma_WT = 1LL << 34,
    Pinnock_Modern = 1LL << 35,
    Common_Just = 1LL << 36,
    Symmetric_Just = 1LL << 37,
    Youn_Well_Tuned_Piano = 1LL << 38,
    Harrison_Strict_Songs = 1LL << 39,
} TuningSystem;

enum Fundamental : uint32_t {
    C        = 1 << 0,
    C41D5    = 1 << 1,
    D        = 1 << 2,
    D41E5    = 1 << 3,
    E        = 1 << 4,
    F        = 1 << 5,
    F41G5    = 1 << 6,
    G        = 1 << 7,
    G41A5    = 1 << 8,
    A        = 1 << 9,
    A41B5    = 1 << 10,
    B        = 1 << 11,
    none     = 0
};

enum AdaptiveSystems {
    None = 1<<0,
    Adaptive = 1<<1,
    Adaptive_Anchored = 1<<2,
    Spring = 1<<3,
};


struct TuningKeyboardState  {
    juce::MidiKeyboardState keyboardState;
    std::array<float,128> tuningOffset = {0.f};
    void setKeyOffset(int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128) tuningOffset[midiNoteNumber] = val; //.set(midiNoteNumber, val);
    }
};


struct TuningParams : chowdsp::ParamHolder
{

    // Adds the appropriate parameters to the Tuning Processor
    TuningParams() : chowdsp::ParamHolder("tuning")
    {
        add (tuningSystem,fundamental,adaptive);
    }
    chowdsp::EnumChoiceParameter<TuningSystem>::Ptr tuningSystem {
        juce::ParameterID{"tuningSystem" , 100},
        "Tuning System",
        TuningSystem::Equal_Temperament,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } ,{'1','/'} ,{'2','-'},{'3','\''}}
    };
    chowdsp::EnumChoiceParameter<Fundamental>::Ptr fundamental {
        juce::ParameterID{"fundamental" , 100},
        "Fundamental",
        Fundamental::C,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } ,{'1','/'} ,{'2','-'},{'3','\''},{'4', '#'},{'5','b'}}
    };
    chowdsp::EnumChoiceParameter<AdaptiveSystems>::Ptr adaptive {
        juce::ParameterID{"adaptiveSystem" , 100},
        "Adaptive System",
        AdaptiveSystems::None,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } ,{'1','/'} ,{'2','-'},{'3','\''},{'4', '#'},{'5','b'}}
    };

    TuningKeyboardState keyboardState;
};

struct TuningNonParameterState : chowdsp::NonParamState
{
    TuningNonParameterState()
    {
    }

};

class TuningProcessor : public bitklavier::PluginBase<chowdsp::PluginStateImpl<TuningParams,TuningNonParameterState,chowdsp::XMLSerializer>>
{
public:
    TuningProcessor(SynthBase* parent,const juce::ValueTree& v);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    bool acceptsMidi() const override
    {
        return true;
    }
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessor::BusesProperties tuningBusLayout()
    {
        return BusesProperties()
            .withOutput ("Output1", juce::AudioChannelSet::stereo(), false)
                .withInput("input",juce::AudioChannelSet::stereo(),false);
    }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

private:

    chowdsp::Gain<float> gain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuningProcessor)
};


#endif //BITKLAVIER2_TUNINGPROCESSOR_H
