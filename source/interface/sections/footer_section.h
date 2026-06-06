// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "CompressorProcessor.h"
#include "EQProcessor.h"
#include "ReverbProcessor.h"
#include "OpenGL_KeymapKeyboard.h"
#include "open_gl_background.h"
#include "peak_meter_section.h"
#include "synth_section.h"

class FooterSection : public SynthSection, BKKeymapKeyboardComponent::Listener
{
public:
    FooterSection(SynthGuiData* data);

    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void reset() override;

    void buttonClicked(juce::Button* clicked_button) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void addListener(SynthSection::Listener* listener) { listeners_.push_back(listener); }

    /**
     * for playback from virtual keyboard
     */
    virtual void BKKeymapKeyboardChanged (juce::String /*name*/, std::bitset<128> keys, int lastKey, juce::ModifierKeys mods = juce::ModifierKeys()) override;

    /** Highlight the footer keyboard with the given keymap key states */
    void displayKeymapState (const std::bitset<128>& keys);

    /** Clear any keymap highlight from the footer keyboard */
    void clearKeymapDisplay();

    /** Clear live (MIDI-driven) key highlights from the footer keyboard */
    void clearLiveKeys();

  private:
    std::shared_ptr<PeakMeterSection> levelMeter;

    KeymapKeyboardState keymap_;
    std::vector<SynthSection::Listener*> listeners_;
    std::shared_ptr<OpenGlQuad> body_;
    std::unique_ptr<OpenGLKeymapKeyboardComponent> keyboard_component_;
    std::unique_ptr<OpenGlTextButton> eqButton;
    std::unique_ptr<OpenGlTextButton> compressorButton;
    std::unique_ptr<OpenGlTextButton> reverbButton;

    EQProcessor* eqProc_ = nullptr;
    CompressorProcessor* compProc_ = nullptr;
    ReverbProcessor* reverbProc_ = nullptr;

    juce::ValueTree gallery;
    chowdsp::ScopedCallbackList callbacks_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterSection)
};




