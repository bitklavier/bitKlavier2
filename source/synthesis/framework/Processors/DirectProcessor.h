//
// Created by Davis Polito on 5/2/24.
//

#pragma once

#include "EnvParams.h"
#include "Identifiers.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "TransposeParams.h"
#include "TuningProcessor.h"
#include "buffer_debugger.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_sources/chowdsp_sources.h>
#include "SampleLoadManager.h"
#include "target_types.h"

struct DirectParams : chowdsp::ParamHolder {
    // gain slider params, for all gain-type knobs
    float rangeStart = -80.0f;
    float rangeEnd = 6.0f;
    float skewFactor = 2.0f;

    // Adds the appropriate parameters to the Direct Processor
    DirectParams(const juce::ValueTree &vt) : chowdsp::ParamHolder("direct"), v(vt)
    {
        add(gainParam,
            hammerParam,
            releaseResonanceParam,
            pedalParam,
            outputSendParam,
            outputGain,
            env,
            transpose,
            resonanceLoaded,
            hammerLoaded,
            pedalLoaded);

        // params that are audio-rate modulatable are added to vector of all continuously modulatable params
        // used in the DirectProcessor constructor
        doForAllParameters([this](auto &param, size_t)
        {
            // if (auto *sliderParam = dynamic_cast<chowdsp::ChoiceParameter *>(&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.push_back(sliderParam);
            //
            // if (auto *sliderParam = dynamic_cast<chowdsp::BoolParameter *>(&param))
            //     if (sliderParam->supportsMonophonicModulation())
            //         modulatableParams.push_back(sliderParam);

            if (auto *sliderParam = dynamic_cast<chowdsp::FloatParameter *>(&param))
                if (sliderParam->supportsMonophonicModulation())
                    modulatableParams.push_back(sliderParam);
        });
    }

    juce::ValueTree v;
    /**
     * these first four GainDBParameters are passed to each of the
     * BKSynths, and affect the noteOn gains, so only have an
     * impact when the noteOn messages are called (they are not
     * continuous, like the outputGain and outputSend params)
     */
    // Gain param
    chowdsp::GainDBParameter::Ptr gainParam{
        juce::ParameterID{"Main", 100},
        "PRIMARY PIANO SOUND",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true, // true => audio rate modulatable (continuously),
        //v.getChildWithProperty("parameter", "Main")
    };

    // Hammer param
    chowdsp::GainDBParameter::Ptr hammerParam{
        juce::ParameterID{"Hammers", 100},
        "RELEASE HAMMERS",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        -24.0f,
        true,
        //v.getChildWithProperty("parameter", "Hammers")
    };

    // Resonance param
    chowdsp::GainDBParameter::Ptr releaseResonanceParam{
        juce::ParameterID{"Resonance", 100},
        "RELEASE RESONANCE",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true,
        //v.getChildWithProperty("parameter", "Resonance")
    };

    // Pedal param
    chowdsp::GainDBParameter::Ptr pedalParam{
        juce::ParameterID{"Pedal", 100},
        "PEDAL",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        -6.0f,
        true,
        //v.getChildWithProperty("parameter", "Pedal")
    };

    // Gain for output send (for blendronic, VSTs, etc...)
    chowdsp::GainDBParameter::Ptr outputSendParam{
        juce::ParameterID{"Send", 100},
        "Send",
        juce::NormalisableRange{rangeStart, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true,
        v.getChildWithProperty("parameter", "Send")
    };

    // for the output gain slider, final gain stage for this prep (meter slider on right side of prep)
    chowdsp::GainDBParameter::Ptr outputGain{
        juce::ParameterID{"OutputGain", 100},
        "Output Gain",
        juce::NormalisableRange{-80.0f, rangeEnd, 0.0f, skewFactor, false},
        0.0f,
        true,
        v.getChildWithProperty("parameter", "OutputGain")
    };

    chowdsp::BoolParameter::Ptr hammerLoaded{ juce::ParameterID{"hammerLoaded", 100}, "Hammer Loaded", false};
    chowdsp::BoolParameter::Ptr resonanceLoaded{ juce::ParameterID{"resonanceLoaded", 100}, "Release Loaded", true};
    chowdsp::BoolParameter::Ptr pedalLoaded{ juce::ParameterID{"pedalLoaded", 100}, "Pedal Loaded", true};


    // ADSR params
    EnvParams env;

    // Transposition slider (holds up to 12 transposition values)
    TransposeParams transpose;

    /** for storing outputLevels of this preparation for display
     *  because we are using an OpenGL slider for the level meter, we don't use the chowdsp params for this
     *      we simply update this in the processBlock() call
     *      and then the level meter will update its values during the OpenGL cycle
     */
    std::tuple<std::atomic<float>, std::atomic<float> > outputLevels;
    std::tuple<std::atomic<float>, std::atomic<float>> sendLevels;

    /****************************************************************************************/
};

struct DirectNonParameterState : chowdsp::NonParamState {
    DirectNonParameterState() {
    }
};


// ********************************************************************************************* //
// ************************************** DirectProcessor ************************************** //
// ********************************************************************************************* //

class DirectProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<DirectParams,
                            DirectNonParameterState> >,
                        public juce::ValueTree::Listener,public TuningListener {
public:
    DirectProcessor(SynthBase &parent, const juce::ValueTree &v, juce::UndoManager* );
    ~DirectProcessor() {
        /*
         * both of these need to be called in the destructor
         * - the first is regarding sample library choice for this preparation
         * - the second is for any attached Tuning prep
         */
        parent.getValueTree().removeListener(this);
        if(tuning !=nullptr) tuning->removeListener(this);
    }

    void tuningStateInvalidated() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override {
    }

    void processAudioBlock(juce::AudioBuffer<float> &buffer) override {
    };

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;
    void processBlockBypassed(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;
    bool acceptsMidi() const override { return true; }

    void addSoundSet(
        juce::ReferenceCountedArray<BKSynthesiserSound > *s, // main samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *h, // hammer samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *r, // release samples
        juce::ReferenceCountedArray<BKSynthesiserSound > *p) // pedal samples
    override {
        mainSynth->addSoundSet(s);
        state.params.hammerLoaded->setParameterValue( h != nullptr);
        hammerSynth->addSoundSet(h);
        state.params.resonanceLoaded->setParameterValue( r != nullptr);
        releaseResonanceSynth->addSoundSet(r);
        state.params.pedalLoaded->setParameterValue( p != nullptr);
        pedalSynth->addSoundSet(p);
    }

    void setA4Frequency(double newA4)
    {
        mainSynth->setA4Frequency(newA4);
        releaseResonanceSynth->setA4Frequency(newA4);
    }

    void setTuning(TuningProcessor *) override;

    /*
     * this is where we define the buses for audio in/out, including the param modulation channels
     *      the "discreteChannels" number is currently just by hand set based on the max that this particularly preparation could have
     *      so if you add new params, might need to increase that number
     */
    juce::AudioProcessor::BusesProperties directBusLayout() {
        return BusesProperties()
                .withOutput("Output", juce::AudioChannelSet::stereo(), true) // Main Output
                .withInput("Input", juce::AudioChannelSet::stereo(), false) // Main Input (not used here)

                /**
                 * IMPORTANT: set discreteChannels below equal to the number of params you want to continuously modulate!!
                 *              for direct, we have 10:
                 *                  - the ADSR params: attackParam, decayParam, sustainParam, releaseParam,
                 *                  - the main params: gainParam, hammerParam, releaseResonanceParam, pedalParam, OutputSendParam, outputGain,
                 *            also, needs to be *2, since ramp mods and LFO mods need separate channels
                 *
                 * todo: work out a way to set this number of channels automatically, perhaps using doForAllParameters(),
                 *          or reading through modulatableParams and counting float params (check if true for last arg?)
                 */
                    // todo: why is this 11 * 2 and not 10 * 2?
                .withInput("Modulation", juce::AudioChannelSet::discreteChannels(10 * 2), true)
                // Mod inputs; numChannels for the number of mods we want to enable
                .withOutput("Modulation", juce::AudioChannelSet::mono(), false)
                // Modulation send channel; disabled for all but Modulation preps!
                .withOutput("Send", juce::AudioChannelSet::stereo(), true); // Send channel (right outputs)
    }

    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const override;

    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor *createEditor() override { return nullptr; }

    /**
     * todo: i think this is not used? remove if so
     * @param vt
     */
    //    void addToVT (juce::ValueTree& vt)
    //    {
    //        state.params.doForAllParameters ([this, &vt] (auto& param, size_t) {
    //            vt.setProperty (param.paramID, chowdsp::ParameterTypeHelpers::getValue (param), nullptr);
    //        });
    //    }

    /**
     * todo: question for Davis: is this used still? if so, what for?
     * DAVIS: It is used to respond to soundset changes. i.e. you change the valuetree
     * and it triggers the samples to be swapped out
     * @param t
     */
    void valueTreePropertyChanged(juce::ValueTree &t, const juce::Identifier &property) {

        if (t == v && property == IDs::soundset) {
            loadSamples();
            return;
        }
        if (!v.getProperty(IDs::soundset).equals(IDs::syncglobal.toString()))
            return;
        if (property == IDs::soundset && t == parent.getValueTree() ) {
            juce::String soundset = t.getProperty(IDs::soundset, "");
            auto* samples = parent.getSamples();

            addSoundSet(
                samples->contains(soundset) ? (*samples)[soundset] : nullptr,
                samples->contains(soundset + "Hammers") ? (*samples)[soundset + "Hammers"] : nullptr,
                samples->contains(soundset + "ReleaseResonance") ? (*samples)[soundset + "ReleaseResonance"] : nullptr,
                samples->contains(soundset + "Pedals") ? (*samples)[soundset + "Pedals"] : nullptr
            );
        }
    }

    void loadSamples() {
        juce::String soundset = v.getProperty(IDs::soundset, IDs::syncglobal.toString());
        if (soundset == IDs::syncglobal.toString()) {
            //if global sync read soundset from global valuetree
            soundset = parent.getValueTree().getProperty(IDs::soundset, "");

            auto* samples = parent.getSamples();

            addSoundSet(
                samples->contains(soundset) ? (*samples)[soundset] : nullptr,
                samples->contains(soundset + "Hammers") ? (*samples)[soundset + "Hammers"] : nullptr,
                samples->contains(soundset + "ReleaseResonance") ? (*samples)[soundset + "ReleaseResonance"] : nullptr,
                samples->contains(soundset + "Pedals") ? (*samples)[soundset + "Pedals"] : nullptr
            );
        }else {
            //otherwise set the piano
            auto* samples = parent.getSamples();

            addSoundSet(
                samples->contains(soundset) ? (*samples)[soundset] : nullptr,
                samples->contains(soundset + "Hammers") ? (*samples)[soundset + "Hammers"] : nullptr,
                samples->contains(soundset + "ReleaseResonance") ? (*samples)[soundset + "ReleaseResonance"] : nullptr,
                samples->contains(soundset + "Pedals") ? (*samples)[soundset + "Pedals"] : nullptr
            );
        }
    }
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
    juce::ScopedPointer<BufferDebugger> bufferDebugger;
    std::unique_ptr<BKSynthesiser> mainSynth;
    std::unique_ptr<BKSynthesiser> hammerSynth;
    std::unique_ptr<BKSynthesiser> releaseResonanceSynth;
    std::unique_ptr<BKSynthesiser> pedalSynth;

    /*
     * array of transpositions associated with a single noteOn msg
     */
    juce::Array<float> midiNoteTranspositions;
    void updateMidiNoteTranspositions(int noteOnNumber);
    void updateAllMidiNoteTranspositions();
    void handleMidiTargetMessages(juce::MidiBuffer& midiMessages);

    /*
     * noteOnSpecMap
     * - key      => midiNoteNumber
     * - value    => specs for that key (start time, direction, loop mode, transpositions)
     *
     * needed here for transpositions
     */
    std::array<NoteOnSpec, MaxMidiNotes> noteOnSpecMap;

    /*
     * see addSoundSet() for usage of ptrToSamples
     */
    std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader> > > *ptrToSamples;
    BKSynthesizerState lastSynthState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectProcessor)
};
