//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "Identifiers.h"
#include "PluginBase.h"
#include "Synthesiser/BKSynthesiser.h"
#include "utils.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <chowdsp_sources/chowdsp_sources.h>

// ********************************************************************************************* //
// ****************************  CompressorParams  ********************************************* //
// ********************************************************************************************* //

struct CompressorParams : chowdsp::ParamHolder
{
    // Adds the appropriate parameters to the Nostalgic Processor
    CompressorParams(const juce::ValueTree& v) : chowdsp::ParamHolder ("eq")
    {
            // add();
    }
};

/****************************************************************************************/

struct CompressorNonParameterState : chowdsp::NonParamState
{
    CompressorNonParameterState() {}
};

// ****************************** CompressorProcessor ****************************************** //
// ********************************************************************************************* //

class CompressorProcessor : public bitklavier::PluginBase<bitklavier::PreparationStateImpl<CompressorParams, CompressorNonParameterState>>,
                        public juce::ValueTree::Listener, public TuningListener
{
public:
    CompressorProcessor (SynthBase& parent, const juce::ValueTree& v);
    ~CompressorProcessor()
    {
        parent.getValueTree().removeListener(this);
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
    // void ProcessMIDIBlock(juce::MidiBuffer& inMidiMessages, juce::MidiBuffer& outMidiMessages, int numSamples);
    // void processContinuousModulations(juce::AudioBuffer<float>& buffer);
    // void updateMidiNoteTranspositions(int noteOnNumber);
    // void updateAllMidiNoteTranspositions();
    // void handleMidiTargetMessages(int channel);
    // bool acceptsMidi() const override { return true; }
    // void addSoundSet (std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s)
    // {
    //     ptrToSamples = s;
    // }

    // void addSoundSet (
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s, // main samples
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h, // hammer samples
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r, // release samples
    //     juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) // pedal samples
    // {
    //     nostalgicSynth->addSoundSet (s);
    // }

    // void setSynchronic (SynchronicProcessor*) override;
    // void setTuning (TuningProcessor*) override;
    void tuningStateInvalidated() override{};

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

    void valueTreePropertyChanged(juce::ValueTree &t, const juce::Identifier &property) {

        if (t == v && property == IDs::soundset) {
            loadSamples();
            return;
        }
        if (!v.getProperty(IDs::soundset).equals(IDs::syncglobal.toString()))
            return;
        if (property == IDs::soundset && t == parent.getValueTree()) {
            juce::String a = t.getProperty(IDs::soundset, "");
            addSoundSet(&(*parent.getSamples())[a],
                        &(*parent.getSamples())[a+"Hammers"],
                        &(*parent.getSamples())[a+"ReleaseResonance"],
                        &(*parent.getSamples())[a+"Pedals"]);
        }
    }

    void loadSamples() {
        juce::String soundset = v.getProperty(IDs::soundset, IDs::syncglobal.toString());
        if (soundset == IDs::syncglobal.toString()) {
            //if global sync read soundset from global valuetree
            soundset = parent.getValueTree().getProperty(IDs::soundset, "");

            addSoundSet(&(*parent.getSamples())[soundset],
                     &(*parent.getSamples())[soundset + "Hammers"],
                     &(*parent.getSamples())[soundset + "ReleaseResonance"],
                     &(*parent.getSamples())[soundset + "Pedals"]);
        }else {
            //otherwise set the piano
            addSoundSet(&(*parent.getSamples())[soundset],
                        &(*parent.getSamples())[soundset + "Hammers"],
                        &(*parent.getSamples())[soundset + "ReleaseResonance"],
                        &(*parent.getSamples())[soundset + "Pedals"]);
        }
    }

private:
    std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* ptrToSamples;
    BKSynthesizerState lastSynthState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorProcessor)
};