//
// Created by Dan Trueman on 10/9/25.
//

#ifndef BITKLAVIER0_OPENGL_KEYMAPKEYBOARD_H
#define BITKLAVIER0_OPENGL_KEYMAPKEYBOARD_H
#pragma once

#include "../components/BKComponents/BKKeymapKeyboardComponent.h"
#include "../../common/synth_gui_interface.h"
#include "../../../synthesis/synth_base.h"
#include "../../../synthesis/sound_engine/sound_engine.h"

class OpenGLKeymapKeyboardComponent: public OpenGlAutoImageComponent<BKKeymapKeyboardComponent>,
                                     public BKKeymapKeyboardComponent::Listener,
                                     public MidiManager::LiveMidiListener {
public:
    OpenGLKeymapKeyboardComponent(KeymapKeyboardState& params, bool helperButtons = true, bool isMono = false) :
        OpenGlAutoImageComponent (&params, helperButtons, isMono), _params(params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        // Receive UI-driven selection changes
        addMyListener(this);

        // Try to register as live MIDI listener (safe if not available yet)
        tryRegisterLiveListener();
    }

    ~OpenGLKeymapKeyboardComponent() override
    {
        tryUnregisterLiveListener();
    }

    void parentHierarchyChanged() override
    {
        tryRegisterLiveListener();
        OpenGlAutoImageComponent::parentHierarchyChanged();
    }

    virtual void resized() override {
        OpenGlAutoImageComponent::resized();
        redoImage();
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDrag(e);
        redoImage();
    }

    void mouseDown(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseDown(e);
        redoImage();
    }

    void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent::mouseMove(e);
        redoImage();
    }

    void buttonClicked(juce::Button *b) override {
        OpenGlAutoImageComponent::buttonClicked(b);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorReturnKeyPressed(textEditor);
        redoImage();
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent::textEditorTextChanged(textEditor);
        redoImage();
    }

    // UI selection changes -> broadcast to engine
    virtual void BKKeymapKeyboardChanged (juce::String /*name*/, std::bitset<128> keys, int lastKey) override
    {
        // keys reflects the current state after the change: if bit is set, it's a key down
        const bool isDown = (lastKey >= 0 && lastKey < 128) ? keys.test ((size_t) lastKey) : false;
        if (auto* iface = findParentComponentOfClass<SynthGuiInterface>())
            if (auto* synth = iface->getSynth())
                if (auto* eng = synth->getEngine())
                {
                    if (isDown)
                        eng->postUINoteOn (lastKey, 1.0f, 1);
                    else
                        eng->postUINoteOff (lastKey, 0.0f, 1);
                }
    }

    // Live external MIDI -> display only, no injection back to engine
    void midiNoteChanged (int note, bool isDown, int /*channel*/, float /*velocity01*/) override
    {
        setLiveKeyState (note, isDown);
        redoImage();
    }

private:
    void tryRegisterLiveListener()
    {
        if (registeredLive_) return;
        if (auto* iface = findParentComponentOfClass<SynthGuiInterface>())
            if (auto* synth = iface->getSynth())
                if (auto* eng = synth->getEngine())
                {
                    eng->addMidiLiveListener (this);
                    registeredLive_ = true;
                }
    }

    void tryUnregisterLiveListener()
    {
        if (! registeredLive_) return;
        if (auto* iface = findParentComponentOfClass<SynthGuiInterface>())
            if (auto* synth = iface->getSynth())
                if (auto* eng = synth->getEngine())
                    eng->removeMidiLiveListener (this);
        registeredLive_ = false;
    }

    KeymapKeyboardState& _params;
    bool registeredLive_ { false };
};

#endif //BITKLAVIER0_OPENGL_KEYMAPKEYBOARD_H
