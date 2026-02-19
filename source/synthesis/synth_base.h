/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <chowdsp_dsp_data_structures/chowdsp_dsp_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <set>
#include <string>
#include "midi_manager.h"
#include "ModulationConnection.h"
#include "circular_queue.h"
#include "ModulatorBase.h"
#include "Factory.h"
class SynthGuiInterface;
template<typename T>
class BKSamplerSound;
class PreparationList;
class SampleLoadManager;

class BKSynthesiserSound;
namespace bitklavier {
    class ConnectionList;
    class ModConnectionList;
}

#include "PluginScannerSubprocess.h"
enum class SwitchTriggerThread
{
    MessageThread,
    AudioThread,
};

class UserPreferencesWrapper;
typedef Factory<juce::AudioProcessor, std::unique_ptr<juce::AudioProcessor>> PreparationFactory; //, int,SynthBase& ,const juce::ValueTree&  > PreparationFactory;

class SynthBase : public juce::ValueTree::Listener {
public:
    static constexpr float kOutputWindowMinNote = 16.0f;
    static constexpr float kOutputWindowMaxNote = 128.0f;

    SynthBase(juce::AudioDeviceManager * = {});

    virtual ~SynthBase();

    bitklavier::SoundEngine *getEngine() { return engine_.get(); }
    juce::MidiKeyboardState *getKeyboardState() { return keyboard_state_.get(); }

    int getSampleRate();
    int getBufferSize();

    virtual const juce::CriticalSection &getCriticalSection() = 0;
    virtual void pauseProcessing(bool pause) = 0;

    bitklavier::ModulationConnectionBank &getModulationBank();

    bitklavier::StateConnectionBank &getStateBank();
    bitklavier::ParamOffsetBank &getParamOffsetBank();

    bool loadFromFile(juce::File preset, std::string &error);
    //unused but could be useful for future mpe and or midi mapping functionality
    void setMpeEnabled(bool enabled);
    bool isMidiMapped(const std::string &name);
    //unused but should be used to allow users to save to the most recently selected file
    bool saveToFile(juce::File preset);
    bool saveToActiveFile();

    void clearActiveFile() { active_file_ = juce::File(); }
    juce::File getActiveFile() { return active_file_; }
    juce::ValueTree &getValueTree();
    juce::UndoManager &getUndoManager();

    //processor adding functions
    juce::AudioProcessorGraph::Node::Ptr addProcessor(std::unique_ptr<juce::AudioProcessor> processor,
                                                      juce::AudioProcessorGraph::NodeID id = {});

    juce::AudioProcessorGraph::Node::Ptr removeProcessor(juce::AudioProcessorGraph::NodeID id);

    juce::AudioProcessorGraph::Node::Ptr addPlugin(std::unique_ptr<juce::AudioPluginInstance> instance,
                                                   const juce::String &error,
                                                   juce::Point<double> pos,
                                                   PluginDescriptionAndPreference::UseARA useARA);

    juce::AudioProcessorGraph::Node *getNodeForId(juce::AudioProcessorGraph::NodeID id);
    std::map<juce::String, juce::ReferenceCountedArray<BKSynthesiserSound>*> * getSamples();

    //all connection code
    void addConnection(juce::AudioProcessorGraph::Connection &connect);

    void removeConnection(const juce::AudioProcessorGraph::Connection &connect);

    void addTuningConnection(juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID);

    void connectTuning(const juce::ValueTree &v);

    void addTempoConnection(juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID);

    void connectTempo(const juce::ValueTree &v);

    void addSynchronicConnection(juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID);

    void connectSynchronic(const juce::ValueTree &v);

    bool isConnected(juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID);

    bool addModulationConnection(juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID);

    bool connectStateModulation(const std::string &source, const std::string &destination);

    bool connectModulation(const std::string &source, const std::string &destination);

    bool connectModulation(const juce::ValueTree &v);

    bool connectReset(const juce::ValueTree &v);
    bool disconnectModulation(const juce::ValueTree& v);
    void disconnectModulation(const std::string &source, const std::string &destination);

    void disconnectStateModulation(const std::string &source, const std::string &destination);

    void connectModulation(bitklavier::ModulationConnection *connection);

    void disconnectModulation(bitklavier::ModulationConnection *connection);

    void disconnectModulation(bitklavier::StateConnection *connection);

    void connectStateModulation(bitklavier::StateConnection *connection, const juce::ValueTree& v);


    ///modulation functionality
    void deleteConnectionsWithId(juce::AudioProcessorGraph::NodeID delete_id);

    std::vector<bitklavier::ModulationConnection *> getSourceConnections(const std::string &sourceId) const;

    std::vector<bitklavier::StateConnection *> getSourceStateConnections(const std::string &sourceId) const;

    std::vector<bitklavier::ModulationConnection *> getDestinationConnections(const std::string &destinationId) const;

    std::vector<bitklavier::StateConnection *> getDestinationStateConnections(const std::string &destinationId) const;

    bitklavier::ModulationConnection *getConnection(const std::string &source, const std::string &destination) const;

    bitklavier::StateConnection *getStateConnection(const std::string &source, const std::string &destination) const;

    int getNumModulations(const std::string &destination);

    void requestResetAllContinuousModsRT();

    virtual SynthGuiInterface *getGuiInterface() = 0;

    bool isSourceConnected(const std::string &source);

    void setActivePiano(const juce::ValueTree &v, SwitchTriggerThread );

    void valueTreeChildAdded(juce::ValueTree &parentTree,
                             juce::ValueTree &childWhichHasBeenAdded);

    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged,
                                  const juce::Identifier &property);
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;

    juce::ValueTree findParentWithType(juce::ValueTree child, const juce::Identifier& typeName_);

    static juce::ValueTree getChildWithPropertyAndType (const juce::ValueTree& parent,
                                                        const juce::Identifier& property,
                                                        const juce::var& value,
                                                        const juce::Identifier& type);

    //single consumer single produce queue (thread safe / non blocking)
    static constexpr size_t actionSize = 64; // sizeof ([this, i = index] { callMessageThreadBroadcaster (i); })
    using AudioThreadAction = juce::dsp::FixedSizeFunction<actionSize, void()>;
    moodycamel::ReaderWriterQueue<AudioThreadAction> processorInitQueue{10};

    juce::AudioDeviceManager *manager;
    std::shared_ptr<UserPreferencesWrapper> user_prefs;
    Factory<ModulatorBase, ModulatorBase*> modulator_factory;
    Factory<juce::AudioProcessor> prepFactory;
    bitklavier::CircularQueue<bitklavier::ModulationConnection *> mod_connections_;
    bitklavier::CircularQueue<bitklavier::StateConnection *> state_connections_;
    /** Calls an action on the main thread via chowdsp::DeferredAction */
    template <typename Callable>
    void callOnMainThread (Callable&& func, bool couldBeAudioThread = false)
    {
        mainThreadAction.call (std::forward<Callable> (func), couldBeAudioThread);
    }
    std::unique_ptr<SampleLoadManager> sampleLoadManager ;

    juce::ValueTree getCurrentPiano() {
        for (auto vt : tree) {
            if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::isActive))
                return vt;
        }
    }
    void finishedSampleLoading();
    void startSampleLoading();
    void clearAllGuiListeners();

protected:
    // Holds the parsed preset tree until samples finish loading
    juce::ValueTree pendingPresetTree;
    std::atomic<bool> samplesLoading { false };
    // True while we’re waiting to apply pendingPresetTree
    std::atomic<bool> presetPending { false };
    chowdsp::DeferredAction mainThreadAction;
    //    bool isInvalidConnection(const electrosynth::mapping_change & change) {return false;}
    juce::ValueTree tree;
    juce::UndoManager um;

    bool loadFromValueTree(const juce::ValueTree &state);

    void processAudio(juce::AudioSampleBuffer *buffer, int channels, int samples, int offset);

    void processAudioAndMidi(juce::AudioBuffer<float> &audio_buffer, juce::MidiBuffer &midi_buffer);

    // , int channels, int samples, int offset, int start_sample = 0, int end_sample = 0);
    void processAudioWithInput(juce::AudioSampleBuffer *buffer, const float *input_buffer,
                               int channels, int samples, int offset);

    void writeAudio(juce::AudioSampleBuffer *buffer, int channels, int samples, int offset);

    void processMidi(juce::MidiBuffer &buffer, int start_sample = 0, int end_sample = 0);

    void processKeyboardEvents(juce::MidiBuffer &buffer, int num_samples);

    void clearAllBackend();
    std::unique_ptr<bitklavier::SoundEngine> engine_;
    std::unique_ptr<MidiManager> midi_manager_;
    std::unique_ptr<juce::MidiKeyboardState> keyboard_state_;

    std::shared_ptr<SynthBase *> self_reference_;

    juce::File active_file_;

    bool expired_;

public:
    PreparationList *getActivePreparationList();
     juce::ValueTree getActivePreparationListValueTree();
    juce::ValueTree getActivePianoValueTree();

    bitklavier::ConnectionList *getActiveConnectionList();

    bitklavier::ModConnectionList *getActiveModConnectionList();

    juce::ValueTree activePiano;
    SwitchTriggerThread switch_trigger_thread = SwitchTriggerThread::MessageThread;
    juce::uint64 sample_index_of_switch;
    juce::uint64 total_samples_passed;
    //ensure prep list is deleted before mod connection and connection

    std::vector<std::unique_ptr<bitklavier::ConnectionList> > connectionLists;
    std::vector<std::unique_ptr<bitklavier::ModConnectionList> > mod_connection_lists_;
    std::vector<std::unique_ptr<PreparationList> > preparationLists;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthBase)
};

class HeadlessSynth : public SynthBase {
public:
    virtual const juce::CriticalSection &getCriticalSection() override {
        return critical_section_;
    }

    virtual void pauseProcessing(bool pause) override {
        if (pause)
            critical_section_.enter();
        else
            critical_section_.exit();
    }

protected:
    virtual SynthGuiInterface *getGuiInterface() override { return nullptr; }

private:
    juce::CriticalSection critical_section_;
};
