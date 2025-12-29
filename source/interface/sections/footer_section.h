//
// Created by Myra Norton on 11/14/25.
//

#pragma once
#include "OpenGL_KeymapKeyboard.h"
#include "open_gl_background.h"
#include "peak_meter_section.h"
#include "synth_section.h"

class EQPreparation;
class CompressorPreparation;

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
    virtual void BKKeymapKeyboardChanged (juce::String /*name*/, std::bitset<128> keys, int lastKey) override;

  private:
    std::shared_ptr<PeakMeterSection> levelMeter;

    KeymapKeyboardState keymap_;
    std::vector<SynthSection::Listener*> listeners_;
    std::shared_ptr<OpenGlQuad> body_;
    std::unique_ptr<OpenGLKeymapKeyboardComponent> keyboard_component_;
    std::unique_ptr<OpenGlTextButton> eqButton;
    std::unique_ptr<OpenGlTextButton> compressorButton;

    juce::ValueTree gallery;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FooterSection)
};




