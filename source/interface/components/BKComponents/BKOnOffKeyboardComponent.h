//
// Created by Davis Polito on 5/27/25.
//

#ifndef BKONOFFKEYBOARDCOMPONENT_H
#define BKONOFFKEYBOARDCOMPONENT_H

#include "juce_audio_utils/juce_audio_utils.h"

class BKOnOffKeyboardComponent : public juce::KeyboardComponentBase{
public:
    enum ColourIds
    {
        whiteNoteColourId               = 0x1005000,
        blackNoteColourId               = 0x1005001,
        keySeparatorLineColourId        = 0x1005002,
        mouseOverKeyOverlayColourId     = 0x1005003,  /**< This colour will be overlaid on the normal note colour. */
        keyDownOverlayColourId          = 0x1005004,  /**< This colour will be overlaid on the normal note colour. */
        textLabelColourId               = 0x1005005,
        upDownButtonBackgroundColourId  = 0x1005006,
        upDownButtonArrowColourId       = 0x1005007,
        shadowColourId                  = 0x1005008,
        keyPlayed                       = 0x1005009
    };
    BKOnOffKeyboardComponent(Orientation o, std::atomic<std::bitset<128>> & keys) : juce::KeyboardComponentBase(o), keys(keys)
    {
        setColour(whiteNoteColourId, juce::Colours::white);
        setColour(blackNoteColourId, juce::Colours::black);
        setColour(keySeparatorLineColourId, juce::Colours::grey);
        setColour(mouseOverKeyOverlayColourId, juce::Colours::yellow);
        setColour(keyDownOverlayColourId, juce::Colours::yellow);
        setColour(textLabelColourId, juce::Colours::black);
        setColour(upDownButtonBackgroundColourId, juce::Colours::grey);
        setColour(upDownButtonArrowColourId, juce::Colours::black);
        setColour(shadowColourId, juce::Colours::black.withAlpha(0.5f));
        setColour(keyPlayed, juce::Colours::red);
    };

    ~BKOnOffKeyboardComponent() {
    }
    void drawWhiteKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) override;
    void drawBlackKey(int midiNoteNumber, juce::Graphics &g, juce::Rectangle<float> area) override;
    void drawKeyboardBackground(juce::Graphics &g, juce::Rectangle<float> area) override;
    int mouseOverNote = -1;
    std::atomic<std::bitset<128>> &keys;

    // Live MIDI overlay (display-only) — separate from selection bitset
    void setLiveKeyState (int midiNoteNumber, bool isDown)
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128)
        {
            liveKeys.set ((size_t) midiNoteNumber, isDown);
            repaint();
        }
    }

    void clearAllLiveKeys()
    {
        liveKeys.reset();
        repaint();
    }

private:
    std::bitset<128> liveKeys;
};


#endif //BKONOFFKEYBOARDCOMPONENT_H
