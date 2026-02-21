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

    //juce::ignoreUnused (sampleRate, samplesPerBlock);
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
    juce::ValueTree state ("bitKlavierPluginState");

    if (user_prefs != nullptr)
    {
        juce::String lastPath = user_prefs->tree.getProperty ("last_gallery_path");
        state.setProperty ("last_gallery_path", lastPath, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
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

SynthGuiInterface* PluginProcessor::getGuiInterface()
{
    juce::AudioProcessorEditor* editor = getActiveEditor();
    if (editor)
        return dynamic_cast<SynthGuiInterface*> (editor);
    return nullptr;
}
