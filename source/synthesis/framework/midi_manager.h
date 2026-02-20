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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "common.h"
#include <map>
#include <string>

#if !defined(JUCE_AUDIO_DEVICES_H_INCLUDED)

class juce::MidiInput {};

class juce::MidiInputCallback {
  public:
    virtual ~juce::MidiInputCallback() = default;
    virtual void handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &midi_message) { }
    virtual void 	handlePartialSysexMessage (juce::MidiInput *source, const uint *messageData, int numBytesSoFar, double timestamp) { }
};

class juce::MidiMessageCollector {
  public:
    void reset(int sample_rate) { }
    void removeNextBlockOfMessages(juce::MidiBuffer& buffer, int num_samples) { }
    void addMessageToQueue(const juce::MidiMessage &midi_message) { }
};

#endif

#include "valuetree_utils/VariantConverters.h"
#include "tracktion_ValueTreeUtilities.h"
#include <juce_data_structures/juce_data_structures.h>
#include "Identifiers.h"

class SynthBase;

namespace bitklavier {
  class SoundEngine;
  struct ValueDetails;
    struct MidiDeviceWrapper {
        MidiDeviceWrapper(const juce::ValueTree &v) : state(v) {
            identifier.referTo(state,IDs::midiDeviceId, nullptr);
        }
        juce::ValueTree state;
        juce::CachedValue<juce::String> identifier;
    };
} // namespace bitklavier

class MidiManager : public juce::MidiInputCallback, public tracktion::engine::ValueTreeObjectList<bitklavier::MidiDeviceWrapper>, private juce::ChangeListener {
  public:
    typedef std::map<int, std::map<std::string, const bitklavier::ValueDetails*>> midi_map;

    enum MidiMainType {
      kNoteOff = 0x80,
      kNoteOn = 0x90,
      kAftertouch = 0xa0,
      kController = 0xb0,
      kProgramChange = 0xc0,
      kChannelPressure = 0xd0,
      kPitchWheel = 0xe0,
    };

    enum MidiSecondaryType {
      kBankSelect = 0x00,
      kModWheel = 0x01,
      kFolderSelect = 0x20,
      kSustainPedal = 0x40,
      kSostenutoPedal = 0x42,
      kSoftPedalOn = 0x43,
      kSlide = 0x4a,
      kLsbPressure = 0x66,
      kLsbSlide = 0x6a,
      kAllSoundsOff = 0x78,
      kAllControllersOff = 0x79,
      kAllNotesOff = 0x7b,
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void valueChangedThroughMidi(const std::string& name, float value) = 0;
        virtual void pitchWheelMidiChanged(float value) = 0;
        virtual void modWheelMidiChanged(float value) = 0;
        virtual void presetChangedThroughMidi(juce::File preset) = 0;
    };

    // Lightweight live MIDI listener for UI visualisation (note on/off only)
    class LiveMidiListener {
      public:
        virtual ~LiveMidiListener() = default;
        virtual void midiNoteChanged (int note, bool isDown, int channel, float velocity01) = 0;
    };

    MidiManager( juce::MidiKeyboardState* keyboard_state, juce::AudioDeviceManager *manager, const juce::ValueTree &v={},
                 Listener* listener = nullptr);
    virtual ~MidiManager() override;

    // Enqueue an external MIDI message (e.g., from UI) to be consumed on the audio thread
    void postExternalMidi (const juce::MidiMessage& msg);

    // Register/unregister live MIDI listeners (UI thread safe via callAsync in implementation)
    void addLiveMidiListener (LiveMidiListener* l);
    void removeLiveMidiListener (LiveMidiListener* l);
    bitklavier::MidiDeviceWrapper* createNewObject(const juce::ValueTree& v) override
    {
        return new bitklavier::MidiDeviceWrapper(v);
    }
    void deleteObject (bitklavier::MidiDeviceWrapper* at) override;
    void newObjectAdded (bitklavier::MidiDeviceWrapper* obj) override;
    void objectRemoved (bitklavier::MidiDeviceWrapper* obj) override;
    void objectOrderChanged() override { }
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;
    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType (IDs::midiInput);
    }
    void armMidiLearn(std::string name);
    void cancelMidiLearn();
    void clearMidiLearn(const std::string& name);
    void midiInput(int control, float value);
    void processMidiMessage(const juce::MidiMessage &midi_message, int sample_position = 0);
    bool isMidiMapped(const std::string& name) const;

    void setSampleRate(double sample_rate);
    void removeNextBlockOfMessages(juce::MidiBuffer& buffer, int num_samples);
    void replaceKeyboardMessages(juce::MidiBuffer& buffer, int num_samples);

    void processAllNotesOff(const juce::MidiMessage& midi_message, int sample_position, int channel);
    void processAllSoundsOff();
    void processSustain(const juce::MidiMessage& midi_message, int sample_position, int channel);
    void processSostenuto(const juce::MidiMessage& midi_message, int sample_position, int channel);
    void processPitchBend(const juce::MidiMessage& midi_message, int sample_position, int channel);
    void processPressure(const juce::MidiMessage& midi_message, int sample_position, int channel);
    void processSlide(const juce::MidiMessage& midi_message, int sample_position, int channel);

    bool isMpeChannelMasterLowerZone(int channel);
    bool isMpeChannelMasterUpperZone(int channel);

    force_inline int lowerZoneStartChannel() { return mpe_zone_layout_.getLowerZone().getFirstMemberChannel() - 1; }
    force_inline int upperZoneStartChannel() { return mpe_zone_layout_.getUpperZone().getLastMemberChannel() - 1; }
    force_inline int lowerZoneEndChannel() { return mpe_zone_layout_.getLowerZone().getLastMemberChannel() - 1; }
    force_inline int upperZoneEndChannel() { return mpe_zone_layout_.getUpperZone().getFirstMemberChannel() - 1; }
    force_inline int lowerMasterChannel() { return mpe_zone_layout_.getLowerZone().getMasterChannel() - 1; }
    force_inline int upperMasterChannel() { return mpe_zone_layout_.getUpperZone().getMasterChannel() - 1; }

    void setMpeEnabled(bool enabled) { mpe_enabled_ = enabled; }

    midi_map getMidiLearnMap() { return midi_learn_map_; }
    void setMidiLearnMap(const midi_map& midi_learn_map) { midi_learn_map_ = midi_learn_map; }

    // juce::MidiInputCallback
    void handleIncomingMidiMessage (juce::MidiInput *source, const juce::MidiMessage &midi_message) override;

    // juce::ChangeListener
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    struct PresetLoadedCallback : public juce::CallbackMessage {
      PresetLoadedCallback(Listener* lis, juce::File pre) : listener(lis), preset(std::move(pre)) { }

      void messageCallback() override {
        if (listener)
          listener->presetChangedThroughMidi(preset);
      }

      Listener* listener;
      juce::File preset;
    };
    juce::MidiMessageCollector midi_collector_;
    juce::AudioDeviceManager *manager;
    void allNotesOff() {
        keyboard_state_->allNotesOff(0);
    }

    void updateDefaultMidiListeners();
  protected:
    void readMpeMessage(const juce::MidiMessage& message);

//    SynthBase* synth_;
//    bitklavier::SoundEngine* engine_;
    juce::MidiKeyboardState* keyboard_state_;

    std::map<std::string, juce::String>* gui_state_;
    Listener* listener_;
    int current_bank_;
    int current_folder_;
    int current_preset_;

    const bitklavier::ValueDetails* armed_value_;
    midi_map midi_learn_map_;

    int msb_pressure_values_[bitklavier::kNumMidiChannels];
    int lsb_pressure_values_[bitklavier::kNumMidiChannels];
    int msb_slide_values_[bitklavier::kNumMidiChannels];
    int lsb_slide_values_[bitklavier::kNumMidiChannels];

    bool mpe_enabled_;
    bool defaultMidiInputEnabled = false;
    juce::MPEZoneLayout mpe_zone_layout_;
    juce::MidiRPNDetector rpn_detector_;

    // UI live listeners, notified on the message thread via callAsync
    juce::ListenerList<LiveMidiListener> live_listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiManager)
};

