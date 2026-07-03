// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ChucKlavierProcessor.h"
#include "ChucKCodeTokeniser.h"
#include "ChucKKnobPanel.h"
#include "FullInterface.h"
#include "Identifiers.h"
#include "open_gl_code_editor.h"
#include "peak_meter_section.h"
#include "synth_button.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_image_component.h"

class ChucKlavierParametersView : public SynthSection,
                                  public juce::Timer,
                                  public juce::CodeDocument::Listener
{
public:
    ChucKlavierParametersView (chowdsp::PluginState& pluginState,
                               ChucKlavierParams& params,
                               juce::String uuid,
                               const juce::ValueTree& prepVT,
                               OpenGlWrapper* open_gl,
                               SynthBase* synth = nullptr,
                               juce::AudioProcessorGraph::NodeID nodeId = {},
                               ChucKlavierProcessor* proc = nullptr)
        : SynthSection (""),
          cparams_ (params),
          prepVT_ (prepVT),
          synth_ (synth),
          nodeId_ (nodeId),
          proc_ (proc),
          opengl_ (open_gl)
    {
        setName ("chucklavier");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (uuid);
        setSkinOverride (Skin::kDirect);

        prepTitle = std::make_shared<PlainTextComponent> (getName(), getName());
        addOpenGlComponent (prepTitle);
        prepTitle->setJustification (juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        FullInterface* parent = findParentComponentOfClass<FullInterface>();
        if (parent)
            parent->hideSoundsetSelector();

        auto& listeners = pluginState.getParameterListeners();

        levelMeter = std::make_unique<PeakMeterSection> (uuid, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel ("Main");
        levelMeter->setVolumeTooltip ("Overall output volume of ChucKlavier");
        addSubSection (levelMeter.get());

        sendLevelMeter = std::make_unique<PeakMeterSection> (uuid, params.outputSend, listeners, &params.sendLevels);
        sendLevelMeter->setLabel ("Send");
        sendLevelMeter->setVolumeTooltip ("Volume of ChucKlavier output to connected effects");
        addSubSection (sendLevelMeter.get());

        inLevelMeter = std::make_unique<PeakMeterSection> (uuid, params.inputGain, listeners, &params.inputLevels);
        inLevelMeter->setLabel ("Internal");
        inLevelMeter->setVolumeTooltip ("Level of audio bussed in from other preparations");
        addSubSection (inLevelMeter.get());

        externalLevelMeter = std::make_unique<PeakMeterSection> (uuid, params.externalGain, listeners, &params.externalLevels);
        externalLevelMeter->setLabel ("External");
        externalLevelMeter->setVolumeTooltip ("Level of external input (mic/line in standalone, sidechain in plugin)");
        addSubSection (externalLevelMeter.get());

        // Code editor — must add to hierarchy BEFORE any call that triggers resized(),
        // since resized() calls findValue() on parent_ which asserts if parent_ is null.
        tokeniser_ = std::make_unique<ChucKCodeTokeniser>();
        scriptEditor = std::make_unique<OpenGlCodeEditor> (scriptDoc_, tokeniser_.get());
        addAndMakeVisible (scriptEditor.get());
        addOpenGlComponent (scriptEditor->getImageComponent());

        scriptEditor->setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
        scriptEditor->setColourScheme (tokeniser_->getDefaultColourScheme());
        scriptEditor->setColour (juce::CodeEditorComponent::backgroundColourId, juce::Colour (0xdd000000));
        scriptEditor->setColour (juce::CodeEditorComponent::highlightColourId, juce::Colours::darkgreen.withAlpha (0.5f));
        scriptEditor->setLineNumbersShown (true);

        // Load saved script (or default) into the document before installing the listener,
        // so the initial replaceAllContent does not trigger a spurious ValueTree write.
        {
            juce::String savedScript = prepVT_.getProperty (IDs::chuckScript, "");
            if (savedScript.isEmpty())
                savedScript = getDefaultScript();
            scriptDoc_.replaceAllContent (savedScript);
        }
        scriptDoc_.addListener (this);

        // Status label — shows compile result or error under the editor.
        // Must be a PlainTextComponent (OpenGL-rendered) — plain juce::Label is
        // invisible underneath the OpenGL canvas.
        statusLabel = std::make_shared<PlainTextComponent> ("chuckStatus", "VM ready");
        statusLabel->setFontType (PlainTextComponent::kRegular);
        statusLabel->setTextSize (11.0f);
        statusLabel->setJustification (juce::Justification::centredLeft);
        statusLabel->setColor (juce::Colours::lightgrey);
        addOpenGlComponent (statusLabel);

        // Send to VM button
        sendScriptButton = std::make_unique<SynthButton> ("sendScript");
        sendScriptButton->setText ("Send to VM");
        sendScriptButton->setTooltip ("Compile and hot-swap the script into the ChucK VM");
        sendScriptButton->setClickingTogglesState (false);
        addSynthButton (sendScriptButton.get(), true);
        sendScriptButton->onClick = [this] { requestScriptSwap (scriptDoc_.getAllContent()); };

        muteButton_ = std::make_unique<SynthButton> ("mute");
        muteButton_->setText ("M");
        muteButton_->setTooltip ("Mute this preparation. Option-click to mute only this one.");
        addSynthButton (muteButton_.get(), true);
        muteButton_->onClick = [this] () {
            bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
            bool newMuted = !cparams_.userMuted_.load (std::memory_order_relaxed);
            cparams_.userMuted_.store (newMuted, std::memory_order_relaxed);
            cparams_.muted_.store (newMuted || cparams_.soloMuted_.load (std::memory_order_relaxed),
                                   std::memory_order_relaxed);
            if (synth_) synth_->coordinateMuteChanged (nodeId_, newMuted, isOptionClick);
        };

        soloButton_ = std::make_unique<SynthButton> ("solo");
        soloButton_->setText ("S");
        soloButton_->setTooltip ("Solo this preparation. Option-click to solo only this one.");
        addSynthButton (soloButton_.get(), true);
        soloButton_->onClick = [this] () {
            bool isOptionClick = juce::ModifierKeys::currentModifiers.isAltDown();
            bool newSoloed = !cparams_.soloed_.load (std::memory_order_relaxed);
            cparams_.soloed_.store (newSoloed, std::memory_order_relaxed);
            if (synth_) synth_->coordinateSoloChanged (nodeId_, isOptionClick);
        };

        startTimer (50);
    }

    // Poll mute/solo state at 50ms; hot-swap timer is handled separately via hotSwapTimer_.
    void timerCallback() override
    {
        bool soloed    = cparams_.soloed_.load (std::memory_order_relaxed);
        bool userMuted = cparams_.userMuted_.load (std::memory_order_relaxed);
        bool soloMuted = cparams_.soloMuted_.load (std::memory_order_relaxed);
        soloButton_->setToggleState (soloed, juce::dontSendNotification);
        if (soloMuted && !userMuted)
        {
            bool blinkPhase = (juce::Time::getMillisecondCounter() / 300) % 2;
            muteButton_->setToggleState (blinkPhase, juce::dontSendNotification);
        }
        else
        {
            muteButton_->setToggleState (userMuted, juce::dontSendNotification);
        }
    }

    void stopAllTimers() override
    {
        stopTimer();
        hotSwapTimer_.stopTimer();
        if (scriptEditor) scriptEditor->stopTimer();
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont (g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    void resized() override;

    std::shared_ptr<PlainTextComponent> prepTitle;

    std::unique_ptr<PeakMeterSection> levelMeter;
    std::unique_ptr<PeakMeterSection> sendLevelMeter;
    std::unique_ptr<PeakMeterSection> inLevelMeter;
    std::unique_ptr<PeakMeterSection> externalLevelMeter;

    juce::CodeDocument                    scriptDoc_;
    std::unique_ptr<ChucKCodeTokeniser>   tokeniser_;
    std::unique_ptr<OpenGlCodeEditor>     scriptEditor;
    std::shared_ptr<PlainTextComponent>   statusLabel;
    std::unique_ptr<SynthButton>         sendScriptButton;
    std::unique_ptr<SynthButton>      muteButton_;
    std::unique_ptr<SynthButton>      soloButton_;

    ~ChucKlavierParametersView() override
    {
        scriptDoc_.removeListener (this);
    }

private:
    // CodeDocument::Listener — write script content back to the ValueTree on every edit,
    // and dismiss the error highlight colour so it doesn't linger while the user types.
    void codeDocumentTextInserted (const juce::String&, int) override
    {
        prepVT_.setProperty (IDs::chuckScript, scriptDoc_.getAllContent(), nullptr);
        if (scriptEditor)
            scriptEditor->setColour (juce::CodeEditorComponent::highlightColourId,
                                     juce::Colours::darkgreen.withAlpha (0.5f));
    }
    void codeDocumentTextDeleted (int, int) override
    {
        prepVT_.setProperty (IDs::chuckScript, scriptDoc_.getAllContent(), nullptr);
        if (scriptEditor)
            scriptEditor->setColour (juce::CodeEditorComponent::highlightColourId,
                                     juce::Colours::darkgreen.withAlpha (0.5f));
    }

    static juce::String getDefaultScript()
    {
        return juce::String (ChucKlavierProcessor::kDefaultScript);
    }

    void requestScriptSwap (const juce::String& newScript)
    {
        if (proc_ == nullptr) return;
        pendingScript_ = newScript;  // NOLINT: pendingScript_ updated before vmSuspended_
        proc_->vmParked_.store (false, std::memory_order_relaxed);
        proc_->vmSuspended_.store (true, std::memory_order_release);
        statusLabel->setColor (juce::Colours::yellow);
        statusLabel->setText (juce::String::fromUTF8 ("Compiling\xe2\x80\xa6"));
        sendScriptButton->setEnabled (false);
        hotSwapTimer_.startTimer (5);
    }

    void setStatusOk (const juce::String& msg)
    {
        statusLabel->setColor (juce::Colours::lightgreen);
        statusLabel->setText (msg);
        sendScriptButton->setEnabled (true);
        if (scriptEditor)
            scriptEditor->setColour (juce::CodeEditorComponent::highlightColourId,
                                     juce::Colours::darkgreen.withAlpha (0.5f));
        rebuildKnobPanel();
    }

    // Rebuild (or create) the knob panel from the current processor slot state.
    void rebuildKnobPanel()
    {
        if (proc_ == nullptr || opengl_ == nullptr) return;

        if (knobPanel_ != nullptr)
        {
            knobPanel_->refresh();
        }
        else
        {
            const juce::String uuid = getComponentID();
            knobPanel_ = std::make_unique<ChucKKnobPanel> (*proc_, uuid, *opengl_);
            addSubSection (knobPanel_.get());
        }

        resized();
        repaint();
    }

    void setStatusError (const juce::String& msg)
    {
        statusLabel->setColor (juce::Colours::orange);
        statusLabel->setText (msg);
        sendScriptButton->setEnabled (true);
        if (scriptEditor)
        {
            scriptEditor->setColour (juce::CodeEditorComponent::highlightColourId,
                                     juce::Colours::red.withAlpha (0.4f));
            highlightErrorLine (parseErrorLine (msg));
        }
    }

    // Highlight the given 1-based line in the editor and scroll it into view.
    void highlightErrorLine (int line1Based)
    {
        if (scriptEditor == nullptr) return;
        if (line1Based < 1 || line1Based > scriptDoc_.getNumLines()) return;
        juce::CodeDocument::Position lineStart (scriptDoc_, line1Based - 1, 0);
        int col = scriptDoc_.getLine (line1Based - 1).trimEnd().length();
        if (col == 0) col = 1;
        juce::CodeDocument::Position lineEnd (scriptDoc_, line1Based - 1, col);
        scriptEditor->setHighlightedRegion (juce::Range<int> (lineStart.getPosition(), lineEnd.getPosition()));
        scriptEditor->scrollToLine (line1Based - 1);
        // setHighlightedRegion queues an async line-token rebuild via rebuildLineTokensAsync().
        // callAsync posts to the end of the message queue — after the rebuild — so redoImage()
        // paints the GL texture only once the selection is baked into the line data.
        auto* ed = scriptEditor.get();
        juce::MessageManager::callAsync ([ed] { ed->redoImage(); });
    }

    // Parse the line number from a ChucK error string (1-based), or -1 if not found.
    // ChucK uses "<compiled.code>" as filename when compiling inline code, producing:
    //   "<compiled.code>:LINE:COL: message"
    static int parseErrorLine (const juce::String& msg)
    {
        // Primary: "<compiled.code>:LINE:COL: ..."
        auto idx = msg.indexOf ("<compiled.code>:");
        if (idx >= 0)
        {
            int line = msg.substring (idx + 16).getIntValue();
            if (line > 0) return line;
        }
        // Fallback: "[chuck:LINE:COL]: ..." (alternate ChucK code path)
        idx = msg.indexOf ("[chuck:");
        if (idx >= 0)
        {
            int line = msg.substring (idx + 7).getIntValue();
            if (line > 0) return line;
        }
        // Last resort: first ":N:" pattern (handles any filename)
        auto colon = msg.indexOfChar (':');
        if (colon >= 0)
        {
            int line = msg.substring (colon + 1).getIntValue();
            if (line > 0) return line;
        }
        return -1;
    }

    // Separate juce::Timer for hot-swap so it can run at 5ms without conflicting
    // with the 50ms mute/solo poll on the outer timer.
    struct HotSwapTimer : public juce::Timer
    {
        explicit HotSwapTimer (ChucKlavierParametersView& owner) : owner_ (owner) {}

        void timerCallback() override;  // implemented in .cpp (includes chuck.h)

        int tickCount_ = 0;
        ChucKlavierParametersView& owner_;
    };

    HotSwapTimer hotSwapTimer_ { *this };

    ChucKlavierParams&                    cparams_;
    juce::ValueTree                       prepVT_;
    SynthBase*                            synth_   = nullptr;
    juce::AudioProcessorGraph::NodeID     nodeId_;
    ChucKlavierProcessor*                 proc_    = nullptr;
    OpenGlWrapper*                        opengl_  = nullptr;
    juce::String                          pendingScript_;
    std::unique_ptr<ChucKKnobPanel>       knobPanel_;
};
