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
#include <array>

    inline constexpr std::array<float,12> tEqualTuning     =  {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.} ;
	inline constexpr std::array<float,12> tJustTuning      = { 0., .117313, .039101, .156414, -.13686, -.019547, -.174873, .019547, .136864, -.15641, -.311745, -.11731 };
	inline constexpr std::array<float,12> tPartialTuning   = { 0., .117313, .039101, -.331291, -.13686, -.019547, -.486824, .019547, .405273, -.15641, -.311745, -.506371 };
	inline constexpr std::array<float,12> tDuodeneTuning   = { 0., .117313, .039101, .156414, -.13686, -.019547, -.097763, .019547, .136864, -.15641, -.039101, -.11731 };
	inline constexpr std::array<float,12> tOtonalTuning    = { 0., .049553, .039101, -.02872, -.13686, -.292191, -.486824, .019547, .405273, .058647, -.311745, -.11731 } ;
	inline constexpr std::array<float,12> tUtonalTuning    = { 0., .117313, .311745, .156414, -.405273, -.019547, .486824, .292191, .136864, .024847, -.039101, -.049553 } ;

    //historical temperaments
    inline constexpr std::array<float,12> tPythagorean     = { 0., 0.13685, 0.0391, -0.05865, 0.0782, -0.01955, 0.1173, 0.01955, 0.1564, 0.05865, -0.0391, 0.09775} ;
    inline constexpr std::array<float,12> tGrammateus      = { 0., 0.01955, 0.0391, 0.05865, 0.0782, -0.01955, 0, 0.01955, 0.0391, 0.05865, 0.0782, 0.09775 };
    inline constexpr std::array<float,12> tKirnbergerII    = { 0., -0.0977871, 0.0391, -0.0586871, -0.1369, -0.0195871, -0.0978, 0.01955, -0.0782371, -0.0489, -0.0391371, -0.11735 };
    inline constexpr std::array<float,12> tKirnbergerIII   = { 0., -0.0977871, -0.06845, -0.0586871, -0.1369, -0.0195871, -0.0978, -0.034225, -0.0782371, -0.102675, -0.0391371, -0.11735 };

    inline constexpr std::array<float,12> tWerkmeisterIII           = {0, -0.0977871, -0.06845, -0.0586871, -0.083125, -0.0195871, -0.0978, -0.034225, -0.0782371, -0.102675, -0.0391371, -0.063575 };
    inline constexpr std::array<float,12> tQuarterCommaMeantone     = {0, -0.239575, -0.06845, 0.122175, -0.1369, 0.053725, -0.20535, -0.034225, -0.2738, -0.102675, 0.08795, -0.171125 };
    inline constexpr std::array<float,12> tSplitWolfQCMeantone      = {0, -0.239575, -0.06845, -0.092925, -0.1369, 0.053725, -0.20535, -0.034225, -0.2738, -0.102675, 0.08795, -0.171125 };
    inline constexpr std::array<float,12> tTransposingQCMeantone    = {0, -0.239575, -0.06845, 0.122175, -0.1369, 0.053725, -0.20535, -0.034225, 0.1564, -0.102675, 0.08795, -0.171125 };
    inline constexpr std::array<float,12> tCorrette                 = {0, -0.239575, -0.06845, -0.0212249, -0.1369, 0.0509674, -0.20535, -0.034225, -0.23795, -0.102675, 0.0148713, -0.171125 };
    inline constexpr std::array<float,12> tRameau                   = {0, -0.132025, -0.06845, -0.0301876, -0.1369, 0.0537248, -0.151575, -0.034225, -0.148325, -0.102675, 0.0879498, -0.171125 };
    inline constexpr std::array<float,12> tMarpourg                 = {0, -0.14995, -0.06845, -0.0391498, -0.1369, 0.0716504, -0.20535, -0.034225, -0.0945499, -0.102675, 0.0162503, -0.171125 };
    inline constexpr std::array<float,12> tEggarsEnglishOrd         = {0, -0.1858, -0.06845, -0.092925, -0.1369, 0.053725, -0.178463, -0.034225, -0.16625, -0.102675, -0.0196, -0.171125 };
    inline constexpr std::array<float,12> tThirdCommaMeantone       = {0, -0.365049, -0.1043, 0.175951, -0.2086, 0.0716508, -0.3129, -0.0521499, -0.417199, -0.15645, 0.123801, -0.26075 };
    inline constexpr std::array<float,12> tDAlembertRousseau        = {0, -0.132025, -0.06845, -0.0943036, -0.1369, -0.0221113, -0.13365, -0.034225, -0.1304, -0.102675, -0.0582074, -0.135275 };
    inline constexpr std::array<float,12> tKellner                  = {0, -0.09775, -0.05474, -0.05865, -0.10948, -0.01955, -0.1173, -0.02737, -0.0782, -0.08211, -0.0391, -0.08993 };
    inline constexpr std::array<float,12> tVallotti                 = {0, -0.0586504, -0.0391002, -0.0195504, -0.0782003, 0.0195496, -0.0782004, -0.0195501, -0.0391004, -0.0586502, 0, -0.0977504 };
    inline constexpr std::array<float,12> tYoungII                  = {0, -0.0977505, -0.0391002, -0.0586505, -0.0782003, -0.0195505, -0.1173, -0.0195501, -0.0782005, -0.0586502, -0.0391005, -0.0977504 };
    inline constexpr std::array<float,12> tSixthCommaMeantone       = {0, -0.136851, -0.0391002, 0.0586493, -0.0782003, 0.0195491, -0.1173, -0.0195501, -0.156401, -0.0586502, 0.0390992, -0.0977504 };
    inline constexpr std::array<float,12> tBachBarnes               = {0, -0.0586504, -0.0391002, -0.0195504, -0.0782003, 0.0195496, -0.0782004, -0.0195501, -0.0391004, -0.0586502, 0, -0.0586503};
    inline constexpr std::array<float,12> tNeidhardt                = {0, -0.0391, -0.0391002, -0.0195499, -0.0586502, -0.0195498, -0.0391001, -0.0195501, -0.0390999, -0.0586502, -0.0195498, -0.0391002 };
    inline constexpr std::array<float,12> tBachLehman               = {0, -0.0195503, -0.0391002, -0.0195502, -0.0782003, 0.0180461, -0.0391003, -0.0195501, -0.0195502, -0.0586502, -0.0195501, -0.0586503 };
    inline constexpr std::array<float,12> tBachODonnell             = {0, -0.0391002, -0.0391002, -0.0391001, -0.0391002, -0.01955, -0.0391003, -0.0195501, -0.0391002, -0.0586502, -0.0391, -0.0586503 };
    inline constexpr std::array<float,12> tBachHill                 = {0, -0.0436113, -0.0330845, -0.0225575, -0.0661691, 0.0165425, -0.0451152, -0.0165423, -0.0421075, -0.0496268, -0.00300749, -0.0466191 };
    inline constexpr std::array<float,12> tBachSwich                = {0, -0.0703799, -0.05474, -0.0182465, -0.10948, 0.0273702, -0.0899299, -0.02737, -0.0443132, -0.08211, 0.00782023, -0.08993 };
    inline constexpr std::array<float,12> tLambert                  = {0, -0.0642355, -0.0279285, -0.0251355, -0.055857, 0.0139645, -0.0837855, -0.0139642, -0.0446855, -0.0418928, -0.00558551, -0.0698213 };
    inline constexpr std::array<float,12> tEighthCommaWT            = {0, -0.0391, -0.01955, 0, -0.0391, 0.009775, -0.05865, -0.009775, -0.01955, -0.029325, 0.01955, -0.048875 };
    inline constexpr std::array<float,12> tPinnockModern            = {0, -0.0390998, -0.01955, 0, -0.0391, 0.0195503, -0.0390998, -0.009775, -0.0195498, -0.029325, 0, -0.0390999 };

    /*
     also add:
     'common just': [1/1, 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 9/5, 15/8, 2/1],
     'symmetric': [1/1, 16/15, 9/8, 6/5, 5/4, 4/3, Math.sqrt(2), 3/2, 8/5, 5/3, 16/9,  15/8,  2/1],
     'well tuned piano': [1/1, 567/512, 9/8, 147/128, 21/16, 1323/1024, 189/128, 3/2, 49/32, 7/4, 441/256, 63/32, 2/1],
     'harrison' : 1/1, 28/27, 9/8, 32/27, 5/4, 4/3, 112/81, 3/2, 128/81, 5/3, 16/9, 15/8 //from Strict Songs
    */

    inline constexpr std::array<float,12> tCommonJust              = {0, 0.11731285269777758, 0.039100017307748376, 0.15641287000552553, -0.13686286135165177, -0.019550008653875465, -0.09776284404390367, 0.019550008653873192, 0.13686286135165232, -0.15641287000552553, 0.175962878659401, -0.117312852697778};
    inline constexpr std::array<float,12> tSymmetric               = {0, 0.11731285269777758, 0.039100017307748376, 0.15641287000552553, -0.13686286135165177, -0.019550008653875465, 0., 0.019550008653873192, 0.13686286135165232, -0.15641287000552553, -0.03910001730774866, -0.117312852697778 };
    inline constexpr std::array<float,12> tWellTunedPiano          = {0, 0.7664590993067458, 0.039100017307748376, -0.603931861963627, 0.7078090733451233, -0.564831844655879, 0.746909090652872, 0.019550008653873192, -0.6234818706175008, 0.6882590646912502, -0.5843818533097533, 0.727359081999 };
    inline constexpr std::array<float,12> tHarrisonStrict          = {0, -0.3703909612703742, 0.039100017307748376, -0.05865002596162355, -0.13686286135165177, -0.019550008653875465, -0.3899409699242483, 0.019550008653873192, -0.07820003461549846, -0.15641287000552553, -0.03910001730774866, -0.117312852697778 };


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
    Young_Well_Tuned_Piano = 1LL << 38,
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
template <std::size_t N>
std::array<float, N> parseIndexValueStringToArrayAbsolute(const std::string& input)
{
    std::array<float, N> result{0};
    std::istringstream iss(input);
    std::string token;

    while (iss >> token)
    {
        auto colonPos = token.find(':');
        if (colonPos == std::string::npos) continue;

        int index = std::stoi(token.substr(0, colonPos));
        float value = std::stof(token.substr(colonPos + 1));

        if (index >= 0 && static_cast<std::size_t>(index) < N)
            result[index] = value;
    }

    return result;
}
template <std::size_t N>
std::array<float, N> parseFloatStringToArrayCircular(const std::string& input)
{
    std::array<float, N> result{};
    std::istringstream iss(input);
    float value;
    std::size_t i = 0;

    while (i < N && iss >> value)
    {
        result[i++] = value;
    }

    // If fewer floats than N, remaining values stay zero (default initialized)

    return result;
}

struct TuningKeyboardState : bitklavier::StateChangeableParameter {
    juce::MidiKeyboardState keyboardState;
    std::array<float,128> absoluteTuningOffset = {0.f};
    int fundamental  =0;
    void setKeyOffset(int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128) absoluteTuningOffset[midiNoteNumber] = val; //.set(midiNoteNumber, val);
    }
    std::array<float,12> circularTuningOffset = {0.f};
    void setCircularKeyOffset(int midiNoteNumber, float val)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 12) circularTuningOffset[midiNoteNumber] = val; //.set(midiNoteNumber, val);
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
