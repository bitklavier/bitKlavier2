// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "Identifiers.h"
#include "IMuteSolable.h"
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
    EqCustom = 1 << 3,
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
                inputGain,
                externalGain,
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

    // To adjust the gain of signals coming in to the EQ (internal audio from graph)
    chowdsp::GainDBParameter::Ptr inputGain {
        juce::ParameterID { "InputGain", 100 },
        "Input Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        0.0f,
        true
   };

    // Gain for external (mic/line/sidechain) input
    chowdsp::GainDBParameter::Ptr externalGain {
        juce::ParameterID { "externalGain", 100 },
        "External Gain",
        juce::NormalisableRange { rangeStart, rangeEnd, 0.0f, skewFactor, false },
        rangeStart,
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
    std::tuple<std::atomic<float>, std::atomic<float>> externalLevels;
    std::atomic<bool> muted_ { false };
    std::atomic<bool> userMuted_ { false };
    std::atomic<bool> soloed_ { false };
    std::atomic<bool> soloMuted_ { false };

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

    // Set by message-thread listeners; checked by the audio thread in processBlock.
    // Used for BusEQ which has no continuous modulations.
    std::atomic<bool> needsCoeffUpdate { true };

    // Pre-allocated coefficient objects, wired into the filter chains by prepareCoefficients().
    // updateCoefficients() writes into these in-place — no heap activity on the audio thread.
    juce::dsp::IIR::Coefficients<float>::Ptr peak1Coeffs, peak2Coeffs, peak3Coeffs;
    juce::dsp::IIR::Coefficients<float>::Ptr loCutCoeffs[4], hiCutCoeffs[4];

    // Computes magnitude entirely from parameter values — safe to call from any thread
    // without touching the audio-thread-owned filter chain objects.
    double magForFreq(double freq) {
        double mag = 1.0;

        if (peak1FilterParams.filterActive->get()) {
            auto c = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                peak1FilterParams.filterFreq->getCurrentValue(),
                peak1FilterParams.filterQ->getCurrentValue(),
                juce::Decibels::decibelsToGain(peak1FilterParams.filterGain->getCurrentValue()));
            mag *= c->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (peak2FilterParams.filterActive->get()) {
            auto c = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                peak2FilterParams.filterFreq->getCurrentValue(),
                peak2FilterParams.filterQ->getCurrentValue(),
                juce::Decibels::decibelsToGain(peak2FilterParams.filterGain->getCurrentValue()));
            mag *= c->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (peak3FilterParams.filterActive->get()) {
            auto c = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                peak3FilterParams.filterFreq->getCurrentValue(),
                peak3FilterParams.filterQ->getCurrentValue(),
                juce::Decibels::decibelsToGain(peak3FilterParams.filterGain->getCurrentValue()));
            mag *= c->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (loCutFilterParams.filterActive->get()) {
            int order = (int)loCutFilterParams.filterSlope->getCurrentValue() / 6;
            auto cs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
                loCutFilterParams.filterFreq->getCurrentValue(), sampleRate, order);
            for (int i = 0; i < cs.size(); ++i)
                mag *= cs[i]->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (hiCutFilterParams.filterActive->get()) {
            int order = (int)hiCutFilterParams.filterSlope->getCurrentValue() / 6;
            auto cs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
                hiCutFilterParams.filterFreq->getCurrentValue(), sampleRate, order);
            for (int i = 0; i < cs.size(); ++i)
                mag *= cs[i]->getMagnitudeForFrequency(freq, sampleRate);
        }

        return mag;
    }

    // Writes peak-filter biquad coefficients in-place. Matches JUCE's makePeakFilter formula.
    static void fillPeakCoeffs (float* c, float freq, float Q, float gainFactor, double sr) noexcept
    {
        double A       = std::sqrt ((double) gainFactor);
        double w0      = juce::MathConstants<double>::twoPi * freq / sr;
        double alpha   = std::sin (w0) / (2.0 * Q);
        double cos_w0  = std::cos (w0);
        double aTimesA = alpha * A;
        double aOverA  = alpha / A;
        double a0      = 1.0 + aOverA;
        c[0] = (float) ((1.0 + aTimesA) / a0);
        c[1] = (float) ((-2.0 * cos_w0) / a0);
        c[2] = (float) ((1.0 - aTimesA) / a0);
        c[3] = (float) ((-2.0 * cos_w0) / a0);   // a1/a0
        c[4] = (float) ((1.0 - aOverA)  / a0);   // a2/a0
    }

    // Writes one biquad stage of a Butterworth highpass cascade in-place.
    // Uses JUCE's pole formula: B = -2*cos(π*(2*i + N - 1)/(2*N)), i = stageIndex+1.
    static void fillHighpassButterworthStage (float* c, float freq, double sr, int totalOrder, int stageIndex) noexcept
    {
        double B      = -2.0 * std::cos (juce::MathConstants<double>::pi
                                         * (2.0 * (stageIndex + 1) + totalOrder - 1)
                                         / (2.0 * totalOrder));
        double omega  = std::tan (juce::MathConstants<double>::pi * freq / sr);
        double omega2 = omega * omega;
        double a0     = 1.0 + B * omega + omega2;
        double a1     = 2.0 * (omega2 - 1.0);
        double a2     = 1.0 - B * omega + omega2;
        c[0] = (float) ( 1.0 / a0);
        c[1] = (float) (-2.0 / a0);
        c[2] = (float) ( 1.0 / a0);
        c[3] = (float) (a1   / a0);
        c[4] = (float) (a2   / a0);
    }

    // Writes one biquad stage of a Butterworth lowpass cascade in-place.
    static void fillLowpassButterworthStage (float* c, float freq, double sr, int totalOrder, int stageIndex) noexcept
    {
        double B      = -2.0 * std::cos (juce::MathConstants<double>::pi
                                         * (2.0 * (stageIndex + 1) + totalOrder - 1)
                                         / (2.0 * totalOrder));
        double omega  = std::tan (juce::MathConstants<double>::pi * freq / sr);
        double omega2 = omega * omega;
        double a0     = 1.0 + B * omega + omega2;
        double a1     = 2.0 * (omega2 - 1.0);
        double a2     = 1.0 - B * omega + omega2;
        c[0] = (float) (omega2        / a0);
        c[1] = (float) (2.0 * omega2  / a0);
        c[2] = (float) (omega2        / a0);
        c[3] = (float) (a1            / a0);
        c[4] = (float) (a2            / a0);
    }

    // Allocates coefficient objects and wires them into the filter chains.
    // Must be called from prepareToPlay (prepare thread) before audio starts.
    // After this, updateCoefficients() writes values in-place without any heap activity.
    void prepareCoefficients (double sr)
    {
        sampleRate = sr;
        auto identity = [] { return juce::dsp::IIR::Coefficients<float>::Ptr (
            new juce::dsp::IIR::Coefficients<float> (1, 0, 0, 1, 0, 0)); };

        peak1Coeffs = identity();
        peak2Coeffs = identity();
        peak3Coeffs = identity();
        leftChain.get<ChainPositions::Peak1>().coefficients  = peak1Coeffs;
        rightChain.get<ChainPositions::Peak1>().coefficients = peak1Coeffs;
        leftChain.get<ChainPositions::Peak2>().coefficients  = peak2Coeffs;
        rightChain.get<ChainPositions::Peak2>().coefficients = peak2Coeffs;
        leftChain.get<ChainPositions::Peak3>().coefficients  = peak3Coeffs;
        rightChain.get<ChainPositions::Peak3>().coefficients = peak3Coeffs;

        for (int i = 0; i < 4; ++i) loCutCoeffs[i] = identity();
        leftChain.get<ChainPositions::LowCut>().get<0>().coefficients  = loCutCoeffs[0];
        leftChain.get<ChainPositions::LowCut>().get<1>().coefficients  = loCutCoeffs[1];
        leftChain.get<ChainPositions::LowCut>().get<2>().coefficients  = loCutCoeffs[2];
        leftChain.get<ChainPositions::LowCut>().get<3>().coefficients  = loCutCoeffs[3];
        rightChain.get<ChainPositions::LowCut>().get<0>().coefficients = loCutCoeffs[0];
        rightChain.get<ChainPositions::LowCut>().get<1>().coefficients = loCutCoeffs[1];
        rightChain.get<ChainPositions::LowCut>().get<2>().coefficients = loCutCoeffs[2];
        rightChain.get<ChainPositions::LowCut>().get<3>().coefficients = loCutCoeffs[3];

        for (int i = 0; i < 4; ++i) hiCutCoeffs[i] = identity();
        leftChain.get<ChainPositions::HighCut>().get<0>().coefficients  = hiCutCoeffs[0];
        leftChain.get<ChainPositions::HighCut>().get<1>().coefficients  = hiCutCoeffs[1];
        leftChain.get<ChainPositions::HighCut>().get<2>().coefficients  = hiCutCoeffs[2];
        leftChain.get<ChainPositions::HighCut>().get<3>().coefficients  = hiCutCoeffs[3];
        rightChain.get<ChainPositions::HighCut>().get<0>().coefficients = hiCutCoeffs[0];
        rightChain.get<ChainPositions::HighCut>().get<1>().coefficients = hiCutCoeffs[1];
        rightChain.get<ChainPositions::HighCut>().get<2>().coefficients = hiCutCoeffs[2];
        rightChain.get<ChainPositions::HighCut>().get<3>().coefficients = hiCutCoeffs[3];
    }

    // Writes current parameter values into the pre-allocated coefficient arrays.
    // No heap allocation — safe to call every audio block.
    void updateCoefficients()
    {
        if (peak1Coeffs == nullptr) return; // not yet prepared

        fillPeakCoeffs (peak1Coeffs->getRawCoefficients(),
            peak1FilterParams.filterFreq->getCurrentValue(),
            peak1FilterParams.filterQ->getCurrentValue(),
            juce::Decibels::decibelsToGain (peak1FilterParams.filterGain->getCurrentValue()),
            sampleRate);
        fillPeakCoeffs (peak2Coeffs->getRawCoefficients(),
            peak2FilterParams.filterFreq->getCurrentValue(),
            peak2FilterParams.filterQ->getCurrentValue(),
            juce::Decibels::decibelsToGain (peak2FilterParams.filterGain->getCurrentValue()),
            sampleRate);
        fillPeakCoeffs (peak3Coeffs->getRawCoefficients(),
            peak3FilterParams.filterFreq->getCurrentValue(),
            peak3FilterParams.filterQ->getCurrentValue(),
            juce::Decibels::decibelsToGain (peak3FilterParams.filterGain->getCurrentValue()),
            sampleRate);

        leftChain.setBypassed<ChainPositions::Peak1>  (!peak1FilterParams.filterActive->get());
        leftChain.setBypassed<ChainPositions::Peak2>  (!peak2FilterParams.filterActive->get());
        leftChain.setBypassed<ChainPositions::Peak3>  (!peak3FilterParams.filterActive->get());
        rightChain.setBypassed<ChainPositions::Peak1> (!peak1FilterParams.filterActive->get());
        rightChain.setBypassed<ChainPositions::Peak2> (!peak2FilterParams.filterActive->get());
        rightChain.setBypassed<ChainPositions::Peak3> (!peak3FilterParams.filterActive->get());

        int loCutOrder  = (int) loCutFilterParams.filterSlope->getCurrentValue() / 6;
        bool loCutOn    = loCutFilterParams.filterActive->get();
        float loCutFreq = loCutFilterParams.filterFreq->getCurrentValue();
        for (int i = 0; i < 4; ++i)
            fillHighpassButterworthStage (loCutCoeffs[i]->getRawCoefficients(), loCutFreq, sampleRate, loCutOrder, i);

        auto& leftLo  = leftChain.get<ChainPositions::LowCut>();
        auto& rightLo = rightChain.get<ChainPositions::LowCut>();
        leftLo.setBypassed<Slope::S12>  (!loCutOn || loCutOrder < 2);
        leftLo.setBypassed<Slope::S24>  (!loCutOn || loCutOrder < 4);
        leftLo.setBypassed<Slope::S36>  (!loCutOn || loCutOrder < 6);
        leftLo.setBypassed<Slope::S48>  (!loCutOn || loCutOrder < 8);
        rightLo.setBypassed<Slope::S12> (!loCutOn || loCutOrder < 2);
        rightLo.setBypassed<Slope::S24> (!loCutOn || loCutOrder < 4);
        rightLo.setBypassed<Slope::S36> (!loCutOn || loCutOrder < 6);
        rightLo.setBypassed<Slope::S48> (!loCutOn || loCutOrder < 8);

        int hiCutOrder  = (int) hiCutFilterParams.filterSlope->getCurrentValue() / 6;
        bool hiCutOn    = hiCutFilterParams.filterActive->get();
        float hiCutFreq = hiCutFilterParams.filterFreq->getCurrentValue();
        for (int i = 0; i < 4; ++i)
            fillLowpassButterworthStage (hiCutCoeffs[i]->getRawCoefficients(), hiCutFreq, sampleRate, hiCutOrder, i);

        auto& leftHi  = leftChain.get<ChainPositions::HighCut>();
        auto& rightHi = rightChain.get<ChainPositions::HighCut>();
        leftHi.setBypassed<Slope::S12>  (!hiCutOn || hiCutOrder < 2);
        leftHi.setBypassed<Slope::S24>  (!hiCutOn || hiCutOrder < 4);
        leftHi.setBypassed<Slope::S36>  (!hiCutOn || hiCutOrder < 6);
        leftHi.setBypassed<Slope::S48>  (!hiCutOn || hiCutOrder < 8);
        rightHi.setBypassed<Slope::S12> (!hiCutOn || hiCutOrder < 2);
        rightHi.setBypassed<Slope::S24> (!hiCutOn || hiCutOrder < 4);
        rightHi.setBypassed<Slope::S36> (!hiCutOn || hiCutOrder < 6);
        rightHi.setBypassed<Slope::S48> (!hiCutOn || hiCutOrder < 8);
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
                    public juce::ValueTree::Listener,
                    public bitklavier::ExternalAudioInputReceiver,
                    public IMuteSolable
{
public:
    EQProcessor (SynthBase& parent, const juce::ValueTree& v, juce::UndoManager*);
    ~EQProcessor()
    {
        parent.getValueTree().removeListener(this);
    }

    std::atomic<bool>& getMuted()    override { return state.params.muted_; }
    std::atomic<bool>& getUserMuted() override { return state.params.userMuted_; }
    std::atomic<bool>& getSoloed()   override { return state.params.soloed_; }
    std::atomic<bool>& getSoloMuted() override { return state.params.soloMuted_; }

    void setExternalInputBuffer (const juce::AudioBuffer<float>* buf) override { externalInputBuffer = buf; }

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
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)    // Main Input (must be enabled to keep Modulation bus off channel 0)
            .withInput ("Send Pad", juce::AudioChannelSet::stereo(), true)  // Padding: absorbs Send output channels so Modulation starts at inputChan >= numOuts (gets read-only zero buffer)

            /**
             * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
             *              for nostalgic, we have 10:
             *                  - the ADSR params: attackParam, decayParam, sustainParam, releaseParam, and
             *                  - the main params: gainParam, hammerParam, releaseResonanceParam, pedalParam, OutputSendParam, outputGain,
             */
             /**
              * todo: check the number of discrete channels to match needs here
              */
            .withInput ("Modulation", juce::AudioChannelSet::discreteChannels (30), true) // Mod inputs; 15 modulatable params × 2 channels (ramp + LFO)
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
    }

private:
    // Timestamp (ms) set the moment the user selects a preset (MessageThread listener).
    // MT Custom-detection listeners skip switching to Custom while < 200ms have elapsed,
    // covering the ~20ms timer delay for preset-driven param changes to propagate.
    juce::int64 presetAppliedAtMs = -1;

    const juce::AudioBuffer<float>* externalInputBuffer = nullptr;

    chowdsp::ScopedCallbackList eqCallbacks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQProcessor)
};
