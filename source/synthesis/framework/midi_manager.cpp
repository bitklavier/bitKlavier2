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

#include "midi_manager.h"


namespace {
  constexpr int kMidiControlBits = 7;
  constexpr float kHighResolutionMax = (1 << (2 * kMidiControlBits)) - 1.0f;
  constexpr float kControlMax = (1 << kMidiControlBits) - 1.0f;

  force_inline float toHighResolutionValue(int msb, int lsb) {
    if (lsb < 0)
      return msb / kControlMax;

    return ((msb << kMidiControlBits) + lsb) / kHighResolutionMax;
  }
} // namespace

MidiManager::MidiManager(juce::MidiKeyboardState* keyboard_state, juce::AudioDeviceManager* manager, const juce::ValueTree &v,
                          Listener* listener) : tracktion::engine::ValueTreeObjectList<bitklavier::MidiDeviceWrapper>(v),
     keyboard_state_(keyboard_state),
    listener_(listener), armed_value_(nullptr),
    msb_pressure_values_(), msb_slide_values_() , manager(manager){
  //engine_ = synth_->get//engine();
  current_bank_ = -1;
  current_folder_ = -1;
  current_preset_ = -1;
  
  for (int i = 0; i < bitklavier::kNumMidiChannels; ++i) {
    lsb_slide_values_[i] = -1;
    lsb_pressure_values_[i] = -1;
  }

  mpe_enabled_ = false;
  mpe_zone_layout_.setLowerZone(bitklavier::kNumMidiChannels - 1);
    rebuildObjects();
    for(auto obj: objects)
    {
        manager->addMidiInputDeviceCallback(obj->identifier, this);
    }
}

MidiManager::~MidiManager() {
  freeObjects();
}

void MidiManager::armMidiLearn(std::string name) {
  current_bank_ = -1;
  current_folder_ = -1;
  current_preset_ = -1;
//  armed_value_ = &bitklavier::Parameters::getDetails(name);
}

void MidiManager::cancelMidiLearn() {
  armed_value_ = nullptr;
}

//void MidiManager::clearMidiLearn(const std::string& name) {
//  for (auto& controls : midi_learn_map_) {
//    if (controls.second.count(name)) {
//      midi_learn_map_[controls.first].erase(name);
//      LoadSave::saveMidiMapConfig(this);
//    }
//  }
//}

void MidiManager::midiInput(int midi_id, float value) {
//  if (armed_value_) {
//    midi_learn_map_[midi_id][armed_value_->name] = armed_value_;
//    armed_value_ = nullptr;
//
//    // TODO: Probably shouldn't write this config on the audio thread.
//    LoadSave::saveMidiMapConfig(this);
//  }

//  if (midi_learn_map_.count(midi_id)) {
//    for (auto& control : midi_learn_map_[midi_id]) {
//      const bitklavier::ValueDetails* details = control.second;
//      float percent = value / kControlMax;
//      float range = details->max - details->min;
//      float translated = percent * range + details->min;
//
//      if (details->value_scale == bitklavier::ValueDetails::kIndexed)
//        translated = std::round(translated);
//      listener_->valueChangedThroughMidi(control.first, translated);
//    }
//  }
}

bool MidiManager::isMidiMapped(const std::string& name) const {
  for (auto& controls : midi_learn_map_) {
    if (controls.second.count(name))
      return true;
  }
  return false;
}

void MidiManager::setSampleRate(double sample_rate) {
  midi_collector_.reset(sample_rate);
}

void MidiManager::removeNextBlockOfMessages(juce::MidiBuffer& buffer, int num_samples) {
  midi_collector_.removeNextBlockOfMessages(buffer, num_samples);
}

void MidiManager::readMpeMessage(const juce::MidiMessage& message) {
  mpe_zone_layout_.processNextMidiEvent(message);
}

void MidiManager::processAllNotesOff(const juce::MidiMessage& midi_message, int sample_position, int channel) {
//  if (isMpeChannelMasterLowerZone(channel))
//    //engine_->allNotesOffRange(sample_position, lowerZoneStartChannel(), lowerZoneEndChannel());
//  else if (isMpeChannelMasterUpperZone(channel))
//    //engine_->allNotesOffRange(sample_position, upperZoneStartChannel(), upperZoneEndChannel());
//  else
//    //engine_->allNotesOff(sample_position, channel);
}

void MidiManager::processAllSoundsOff() {
  //engine_->allSoundsOff();
}

void MidiManager::processSustain(const juce::MidiMessage& midi_message, int sample_position, int channel) {
  bool on = midi_message.isSustainPedalOn();
  if (isMpeChannelMasterLowerZone(channel)) {
    //if (on)
      //engine_->sustainOnRange(lowerZoneStartChannel(), lowerZoneEndChannel());
    //else
      //engine_->sustainOffRange(sample_position, lowerZoneStartChannel(), lowerZoneEndChannel());
  }
  else if (isMpeChannelMasterUpperZone(channel)) {
    //if (on)
      //engine_->sustainOnRange(upperZoneStartChannel(), upperZoneEndChannel());
    //else
      //engine_->sustainOffRange(sample_position, upperZoneStartChannel(), upperZoneEndChannel());
  }
  else {
    //if (on)
      //engine_->sustainOn(channel);
    //else
      //engine_->sustainOff(sample_position, channel);
  }
}

void MidiManager::processSostenuto(const juce::MidiMessage& midi_message, int sample_position, int channel) {
  bool on = midi_message.isSostenutoPedalOn();
  if (isMpeChannelMasterLowerZone(channel)) {
    //if (on)
      //engine_->sostenutoOnRange(lowerZoneStartChannel(), lowerZoneEndChannel());
    //else
      //engine_->sostenutoOffRange(sample_position, lowerZoneStartChannel(), lowerZoneEndChannel());
  }
  else if (isMpeChannelMasterUpperZone(channel)) {
    //if (on)
      //engine_->sostenutoOnRange(upperZoneStartChannel(), upperZoneEndChannel());
    //else
      //engine_->sostenutoOffRange(sample_position, upperZoneStartChannel(), upperZoneEndChannel());
  }
  else {
   // if (on)
      //engine_->sostenutoOn(channel);
   // else
      //engine_->sostenutoOff(sample_position, channel);
  }
}

void MidiManager::processPitchBend(const juce::MidiMessage& midi_message, int sample_position, int channel) {
  float percent = midi_message.getPitchWheelValue() / kHighResolutionMax;
  float value = 2 * percent - 1.0f;

  if (isMpeChannelMasterLowerZone(channel)) {
    //engine_->setZonedPitchWheel(value, lowerMasterChannel(), lowerMasterChannel() + 1);
    //engine_->setZonedPitchWheel(value, lowerZoneStartChannel(), lowerZoneEndChannel());
    listener_->pitchWheelMidiChanged(value);
  }
  else if (isMpeChannelMasterUpperZone(channel)) {
    //engine_->setZonedPitchWheel(value, upperMasterChannel(), upperMasterChannel() + 1);
    //engine_->setZonedPitchWheel(value, upperZoneStartChannel(), upperZoneEndChannel());
    listener_->pitchWheelMidiChanged(value);
  }
  //else if (mpe_enabled_)
    //engine_->setPitchWheel(value, channel);
 // else {
    //engine_->setZonedPitchWheel(value, channel, channel);
   // listener_->pitchWheelMidiChanged(value);
  //}
}

void MidiManager::processPressure(const juce::MidiMessage& midi_message, int sample_position, int channel) {
  float value = toHighResolutionValue(msb_pressure_values_[channel], lsb_pressure_values_[channel]);
 // if (isMpeChannelMasterLowerZone(channel))
    //engine_->setChannelRangeAftertouch(lowerZoneStartChannel(), lowerZoneEndChannel(), value, 0);
 // else if (isMpeChannelMasterUpperZone(channel))
    //engine_->setChannelRangeAftertouch(upperZoneStartChannel(), upperZoneEndChannel(), value, 0);
 // else
    //engine_->setChannelAftertouch(channel, value, sample_position);
}

void MidiManager::processSlide(const juce::MidiMessage& midi_message, int sample_position, int channel) {
  float value = toHighResolutionValue(msb_slide_values_[channel], lsb_slide_values_[channel]);
  //if (isMpeChannelMasterLowerZone(channel))
    //engine_->setChannelRangeSlide(value, lowerZoneStartChannel(), lowerZoneEndChannel(), 0);
  //else if (isMpeChannelMasterUpperZone(channel))
    //engine_->setChannelRangeSlide(value, upperZoneStartChannel(), upperZoneEndChannel(), 0);
 // else
    //engine_->setChannelSlide(channel, value, sample_position);
}

force_inline bool MidiManager::isMpeChannelMasterLowerZone(int channel) {
  return mpe_enabled_ && mpe_zone_layout_.getLowerZone().isActive() && lowerMasterChannel() == channel;
}

force_inline bool MidiManager::isMpeChannelMasterUpperZone(int channel) {
  return mpe_enabled_ && mpe_zone_layout_.getUpperZone().isActive() && upperMasterChannel() == channel;
}

//unused
void MidiManager::processMidiMessage(const juce::MidiMessage& midi_message, int sample_position) {
  if (midi_message.isController())
    readMpeMessage(midi_message);

  int channel = midi_message.getChannel() - 1;
  MidiMainType type = static_cast<MidiMainType>(midi_message.getRawData()[0] & 0xf0);
  switch (type) {
    case kProgramChange:
      return;
    case kNoteOn: {
      juce::uint8 velocity = midi_message.getVelocity();
    //  if (velocity)
        //engine_->noteOn(midi_message.getNoteNumber(), velocity / kControlMax, sample_position, channel);
    //  else
        //engine_->noteOff(midi_message.getNoteNumber(), velocity / kControlMax, sample_position, channel);
      return;
    }
    case kNoteOff: {
      float velocity = midi_message.getVelocity() / kControlMax;
      //engine_->noteOff(midi_message.getNoteNumber(), velocity, sample_position, channel);
      return;
    }
    case kAftertouch: {
      int note = midi_message.getNoteNumber();
      float value = midi_message.getAfterTouchValue() / kControlMax;
      //engine_->setAftertouch(note, value, sample_position, channel);
      return;
    }
    case kChannelPressure: {
      msb_pressure_values_[channel] = midi_message.getChannelPressureValue();
      processPressure(midi_message, sample_position, channel);
      return;
    }
    case kPitchWheel: {
      processPitchBend(midi_message, sample_position, channel);
      return;
    }
    case kController: {
      MidiSecondaryType secondary_type = static_cast<MidiSecondaryType>(midi_message.getControllerNumber());
      switch (secondary_type) {
        case kSlide: {
          msb_slide_values_[channel] = midi_message.getControllerValue();
          processSlide(midi_message, sample_position, channel);
          break;
        }
        case kLsbPressure: {
          lsb_pressure_values_[channel] = midi_message.getControllerValue();
          processPressure(midi_message, sample_position, channel);
          break;
        }
        case kLsbSlide: {
          lsb_slide_values_[channel] = midi_message.getControllerValue();
          processSlide(midi_message, sample_position, channel);
          break;
        }
        case kSustainPedal: {
          processSustain(midi_message, sample_position, channel);
          break;
        }
        case kSostenutoPedal: {
          processSostenuto(midi_message, sample_position, channel);
          break;
        }
        case kSoftPedalOn: // TODO
          break;
        case kModWheel: {
          float percent = (1.0f * midi_message.getControllerValue()) / kControlMax;
          //engine_->setModWheel(percent, channel);
          listener_->modWheelMidiChanged(percent);
          break;
        }
        case kAllNotesOff:
        case kAllControllersOff:
          processAllNotesOff(midi_message, sample_position, channel);
          return;
        case kAllSoundsOff:
          processAllSoundsOff();
          break;
        case kBankSelect:
          current_bank_ = midi_message.getControllerValue();
          return;
        case kFolderSelect:
          current_folder_ = midi_message.getControllerValue();
          return;
      }
      midiInput(midi_message.getControllerNumber(), midi_message.getControllerValue());
    }
  }
}






void MidiManager::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage &midi_message) {
  midi_collector_.addMessageToQueue(midi_message);
}

//be sure that this would be filled with the correct start and numsamples
//for an internal block size different from the external block size
void MidiManager::replaceKeyboardMessages(juce::MidiBuffer& buffer, int num_samples) {
  keyboard_state_->processNextMidiBuffer(buffer, 0, num_samples, true);
}
