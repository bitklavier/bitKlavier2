// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKlavierParametersView.h"

// VERSION is defined as a preprocessor macro by the JUCE build system ("5.1.0").
// chuck.h has a member `static std::string VERSION` which the macro corrupts.
// Undef before including.
#undef VERSION
#include "chuck.h"
#include "chuck_errmsg.h"

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

    // AT has parked — safe to compile on the message thread now.
    stopTimer();
    tickCount_ = 0;

    auto* vm = proc->getVM();
    if (vm == nullptr)
    {
        proc->vmSuspended_.store (false, std::memory_order_release);
        owner_.setStatusError ("VM not initialised");
        return;
    }

    // Compile first — only remove old shreds on success so failures leave the
    // previous script running.
    bool ok = vm->compileCode (owner_.pendingScript_.toStdString(), "",
                               /*count=*/ 1, /*immediate=*/ false);
    if (ok)
    {
        vm->removeAllShreds();
        vm->compileCode (owner_.pendingScript_.toStdString(), "",
                         /*count=*/ 1, /*immediate=*/ true);
        proc->vmSuspended_.store (false, std::memory_order_release);
        owner_.setStatusOk ("Compiled OK");
    }
    else
    {
        proc->vmSuspended_.store (false, std::memory_order_release);
        juce::String errMsg = EM_lasterror();
        if (errMsg.isEmpty()) errMsg = "Compile error";
        owner_.setStatusError (errMsg);
    }
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
