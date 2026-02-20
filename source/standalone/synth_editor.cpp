/* Copyright 2013-2019 Matt Tytel

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

#include "synth_editor.h"

#include "default_look_and_feel.h"
#include "FullInterface.h"
#include "sound_engine.h"

#include "startup.h"
#include "utils.h"
#include "../common/UserPreferences.h"
#include "../common/load_save.h"
#include <memory>

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

SynthEditor::SynthEditor(bool use_gui) : SynthGuiInterface(this, use_gui), SynthBase(&deviceManager) {
  static constexpr int kHeightBuffer = 50;
#if PERFETTO
  MelatoninPerfetto::get().beginSession();
#endif
  //computer_keyboard_ = std::make_unique<SynthComputerKeyboard>(engine_.get(), keyboard_state_.get());
  current_time_ = 0.0;

  setAudioChannels(0, bitklavier::kNumChannels);

  juce::AudioDeviceManager::AudioDeviceSetup setup;
  deviceManager.getAudioDeviceSetup(setup);
  setup.sampleRate = bitklavier::kDefaultSampleRate;
  setup.bufferSize = 128;
  deviceManager.initialise(0, bitklavier::kNumChannels, nullptr, true, "", &setup);

  if (deviceManager.getCurrentAudioDevice() == nullptr) {
    const juce::OwnedArray<juce::AudioIODeviceType>& device_types = deviceManager.getAvailableDeviceTypes();

    for (juce::AudioIODeviceType* device_type : device_types) {
      deviceManager.setCurrentAudioDeviceType(device_type->getTypeName(), true);
      if (deviceManager.getCurrentAudioDevice())
        break;
    }
  }

    /*
     * todo: confirm this fix is correct, since getDevices() has been removed from JUCE
     */
    //current_midi_ins_ = juce::StringArray(juce::MidiInput::getDevices());
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    current_midi_ins_.clearQuick();
    for (auto& info : midiInputs)
    {
        current_midi_ins_.add (info.name);
    }

  for (const juce::String& midi_in : current_midi_ins_)
    deviceManager.setMidiInputDeviceEnabled(midi_in, true);

  deviceManager.addMidiInputDeviceCallback("", static_cast<juce::MidiInputCallback*>(midi_manager_.get()));

  if (use_gui) {
    setLookAndFeel(DefaultLookAndFeel::instance());
    addAndMakeVisible(gui_.get());
    //maybe want this maybe not -- 4/22/25 -- using this breaks pauseprocessing checks
   // gui_->reset();


    juce::Rectangle<int> total_bounds = juce::Desktop::getInstance().getDisplays().getTotalBounds(true);
    total_bounds.removeFromBottom(kHeightBuffer);

    float window_size = 1.0f;
    window_size = std::min(window_size, total_bounds.getWidth() / (1.0f * bitklavier::kDefaultWindowWidth));
    window_size = std::min(window_size, total_bounds.getHeight() / (1.0f * bitklavier::kDefaultWindowHeight));
    int width = std::round(window_size * bitklavier::kDefaultWindowWidth);
    int height = std::round(window_size * bitklavier::kDefaultWindowHeight);
    setSize(width, height);

    setWantsKeyboardFocus(true);
    //addKeyListener(computer_keyboard_.get());
    setOpaque(true);
  }
 // mainmenumodel on mac

  menuModel = std::make_unique<MainMenuModel>(commandManager);
  juce::MenuBarModel::setMacMainMenu(menuModel.get());

  // Defer initial gallery auto-load until the window/component has a peer (GL context attached).
  // See tryAutoLoadInitialGallery() and timerCallback().

  startTimer(500);
  isInit = true;
}

SynthEditor::~SynthEditor() {
#if PERFETTO
  MelatoninPerfetto::get().endSession();
#endif
  juce::PopupMenu::dismissAllActiveMenus();
  shutdownAudio();
  juce::MenuBarModel::setMacMainMenu(nullptr);
  engine_->shutdown();
}

void SynthEditor::prepareToPlay(int buffer_size, double sample_rate) {
  //engine_->setSampleRate(sample_rate);
  engine_->prepareToPlay(sample_rate, buffer_size);
  midi_manager_->setSampleRate(sample_rate);
}

/*commented out the code to create an internal buffer size
// would need to update our code internally to pass buffers properly sized
// may not be feasible since we use a basic processblock that and our midi-receivers i.e.
* keymap processor gets its midi directly from a midi queue.
* we would probably have to edit keymapprocessor to ensure it only sends midi up to the internal block size
* i.e. if the daw gives us a 256 block but our internal block is locked to 128
* we would need to store all midi messages that occured on/after sample 128
* and then process it in the "second" pass
* we do not hold a global midi_manager in bitklavier
* each keymap processor holds a midi manager and passes its midi along.
* see MidiManager::replaceKeyboardMessages in midi_manager.h
*/
void SynthEditor::getNextAudioBlock(const juce::AudioSourceChannelInfo& buffer) {
  juce::ScopedLock lock(getCriticalSection());

  int num_samples = buffer.buffer->getNumSamples();
  int synth_samples = std::min(num_samples, bitklavier::kMaxBufferSize);

  //processModulationChanges();
  juce::MidiBuffer midi_messages;
//  midi_manager_->removeNextBlockOfMessages(midi_messages, num_samples);
//  processKeyboardEvents(midi_messages, num_samples);

  double sample_time = 1.0 / getSampleRate();
//  for (int b = 0; b < num_samples; b += synth_samples) {
//    int current_samples = std::min<int>(synth_samples, num_samples - b);
//    //engine_->correctToTime(current_time_);
//
////    processMidi(midi_messages, b, b + current_samples);
////    processAudio(buffer.buffer, bitklavier::kNumChannels, current_samples, b);
//    current_time_ += current_samples * sample_time;
//  }
    processAudioAndMidi(*buffer.buffer, midi_messages);

}

void SynthEditor::releaseResources() {
    engine_->releaseResources();
}

void SynthEditor::resized() {
  if (gui_)
  {
    gui_->setBounds (getBounds());
  }
}

void SynthEditor::timerCallback()
{
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    juce::StringArray midi_ins;
    for (auto& info : midiInputs)
    {
        midi_ins.add (info.name);
    }

  for (int i = 0; i < midi_ins.size(); ++i) {
    if (!current_midi_ins_.contains(midi_ins[i]))
      deviceManager.setMidiInputEnabled(midi_ins[i], true);
  }

  current_midi_ins_ = midi_ins;

  // Perform one-shot initial gallery auto-load once a peer is available to avoid early GL calls
  if (! initialGalleryLoadAttempted)
      tryAutoLoadInitialGallery();
}

void SynthEditor::animate(bool animate) {
  if (gui_)
    gui_->animate(animate);
}

void SynthEditor::pauseProcessing(bool pause) {
  DBG("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    DBG(juce::String(int(pause)));
        if (pause)
            critical_section_.enter();
        else
            critical_section_.exit();

}

void SynthEditor::tryAutoLoadInitialGallery()
{
    // Only proceed when the component has a peer (ensures GL context can attach)
    auto* peer = getPeer();
    if (peer == nullptr)
        return; // try again on next timer tick

    // Guard so we attempt only once
    initialGalleryLoadAttempted = true;

    juce::String lastPath = user_prefs->tree.getProperty("last_gallery_path");
    juce::File candidate = expandUserPath(lastPath);

    auto tryLoad = [this](const juce::File& f) -> bool {
        if (! f.existsAsFile())
            return false;
        std::string error;
        return SynthGuiInterface::loadFromFile(const_cast<juce::File&>(f), error);
    };

    bool loaded = false;
    if (candidate.existsAsFile())
        loaded = tryLoad(candidate);

    if (!loaded)
    {
        auto docs = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                        .getChildFile("Documents")
                        .getChildFile("bitKlavier")
                        .getChildFile("galleries");
        juce::File basicPiano = docs.getChildFile("Basic Piano.").withFileExtension(bitklavier::kPresetExtension.c_str());
        if (!basicPiano.existsAsFile())
            basicPiano = docs.getChildFile("Basic Piano").withFileExtension(bitklavier::kPresetExtension.c_str());
        if (!basicPiano.existsAsFile())
            basicPiano = docs.getChildFile("BasicPiano").withFileExtension(bitklavier::kPresetExtension.c_str());

        tryLoad(basicPiano); // ignore result; ok to do nothing if missing
    }
}
