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
    proc->vmSuspended_.store (false, std::memory_order_release);

    if (ok)
        owner_.setStatusOk (proc->lastCompileMessage);
    else
        owner_.setStatusError (proc->lastCompileMessage);
}

void ChucKlavierParametersView::resized()
{
    const int titleWidth    = getTitleWidth();
    const int smallPadding  = findValue (Skin::kPadding);
    const int largePadding  = findValue (Skin::kLargePadding);
    const int buttonHeight  = 22;
    const int statusHeight  = 18;

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

    // Remaining area: script editor + status label + Send button
    bounds.reduce (largePadding, largePadding);

    // Send to VM button at the bottom
    auto buttonArea = bounds.removeFromBottom (buttonHeight);
    sendScriptButton->setBounds (buttonArea);
    bounds.removeFromBottom (smallPadding);

    // Status label above Send button
    auto statusArea = bounds.removeFromBottom (statusHeight);
    statusLabel->setBounds (statusArea);
    bounds.removeFromBottom (smallPadding);

    // Script editor fills the rest
    if (bounds.getHeight() > 0)
        scriptEditor->setBounds (bounds);

    SynthSection::resized();
}
