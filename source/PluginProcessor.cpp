// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "sound_engine.h"
#include "UserPreferences.h"

namespace {
    static juce::File expandUserPath(const juce::String& path)
    {
        if (path.isEmpty())
            return juce::File();

        if (path.startsWith("~/"))
        {
            auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
            return home.getChildFile(path.fromFirstOccurrenceOf("~/", false, false));
        }
        return juce::File(path);
    }
}


//==============================================================================
PluginProcessor::PluginProcessor()
    : juce::AudioProcessor (BusesProperties()
#if !JucePlugin_IsMidiEffect
    #if !JucePlugin_IsSynth
              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    #endif
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
    #if JucePlugin_IsSynth
              .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
    #endif
#endif
      ),
      SynthBase (nullptr)
{
    DBG ("PluginProcessor constructed: " + juce::String::toHexString ((juce::uint64) this));
}

PluginProcessor::~PluginProcessor()
{
    DBG ("PluginProcessor destroyed: " + juce::String::toHexString ((juce::uint64) this));
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
    // Ensure hosts always route MIDI to the plugin
    return true;
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    engine_->prepareToPlay (sampleRate, samplesPerBlock);
    engine_->setInputsOutputs (getMainBusNumInputChannels(),
        getMainBusNumOutputChannels());

    // On first creation (no DAW session to restore), load Basic Piano so the plugin is
    // playable immediately. setStateWasCalled_ is true when the DAW called setStateInformation
    // before prepareToPlay, which is the contract all conforming hosts follow on session restore.
    //
    // The pre-queue check is a fast path; the queued lambda re-checks the same conditions
    // at fire time. This matters because Logic AU has been observed to call prepareToPlay
    // before setStateInformation, in which case this guard would queue a BasicPiano load
    // that then races setStateInformation's saved-gallery restore and clobbers it.
    if (!setStateWasCalled_ && !defaultLoadAttempted_)
    {
        defaultLoadAttempted_ = true;
        juce::MessageManager::callAsync ([this]()
        {
            // Re-check at fire time: if setStateInformation ran in the interim and started
            // a restore (or one is otherwise in flight), defer to it and do nothing.
            if (setStateWasCalled_ || isSamplesLoading() || hasPendingPreset())
                return;

            auto galleries = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                 .getChildFile ("Documents")
                                 .getChildFile ("bitKlavier")
                                 .getChildFile ("galleries");
            juce::File basicPiano = galleries.getChildFile ("Basic Piano.bk2");
            if (!basicPiano.existsAsFile())
                basicPiano = galleries.getChildFile ("BasicPiano.bk2");
            if (!basicPiano.existsAsFile())
                return;

            std::string error;
            // Use the GUI-aware loadFromFile when the editor is already open (VST3 hosts
            // often create the editor before prepareToPlay), so the gallery name label
            // gets updated in the header. Fall back to the base version otherwise —
            // PluginEditor's constructor will set the name from getActiveFile() when it opens.
            if (auto* gui = getGuiInterface())
                gui->loadFromFile (basicPiano, error);
            else
                loadFromFile (basicPiano, error);
        });
    }
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    engine_->releaseResources();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
    #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    // Sidechain input bus (bus 0 for synths) must be stereo or disabled
    #if JucePlugin_IsSynth
    {
        auto sideChain = layouts.getChannelSet (true, 0);
        if (!sideChain.isDisabled() && sideChain != juce::AudioChannelSet::stereo())
            return false;
    }
    #endif

    return true;
#endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (auto* ph = getPlayHead())
    {
        auto position = ph->getPosition();
        if (position.hasValue())
        {
            if (auto tempo = position->getBpm())
                hostTempo = *tempo;
        }
    }

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Route DAW sidechain to Blendronic (and any other ExternalAudioInputReceiver processors).
    // The sidechain bus is input bus 0 for this synth plugin; Logic Pro activates it via the
    // "Side Chain" menu in the channel strip.
    #if JucePlugin_IsSynth
    if (engine_ != nullptr)
    {
        auto* sidechainBus = getBus (true, 0);
        if (sidechainBus != nullptr && sidechainBus->isEnabled())
        {
            auto sideChainBuf = getBusBuffer (buffer, true, 0);
            engine_->setExternalInput (sideChainBuf, sideChainBuf.getNumChannels());
        }
        else
        {
            engine_->setExternalInput (juce::AudioBuffer<float>(), 0);
        }
    }
    #endif

    // Forward host playhead to our internal graph so sub-processors (e.g., Tempo) can query it
    if (engine_ != nullptr)
        engine_->setPlayHead (getPlayHead());

    // bitKlavier processing
    processAudioAndMidi (buffer, midiMessages);
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    DBG ("PluginProcessor::createEditor called for processor: " + juce::String::toHexString ((juce::uint64) this));
    
    if (auto* currentEditor = getActiveEditor())
    {
        DBG ("  WARNING: activeEditor already exists: " + juce::String::toHexString ((juce::uint64) currentEditor));
        return currentEditor;
    }

    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or juce::ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    // juce::ignoreUnused (destData);

    juce::ValueTree state ("bitKlavierPluginState");

    // Per-instance: write THIS plugin's currently-loaded gallery, not the process-wide
    // user_prefs "last_gallery_path" (which is shared via juce::SharedResourcePointer
    // across every plugin instance in the host — Logic with two bitKlavier tracks would
    // otherwise see whichever gallery was loaded last and save the same path into both
    // tracks' state blocks, so both tracks restore to the same gallery).
    const juce::String lastPath = getActiveFile().getFullPathName();
    state.setProperty ("last_gallery_path", lastPath, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    // Add a unique value every time you save; otherwise some DAWS (like Reaper) might ignore this save
    xml->setAttribute("reaper_force_load", (int)juce::Time::getMillisecondCounter());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    // juce::ignoreUnused (data, sizeInBytes);
    DBG("PluginProcessor::setStateInformation");
    setStateWasCalled_ = true; // DAW is restoring state; suppress default-gallery load
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        juce::ValueTree state = juce::ValueTree::fromXml (*xmlState);

        if (state.hasProperty ("last_gallery_path"))
        {
            juce::String lastPath = state.getProperty ("last_gallery_path");
            juce::File candidate = expandUserPath (lastPath);

            if (candidate.existsAsFile())
            {
                std::string error;
                loadFromFile (candidate, error);
            }
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

const juce::CriticalSection& PluginProcessor::getCriticalSection()
{
    return getCallbackLock();
}

void PluginProcessor::pauseProcessing (bool pause)
{
    suspendProcessing (pause);
}

void PluginProcessor::updateHostDisplay (const ChangeDetails& details)
{
    DBG("PluginProcessor::updateHostDisplay");
    juce::AudioProcessor::updateHostDisplay (details);
}

SynthGuiInterface* PluginProcessor::getGuiInterface()
{
    juce::AudioProcessorEditor* editor = getActiveEditor();
    if (editor)
        return dynamic_cast<SynthGuiInterface*> (editor);
    return nullptr;
}
