//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "Identifiers.h"
#include "PluginBase.h"
#include "EQFilterParams.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include "chowdsp_parameters/ParamUtils/chowdsp_ParameterTypes.h"

enum EqPresetComboBox {
    EqOff = 1 << 0,
    Highshelf = 1 << 1,
    Lowshelf = 1 << 2,
};

// ********************************************************************************************* //
// ****************************  EQParams  ********************************************* //
// ********************************************************************************************* //

struct EQParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Adds the appropriate parameters to the Nostalgic Processor
    EQParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("eq")
    {
            add(activeEq,
                resetEq,
                inputGain,
                outputSend,
                outputGain,
                loCutFilterParams,
                peak1FilterParams,
                peak2FilterParams,
                peak3FilterParams,
                hiCutFilterParams,
                presets);

        doForAllParameters ([this] (auto& param, size_t) {
            // if (auto* sliderParam = dynamic_cast<chowdsp::BoolParameter*> (&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.emplace_back(sliderParam);

            if (auto* sliderParam = dynamic_cast<chowdsp::FloatParameter*> (&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.emplace_back(sliderParam);
        });
    }

    // active bool
    chowdsp::BoolParameter::Ptr activeEq {
        juce::ParameterID { "activeEq", 100 },
        "activeEq",
        false
    };

    // reset bool
    chowdsp::BoolParameter::Ptr resetEq {
        juce::ParameterID { "resetEq", 100},
        "reset",
        false
    };

    // To adjust the gain of signals coming in to blendronic
    chowdsp::GainDBParameter::Ptr inputGain {
        juce::ParameterID { "InputGain", 100 },
        "Input Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
   };

    // Gain for output send (for other blendronics, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSend {
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

    // combo box
    chowdsp::EnumChoiceParameter<EqPresetComboBox>::Ptr presets {
        juce::ParameterID { "eqPresets", 100 },
        "eqPresets",
        EqPresetComboBox::EqOff,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
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

    // filters
    EQCutFilterParams loCutFilterParams{"loCut"};
    EQPeakFilterParams peak1FilterParams{"peak1"};
    EQPeakFilterParams peak2FilterParams{"peak2"};
    EQPeakFilterParams peak3FilterParams{"peak3"};
    EQCutFilterParams hiCutFilterParams{"hiCut"};

    juce::Array<bool> activeFilters = {false, false, false, false, false};

    // DSP stuff
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    using Chain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter>;
    Chain leftChain;
    Chain rightChain;

    double sampleRate = 44100;

    double magForFreq(double freq) {
        double mag = 1.f;

        // Since leftChain and state.params.rightChain use the same coefficients, it's fine to just get them from left
        if (!leftChain.isBypassed<ChainPositions::Peak1>())
            mag *= leftChain.get<ChainPositions::Peak1>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!leftChain.isBypassed<ChainPositions::Peak2>())
            mag *= leftChain.get<ChainPositions::Peak2>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!leftChain.isBypassed<ChainPositions::Peak3>())
            mag *= leftChain.get<ChainPositions::Peak3>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<0>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<0>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<1>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<1>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<2>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<2>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::LowCut>().isBypassed<3>())
            mag *= leftChain.get<ChainPositions::LowCut>().get<3>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<0>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<0>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<1>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<1>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<2>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<2>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(!leftChain.get<ChainPositions::HighCut>().isBypassed<3>())
            mag *= leftChain.get<ChainPositions::HighCut>().get<3>()
            .coefficients->getMagnitudeForFrequency(freq, sampleRate);

        return mag;
    }

    void updateCoefficients()
    {
        // state.params.sampleRate = getSampleRate();
        auto peak1Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(44100,
            peak1FilterParams.filterFreq->getCurrentValue(),
            peak1FilterParams.filterQ->getCurrentValue(),
            juce::Decibels::decibelsToGain(peak1FilterParams.filterGain->getCurrentValue()));
        auto peak2Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(44100,
            peak2FilterParams.filterFreq->getCurrentValue(),
            peak2FilterParams.filterQ->getCurrentValue(),
            juce::Decibels::decibelsToGain(peak2FilterParams.filterGain->getCurrentValue()));
        auto peak3Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(44100,
            peak3FilterParams.filterFreq->getCurrentValue(),
            peak3FilterParams.filterQ->getCurrentValue(),
            juce::Decibels::decibelsToGain(peak3FilterParams.filterGain->getCurrentValue()));

        *leftChain.get<ChainPositions::Peak1>().coefficients = *peak1Coefficients;
        *leftChain.get<ChainPositions::Peak2>().coefficients = *peak2Coefficients;
        *leftChain.get<ChainPositions::Peak3>().coefficients = *peak3Coefficients;
        *rightChain.get<ChainPositions::Peak1>().coefficients = *peak1Coefficients;
        *rightChain.get<ChainPositions::Peak2>().coefficients = *peak2Coefficients;
        *rightChain.get<ChainPositions::Peak3>().coefficients = *peak3Coefficients;

        leftChain.setBypassed<ChainPositions::Peak1>(!peak1FilterParams.filterActive->get());
        leftChain.setBypassed<ChainPositions::Peak2>(!peak2FilterParams.filterActive->get());
        leftChain.setBypassed<ChainPositions::Peak3>(!peak3FilterParams.filterActive->get());
        rightChain.setBypassed<ChainPositions::Peak1>(!peak1FilterParams.filterActive->get());
        rightChain.setBypassed<ChainPositions::Peak2>(!peak2FilterParams.filterActive->get());
        rightChain.setBypassed<ChainPositions::Peak3>(!peak3FilterParams.filterActive->get());

        // calculate and set low cut filter coefficients
        int lowCutSlope = loCutFilterParams.filterSlope->getCurrentValue();
        auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
            loCutFilterParams.filterFreq->getCurrentValue(),
            44100,
            lowCutSlope / 6);

        auto& leftLowCutChain = leftChain.get<ChainPositions::LowCut>();
        leftLowCutChain.setBypassed<Slope::S12>(true);
        leftLowCutChain.setBypassed<Slope::S24>(true);
        leftLowCutChain.setBypassed<Slope::S36>(true);
        leftLowCutChain.setBypassed<Slope::S48>(true);
        auto& rightLowCutChain = rightChain.get<ChainPositions::LowCut>();
        rightLowCutChain.setBypassed<Slope::S12>(true);
        rightLowCutChain.setBypassed<Slope::S24>(true);
        rightLowCutChain.setBypassed<Slope::S36>(true);
        rightLowCutChain.setBypassed<Slope::S48>(true);

        // breaks omitted on purpose to facilitate fall through
        switch(lowCutSlope) {
            case 48: {
                *leftLowCutChain.get<Slope::S48>().coefficients =
                *lowCutCoefficients[Slope::S48];
                leftLowCutChain.setBypassed<Slope::S48>(!loCutFilterParams.filterActive->get());
                *rightLowCutChain.get<Slope::S48>().coefficients =
                *lowCutCoefficients[Slope::S48];
                rightLowCutChain.setBypassed<Slope::S48>(!loCutFilterParams.filterActive->get());
            }
            case 36: {
                *leftLowCutChain.get<Slope::S36>().coefficients =
                *lowCutCoefficients[Slope::S36];
                leftLowCutChain.setBypassed<Slope::S36>(!loCutFilterParams.filterActive->get());
                *rightLowCutChain.get<Slope::S36>().coefficients =
                *lowCutCoefficients[Slope::S36];
                rightLowCutChain.setBypassed<Slope::S36>(!loCutFilterParams.filterActive->get());
            }
            case 24: {
                *leftLowCutChain.get<Slope::S24>().coefficients =
                *lowCutCoefficients[Slope::S24];
                leftLowCutChain.setBypassed<Slope::S24>(!loCutFilterParams.filterActive->get());
                *rightLowCutChain.get<Slope::S24>().coefficients =
                *lowCutCoefficients[Slope::S24];
                rightLowCutChain.setBypassed<Slope::S24>(!loCutFilterParams.filterActive->get());
            }
            case 12: {
                *leftLowCutChain.get<Slope::S12>().coefficients =
                *lowCutCoefficients[Slope::S12];
                leftLowCutChain.setBypassed<Slope::S12>(!loCutFilterParams.filterActive->get());
                *rightLowCutChain.get<Slope::S12>().coefficients =
                *lowCutCoefficients[Slope::S12];
                rightLowCutChain.setBypassed<Slope::S12>(!loCutFilterParams.filterActive->get());
            }
        }

        // calculate and set high cut filter coefficients
        int highCutSlope = hiCutFilterParams.filterSlope->getCurrentValue();
        auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            hiCutFilterParams.filterFreq->getCurrentValue(),
            44100,
            highCutSlope / 6);

        auto& leftHighCutChain = leftChain.get<ChainPositions::HighCut>();
        leftHighCutChain.setBypassed<Slope::S12>(true);
        leftHighCutChain.setBypassed<Slope::S24>(true);
        leftHighCutChain.setBypassed<Slope::S36>(true);
        leftHighCutChain.setBypassed<Slope::S48>(true);
        auto& rightHighCutChain = rightChain.get<ChainPositions::HighCut>();
        rightHighCutChain.setBypassed<Slope::S12>(true);
        rightHighCutChain.setBypassed<Slope::S24>(true);
        rightHighCutChain.setBypassed<Slope::S36>(true);
        rightHighCutChain.setBypassed<Slope::S48>(true);

        // breaks omitted on purpose to facilitate fall through
        switch(highCutSlope) {
            case 48: {
                *leftHighCutChain.get<Slope::S48>().coefficients = *highCutCoefficients[Slope::S48];
                leftHighCutChain.setBypassed<Slope::S48>(!hiCutFilterParams.filterActive->get());
                *rightHighCutChain.get<Slope::S48>().coefficients = *highCutCoefficients[Slope::S48];
                rightHighCutChain.setBypassed<Slope::S48>(!hiCutFilterParams.filterActive->get());
            }
            case 36: {
                *leftHighCutChain.get<Slope::S36>().coefficients = *highCutCoefficients[Slope::S36];
                leftHighCutChain.setBypassed<Slope::S36>(!hiCutFilterParams.filterActive->get());
                *rightHighCutChain.get<Slope::S36>().coefficients = *highCutCoefficients[Slope::S36];
                rightHighCutChain.setBypassed<Slope::S36>(!hiCutFilterParams.filterActive->get());
            }
            case 24: {
                *leftHighCutChain.get<Slope::S24>().coefficients = *highCutCoefficients[Slope::S24];
                leftHighCutChain.setBypassed<Slope::S24>(!hiCutFilterParams.filterActive->get());
                *rightHighCutChain.get<Slope::S24>().coefficients = *highCutCoefficients[Slope::S24];
                rightHighCutChain.setBypassed<Slope::S24>(!hiCutFilterParams.filterActive->get());
            }
            case 12: {
                *leftHighCutChain.get<Slope::S12>().coefficients = *highCutCoefficients[Slope::S12];
                leftHighCutChain.setBypassed<Slope::S12>(!hiCutFilterParams.filterActive->get());
                *rightHighCutChain.get<Slope::S12>().coefficients = *highCutCoefficients[Slope::S12];
                rightHighCutChain.setBypassed<Slope::S12>(!hiCutFilterParams.filterActive->get());
            }
        }
        // DBG(leftHighCutChain.get<Slope::S12>().coefficients->getMagnitudeForFrequency(1000, 48000));
    }
    enum Slope {
        S12,
        S24,
        S36,
        S48
    };
    enum ChainPositions {
        LowCut,
        Peak1,
        Peak2,
        Peak3,
        HighCut
    };
};

/****************************************************************************************/

struct EQNonParameterState : chowdsp::NonParamState
{
    EQNonParameterState() {}
};

// ****************************** EQProcessor ****************************************** //
// ********************************************************************************************* //

class EQProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<EQParams, EQNonParameterState>>,
                        public juce::ValueTree::Listener
{
public:
    EQProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~EQProcessor()
    {
        parent.getValueTree().removeListener(this);
    }

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<EQProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    // void setupModulationMappings();

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override{};

    // Called upon initializiation and whenever a user changes parameters with a slider
    // void updateCoefficients();
    // Return the magnitude corresponding to this frequency based off the current parameters of this equalizer
    // double magForFreq(double freq);


    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties eqBusLayout()
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
             /**
              * todo: check the number of discrete channels to match needs here
              */
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (10), true) // Mod inputs; numChannels for the number of mods we want to enable
            .withOutput("Modulation", juce::AudioChannelSet::mono(),false)  // Modulation send channel; disabled for all but Modulation preps!
            .withOutput("Send",juce::AudioChannelSet::stereo(),true);       // Send channel (right outputs)
    }
    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    void resetToDefaults()
    {
        state.params.activeEq->setParameterValue(false);
        state.params.loCutFilterParams.resetToDefault();
        state.params.peak1FilterParams.resetToDefault();
        state.params.peak2FilterParams.resetToDefault();
        state.params.peak3FilterParams.resetToDefault();
        state.params.hiCutFilterParams.resetToDefault();
        state.params.resetEq->setParameterValue(false);
    }

private:
    chowdsp::ScopedCallbackList eqCallbacks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQProcessor)
};
