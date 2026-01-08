//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "Identifiers.h"
#include "PluginBase.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_sources/chowdsp_sources.h>

enum CompressorPresetComboBox {
    Default = 1 << 0,
    Piano = 1 << 1,
    Piano_2 = 1 << 2,
    Brick_Wall = 1 << 3,
    Aggressive = 1 << 4
};

// ********************************************************************************************* //
// ****************************  CompressorParams  ********************************************* //
// ********************************************************************************************* //

struct CompressorParams : chowdsp::ParamHolder
{
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;
    std::atomic<float> maxGainReduction = 0.0f;

    // Adds the appropriate parameters to the Nostalgic Processor
    CompressorParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("compressor")
    {
        add(activeCompressor,
            resetCompressor,
            inputGain,
            outputSend,
            outputGain,
            attack,
            release,
            threshold,
            makeup,
            // mix,
            ratio,
            knee,
            presets);
    }

    // active bool
    chowdsp::BoolParameter::Ptr activeCompressor {
        juce::ParameterID { "activeCompressor", 100 },
        "activeCompressor",
        false
    };

    // reset bool
    chowdsp::BoolParameter::Ptr resetCompressor {
        juce::ParameterID { "resetCompressor", 100},
        "resetCompressor",
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

    /*
     * for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float>> outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> inputLevels;

    // attack 0 to 100.00ms, center 50, default 0
    chowdsp::TimeMsParameter::Ptr attack //ms
    {
        juce::ParameterID{"compAttack",100},
        "compAttack",
        juce::NormalisableRange{0.00f,100.00f,0.01f,1.f,false},
        5.f
    };

    // release 5 to 1500.00ms, center 750, default 5
    chowdsp::TimeMsParameter::Ptr release //ms
    {
        juce::ParameterID{"compRelease",100},
        "compRelease",
        juce::NormalisableRange{5.00f,1500.00f,0.01f,1.f,false},
        20.00f
    };

    // threshold -60.0 to 0 db, center -30, default 0
    chowdsp::GainDBParameter::Ptr threshold {
        juce::ParameterID { "compThreshold", 100 },
        "compThreshold",
        juce::NormalisableRange { -60.0f, 0.0f, 0.1f, 1.f, false },
        0.0f,
        true
    };

    // makeup -40.0db to 40.0db, center 0, default 0
    chowdsp::GainDBParameter::Ptr makeup {
        juce::ParameterID { "compMakeup", 100 },
        "compMakeup",
        juce::NormalisableRange { -40.0f, 40.0f, 0.1f, 1.f, false },
        0.0f,
        true
    };

    // mix 0 to 1.000, center 0.500, default 1
    // chowdsp::FloatParameter::Ptr mix {
    //     juce::ParameterID { "compMix", 100 },
    //     "compMix",
    //     chowdsp::ParamUtils::createNormalisableRange (0.000f, 1.000f, 0.500f, 0.001f),
    //     1.000f,
    //     &chowdsp::ParamUtils::floatValToString,
    //     &chowdsp::ParamUtils::stringToFloatVal,
    //     true
    // };

    // ratio 1.00 to 24.00, center 12.00, default 1
    chowdsp::FloatParameter::Ptr ratio {
        juce::ParameterID { "compRatio", 100 },
        "compRatio",
        chowdsp::ParamUtils::createNormalisableRange (1.00f, 24.00f, 12.00f, 0.01f),
        1.00f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // knee 0 to 24.0, center 12.0, default 0
    chowdsp::FloatParameter::Ptr knee {
        juce::ParameterID { "compKnee", 100 },
        "compKnee",
        chowdsp::ParamUtils::createNormalisableRange (0.f, 24.f, 12.f, 0.1f),
        0.f,
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal,
        true
    };

    // combo box
    chowdsp::EnumChoiceParameter<CompressorPresetComboBox>::Ptr presets {
        juce::ParameterID { "compressorPresets", 100 },
        "compressorPresets",
        CompressorPresetComboBox::Default,
        std::initializer_list<std::pair<char, char>> { { '_', ' ' } }
    };
};

/****************************************************************************************/

struct CompressorNonParameterState : chowdsp::NonParamState
{
    CompressorNonParameterState() {}
};

// ****************************** CompressorProcessor ****************************************** //
// ********************************************************************************************* //

class CompressorProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<CompressorParams, CompressorNonParameterState>>,
                        public juce::ValueTree::Listener
{
public:
    CompressorProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~CompressorProcessor()
    {
        parent.getValueTree().removeListener(this);
        rawSidechainSignal = nullptr;
        gainReduction.set(0.0f);
        currentInput.set(-std::numeric_limits<float>::infinity());
        currentOutput.set(-std::numeric_limits<float>::infinity());
    }

    static std::unique_ptr<juce::AudioProcessor> create (SynthBase& parent, const juce::ValueTree& v)
    {
        return std::make_unique<CompressorProcessor> (parent, v);
    }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    // void setupModulationMappings();

    void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override{};


    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties compressorBusLayout()
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
    void processCrestFactor(const float* src, const int numSamples);
    float applyCompression(float& input);
    void applyCompressionToBuffer(float* src, int numSamples);
    void applyBallistics(float* src, int numSamples);
    float processPeakBranched(const float& in);

private:
    juce::AudioBuffer<float> originalSignal;
    std::vector<float> sidechainSignal;
    float* rawSidechainSignal{nullptr};

    // crest factor
    double attackTimeInSeconds{0.0}, releaseTimeInSeconds{0.14};
    double avgAttackTime{0.0}, avgReleaseTime{0.14};
    double peakState{0.0};
    double rmsState{0.0};
    double a1{0.0}, b1{0.0};
    double sampleRate{0.0};
    double maxAttackTime{0.08}, maxReleaseTime{1.0}; //respective 8ms and 1sec
    double cFactor{0.0};

    double state01{0.0}, state02{0.0};
    double alphaAttack{0.0};
    double alphaRelease{0.0};

    float slope;
    float kneeHalf;
    float overshoot;

    juce::Atomic<float> gainReduction;
    juce::Atomic<float> currentInput;
    juce::Atomic<float> currentOutput;

    chowdsp::ScopedCallbackList compressorCallbacks;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorProcessor)
};