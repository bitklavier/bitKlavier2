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


class SynthBase;
#if HEADLESS

class FullInterface { };
class juce::AudioDeviceManager { };

#endif

//#include "synth_base.h"

#include "ApplicationCommandHandler.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
class SynthBase;
class SampleLoadManager;
class UserPreferencesWrapper;
class FullInterface;
struct OpenGlWrapper;
class ModulatorBase;
class PreparationSection;
struct SynthGuiData {
  SynthGuiData(SynthBase* synth_base);
  juce::ValueTree& tree;
  juce::UndoManager& um;
  SynthBase* synth;
};
namespace bitklavier
{
    class ModulationProcessor;
class ModulationConnection;
class StateConnection;
}
class
SynthGuiInterface {
  public:
    SynthGuiInterface(SynthBase* synth, bool use_gui = true);
    virtual ~SynthGuiInterface();

    virtual juce::AudioDeviceManager* getAudioDeviceManager() { return nullptr; }
    SynthBase* getSynth() { return synth_; }
    virtual void updateFullGui();
    virtual void updateGuiControl(const std::string& name, float value);
    float getControlValue(const std::string& name);
    void tryEnqueueProcessorInitQueue(juce::FixedSizeFunction<64, void()> callback);
    const juce::CriticalSection& getCriticalSection();
    void connectModulation(std::string source, std::string destination);
    void connectStateModulation(std::string source, std::string destination);
    void disconnectModulation(std::string source, std::string destination);
    void disconnectStateModulation(std::string source, std::string destination);
    void disconnectModulation(bitklavier::ModulationConnection* connection);
    void disconnectModulation(bitklavier::StateConnection* connection);
    void notifyModulationsChanged();

    void  addProcessor(PreparationSection* );
    void  addModulationNodeConnection(juce::AudioProcessorGraph::NodeID source, juce::AudioProcessorGraph::NodeID destination);
    void setFocus();
    void notifyChange();
    void notifyFresh();
    void openSaveDialog();
    void openLoadDialog();
    void externalPresetLoaded(juce::File preset);
    void setGuiSize(float scale);
    bool loadFromFile(juce::File preset, std::string& error);
    bool isConnected(juce::AudioProcessorGraph::Connection &connection);
    bool isConnected(juce::AudioProcessorGraph::NodeID,juce::AudioProcessorGraph::NodeID);
    void synchronizeValueTree();
    FullInterface* getGui() { return gui_.get(); }
    OpenGlWrapper* getOpenGlWrapper();
    juce::File getActiveFile();
    std::shared_ptr<UserPreferencesWrapper> userPreferences;
    std::unique_ptr<SampleLoadManager> sampleLoadManager ;
    std::unique_ptr<ApplicationCommandHandler> commandHandler;
  protected:
    std::atomic<bool> loading;
juce::ApplicationCommandManager commandManager;

    std::unique_ptr<juce::FileChooser> filechooser;

    SynthBase* synth_;
    std::unique_ptr<FullInterface> gui_;
  
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthGuiInterface)
};

