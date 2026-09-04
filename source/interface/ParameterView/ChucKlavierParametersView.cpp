// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierParametersView.h"

// No ChucK headers needed here — EM_lasterror() is read inside doHotSwap()
// (ChucKlavierProcessor.cpp) while the temporary VM is still alive.

void ChucKlavierParametersView::HotSwapTimer::timerCallback()
{
    auto* proc = owner_.proc_;
    if (proc == nullptr)
    {
        stopTimer();
        owner_.setStatusError ("No processor");
        return;
    }

    if (!proc->vmParked_.load (std::memory_order_acquire))
    {
        ++tickCount_;
        // Give up after ~200ms (40 × 5ms) if processor is bypassed / not running.
        if (tickCount_ > 40)
        {
            stopTimer();
            tickCount_ = 0;
            proc->vmSuspended_.store (false, std::memory_order_release);
            owner_.setStatusError ("Timeout waiting for VM to park");
        }
        return;
    }

    // AT has parked — safe to rebuild the VM on the message thread now.
    stopTimer();
    tickCount_ = 0;

    // doHotSwap builds a fresh ChucK VM and compiles into it, then replaces vm_.
    // A fresh VM is required because UGen graph connections (e.g. adc => dac)
    // persist after removeAllShreds() — rebuilding guarantees a clean graph.
    bool ok = proc->doHotSwap (owner_.pendingScript_.toStdString());

    if (ok)
    {
        // Parse //@knob annotations and reconcile the stable slot->name table,
        // then cache t_CKFLOAT* pointers from the fresh VM.
        // Must run before vmSuspended_ is cleared so AT stays silent while we
        // read global pointers from the just-built VM.
        proc->reconcileSlots (owner_.pendingScript_);
    }

    proc->vmSuspended_.store (false, std::memory_order_release);

    if (ok)
        owner_.setStatusOk (proc->lastCompileMessage);
    else
        owner_.setStatusError (proc->lastCompileMessage);
}

void ChucKlavierParametersView::resized()
{
    // Lazy-create the knob panel on the first resize with valid bounds.
    // This ensures the panel is created only after the component is in the display
    // hierarchy (constructor-time creation fails because bounds are zero).
    if (knobPanel_ == nullptr && proc_ != nullptr && opengl_ != nullptr
        && getWidth() > 0 && getHeight() > 0)
    {
        knobPanel_ = std::make_unique<ChucKKnobPanel> (*proc_, getComponentID(), *opengl_);
        addSubSection (knobPanel_.get());
    }

    const int titleWidth    = getTitleWidth();
    const int smallPadding  = findValue (Skin::kPadding);
    const int largePadding  = findValue (Skin::kLargePadding);
    const int buttonHeight  = 22;
    const int statusHeight  = 36;

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft (titleWidth);
    prepTitle->setBounds (titleArea);
    prepTitle->setTextSize (findValue (Skin::kPrepTitleSize));

    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft (titleWidth);
    bounds.removeFromLeft (smallPadding);
    bounds.removeFromTop (8);
    bounds.removeFromBottom (20);

    // Left group: External then Internal meters
    externalLevelMeter->setBounds (bounds.removeFromLeft (titleWidth));
    bounds.removeFromLeft (smallPadding);
    inLevelMeter->setBounds (bounds.removeFromLeft (titleWidth));

    // Right group: Main then Send meters
    juce::Rectangle<int> meterArea = bounds.removeFromRight (titleWidth);
    levelMeter->setBounds (meterArea);
    bounds.removeFromRight (smallPadding);
    sendLevelMeter->setBounds (bounds.removeFromRight (titleWidth));

    // M/S buttons below right meters
    {
        int muteLeft  = sendLevelMeter->getX();
        int muteRight = levelMeter->getRight();
        int muteTop   = levelMeter->getBottom() + smallPadding;
        int muteH     = getLocalBounds().getBottom() - muteTop - smallPadding;
        if (muteH > 0)
        {
            constexpr int kGap = 2;
            int totalW = muteRight - muteLeft;
            int halfW  = (totalW - kGap) / 2;
            muteButton_->setBounds (muteLeft, muteTop, halfW, muteH);
            soloButton_->setBounds (muteLeft + halfW + kGap, muteTop, totalW - halfW - kGap, muteH);
        }
    }

    // Remaining area: 3/4 editor, 1/4 knob panel.
    bounds.reduce (largePadding, largePadding);

    if (knobPanel_ != nullptr)
        knobPanel_->setBounds (bounds.removeFromRight (bounds.getWidth() / 4));

    // Button row: [Send to VM] [Console] [Open in Window], bottom flush with meter.
    const int meterBottom = levelMeter->getBottom();
    {
        constexpr int kConsoleW = 64;
        constexpr int kWindowW  = 112;
        constexpr int kBtnGap   = 4;
        const int rightW = kConsoleW + kBtnGap + kWindowW;
        const int sendW  = bounds.getWidth() - rightW - kBtnGap;
        const int btnY   = meterBottom - buttonHeight;
        sendScriptButton->setBounds (bounds.getX(), btnY, sendW, buttonHeight);
        int x = bounds.getX() + sendW + kBtnGap;
        if (consoleButton_ != nullptr)
        {
            consoleButton_->setBounds (x, btnY, kConsoleW, buttonHeight);
            x += kConsoleW + kBtnGap;
        }
        if (openInWindowButton_ != nullptr)
            openInWindowButton_->setBounds (x, btnY, kWindowW, buttonHeight);
    }

    const int statusTop = meterBottom - buttonHeight - smallPadding - statusHeight;
    statusLabel->setBounds (bounds.getX(), statusTop, bounds.getWidth(), statusHeight);
    const int editorBottom = statusTop - smallPadding;

    // Find bar sits at the very top of the editor area when visible.
    constexpr int kFindH   = 22;
    constexpr int kFindGap = 4;
    int editorTop = bounds.getY();
    if (findBarVisible_ && findField_ != nullptr)
    {
        findField_->setBounds (bounds.getX(), editorTop, bounds.getWidth(), kFindH);
        editorTop += kFindH + kFindGap;
    }

    if (editorBottom > editorTop)
        scriptEditor->setBounds (juce::Rectangle<int> (bounds.getX(), editorTop, bounds.getWidth(), editorBottom - editorTop));

    SynthSection::resized();
}
