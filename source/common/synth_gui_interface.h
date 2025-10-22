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
#include "ApplicationCommandHandler.h"
#include "valuetree_utils/VariantConverters.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

class UserPreferencesWrapper;
class FullInterface;
struct OpenGlWrapper;
class ModulatorBase;
class SampleLoadManager;
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

class PopupItems;
class
SynthGuiInterface :  public juce::ApplicationCommandTarget {
public:
    SynthGuiInterface(SynthBase* synth, bool use_gui = true);
    virtual ~SynthGuiInterface();
    // Define your command IDs
    enum CommandIDs {
        undo = 0x2000,
        redo,
        save,
        load,
        showPluginListEditor   = 0x30100
    };

    void getAllCommands(juce::Array<juce::CommandID> &commands) override {
        commands.addArray({undo, redo,showPluginListEditor});
    }

    void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo &info) override {
        switch (id) {
            case undo:
                info.setInfo("Undo", "Undo last action", "Edit", 0);
                info.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            break;
            case redo:
                info.setInfo("Redo", "Redo last action", "Edit", 0);
                info.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;

            case showPluginListEditor:
                info.setInfo("Show Plugins", "Show Plugins", "Options", 0);
                info.addDefaultKeypress ('p', juce::ModifierKeys::commandModifier);
            break;
        }
    }

    bool perform(const InvocationInfo &info) override;

    ApplicationCommandTarget* getNextCommandTarget() override {return nullptr;}

    virtual juce::AudioDeviceManager* getAudioDeviceManager() { return nullptr; }
    SynthBase* getSynth() { return synth_; }
    SampleLoadManager* getSampleLoadManager();
    juce::UndoManager* getUndoManager();
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
    void notifyPrepPopupMoved();
    void  addProcessor(std::unique_ptr<juce::AudioPluginInstance> instance );
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
    void allNotesOff();
    void setActivePiano(const juce::ValueTree&);
    void addPiano(const juce::String&);
    void setPianoSwitchTriggerThreadMessage();
    void removeAllGuiListeners();
    std::vector<std::string> getAllPianoNames();
    FullInterface* getGui() { return gui_.get(); }
    OpenGlWrapper* getOpenGlWrapper();
    juce::File getActiveFile();

   // std::unique_ptr<ApplicationCommandHandler> commandHandler;
    PopupItems getPluginPopupItems();
    class PluginListWindow;
    std::unique_ptr<PluginListWindow> pluginListWindow;
   juce::ScopedMessageBox messageBox;
    juce::ApplicationCommandManager commandManager;
    juce::ValueTree gallery;

  protected:
    std::atomic<bool> loading;
    std::unique_ptr<juce::FileChooser> filechooser;
    SynthBase* synth_;
    std::unique_ptr<FullInterface> gui_;

  
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthGuiInterface)
};

