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
#include "PreparationStateImpl.h"
#include "tuning_systems.h"
#include "array_to_string.h"
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


struct TuningKeyboardState : bitklavier::StateChangeableParameter {

    juce::MidiKeyboardState keyboardState;

    std::array<float,128> absoluteTuningOffset = {0.f};
    std::array<float,12> circularTuningOffset = {0.f};
    int fundamental = 0;

    void setKeyOffset(int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128) absoluteTuningOffset[midiNoteNumber] = val; //.set(midiNoteNumber, val);
    }

    void setCircularKeyOffset(int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 12) circularTuningOffset[midiNoteNumber] = val; //.set(midiNoteNumber, val);
        DBG("setCircularKeyOffset " + juce::String(midiNoteNumber) + " : " + juce::String(val));
    }

    void setKeyOffset(int midiNoteNumber, float val, bool circular)
    {
        if (circular) setCircularKeyOffset(midiNoteNumber,val);
        else setKeyOffset(midiNoteNumber,val);
    }

    void processStateChanges() override {
        for (auto [index,change] : stateChanges.changeState) {
            static juce::var nullVar;
            auto val    = change.getProperty(IDs::absoluteTuning);
            auto val1  = change.getProperty(IDs::circularTuning);
            if (val != nullVar) {
                absoluteTuningOffset = parseIndexValueStringToArrayAbsolute<128>(val.toString().toStdString());
            }else if (val1 !=nullVar) {
                circularTuningOffset = parseFloatStringToArrayCircular<12>(val1.toString().toStdString());
               // absoluteTuningOffset = std::array<float,128>(val1.toString().toStdString());

            }
        }
    }

    static std::array<float,12> rotateValuesByFundamental(std::array<float,12> vals, int fundamental) {
        int offset;
        if(fundamental <= 0) offset = 0;
        else offset = fundamental;
        std::array<float,12> new_vals = {0.f};
        for(int i=0; i<12; i++)
        {
            int index = ((i - offset) + 12) % 12;
            new_vals[i] = vals[index];
        }
        return new_vals;
    }

    void setFundamental(int fund) {
        //need to shift keyValues over by difference in fundamental
        int oldFund = fundamental;
        fundamental = fund;
        int offset = fund - oldFund;
        auto vals  = circularTuningOffset;
        for ( int i= 0 ;i<12;i++) {
            int index = ((i - offset) + 12) % 12;
            circularTuningOffset[i] = vals[index];
        }
    }

    std::atomic<bool> setFromAudioThread;
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
    /** Custom serializer */
    template <typename Serializer>
    static typename Serializer::SerializedType serialize (const TuningParams& paramHolder);

    /** Custom deserializer */
    template <typename Serializer>
    static void deserialize (typename Serializer::DeserializedType deserial, TuningParams& paramHolder);
};

struct TuningNonParameterState : chowdsp::NonParamState
{
    TuningNonParameterState()
    {
    }

};



class TuningProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<TuningParams,TuningNonParameterState>>
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
