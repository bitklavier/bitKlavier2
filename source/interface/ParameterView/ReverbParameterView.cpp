// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReverbParameterView.h"
#include "FullInterface.h"

void ReverbParameterView::resized()
{
    if (auto* parent = findParentComponentOfClass<FullInterface>())
        parent->hideSoundsetSelector();

    const int title_width       = getTitleWidth();
    const int smallpadding      = findValue (Skin::kPadding);
    const int largepadding      = findValue (Skin::kLargePadding);
    const int labelsectionheight = findValue (Skin::kLabelHeight);
    const int knobsectionheight = findValue (Skin::kKnobSectionHeight);
    const float knobLabelSize   = findValue (Skin::kKnobLabelSizeSmall);

    // Rotated title on the far left
    prepTitle->setBounds (getLocalBounds().removeFromLeft (title_width));
    prepTitle->setTextSize (findValue (Skin::kPrepTitleSize));

    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft (title_width + smallpadding);

    // Level meters on left and right edges
    if (isPrepVersion_ && externalLevelMeter)
        externalLevelMeter->setBounds (bounds.removeFromLeft (title_width));
    inLevelMeter->setBounds (bounds.removeFromLeft (title_width));
    bounds.removeFromRight  (int (title_width * 0.85f));
    levelMeter->setBounds   (bounds.removeFromRight (title_width));
    if (isPrepVersion_ && sendLevelMeter)
        sendLevelMeter->setBounds (bounds.removeFromRight (title_width));

    bounds.removeFromTop (largepadding * 6);
    bounds.reduce (largepadding * 6, largepadding);

    // ── Power / Presets section ──────────────────────────────────────
    auto presets_rect = bounds.removeFromTop (int (knobsectionheight * 1.0));
    presets_rect.reduce (presets_rect.getWidth() / 5, 0);
    presetsBorder->setBounds (presets_rect);
    presets_rect.reduce (largepadding, int (largepadding * 1.5));
    presets_rect.removeFromTop (largepadding);

    const int btnGap = 4;
    const int btnW   = (presets_rect.getWidth() - btnGap) / 2;
    activeReverb_toggle->setBounds (presets_rect.removeFromLeft (btnW));
    presets_rect.removeFromLeft (btnGap);
    presetsButton->setBounds       (presets_rect);

    bounds.removeFromTop (largepadding);

    // ── Two rows of knob groups ──────────────────────────────────────
    const int rowH = int (knobsectionheight * 1.7);
    auto row1 = bounds.removeFromTop (rowH);
    bounds.removeFromTop (largepadding);
    auto row2 = bounds.removeFromTop (rowH);

    // Row 1: groups 1–3 (4 knobs each)
    const int g1w = (row1.getWidth() - largepadding * 2) / 3;
    auto g1r = row1.removeFromLeft (g1w);
    row1.removeFromLeft (largepadding);
    auto g2r = row1.removeFromLeft (g1w);
    row1.removeFromLeft (largepadding);
    auto g3r = row1;

    group1Border->setBounds (g1r);
    group2Border->setBounds (g2r);
    group3Border->setBounds (g3r);

    // Row 2: groups 4–5 (3 knobs each)
    const int g2w = (row2.getWidth() - largepadding) / 2;
    auto g4r = row2.removeFromLeft (g2w);
    row2.removeFromLeft (largepadding);
    auto g5r = row2;

    group4Border->setBounds (g5r);  // lo group on left
    group5Border->setBounds (g4r);  // hi group on right

    // ── Helper: lay out N knobs inside a group rect ──────────────────
    auto layoutKnobs = [&] (juce::Rectangle<int> r, int n,
                             std::initializer_list<SynthSlider*>         knobs,
                             std::initializer_list<PlainTextComponent*>  labels)
    {
        r.reduce (largepadding, knobsectionheight / 3);
        const int colPad = 12;
        const int colW   = (r.getWidth() - (n - 1) * colPad) / n;

        auto kit = knobs.begin();
        auto lit = labels.begin();
        for (int i = 0; i < n; ++i, ++kit, ++lit)
        {
            auto col = r.removeFromLeft (colW);
            if (i < n - 1) r.removeFromLeft (colPad);
            (*kit)->setBounds (col);
            (*lit)->setBounds ({ col.getX(), col.getBottom() - largepadding,
                                 col.getWidth(), labelsectionheight });
            (*lit)->setTextSize (knobLabelSize);
        }
    };

    layoutKnobs (g1r, 4,
        { dryLevel_knob.get(), earlyLevel_knob.get(), earlySend_knob.get(), lateLevel_knob.get() },
        { dryLevel_label.get(), earlyLevel_label.get(), earlySend_label.get(), lateLevel_label.get() });

    layoutKnobs (g2r, 4,
        { diffuse_knob.get(), modulation_knob.get(), spin_knob.get(), wander_knob.get() },
        { diffuse_label.get(), modulation_label.get(), spin_label.get(), wander_label.get() });

    layoutKnobs (g3r, 4,
        { size_knob.get(), width_knob.get(), predelay_knob.get(), decay_knob.get() },
        { size_label.get(), width_label.get(), predelay_label.get(), decay_label.get() });

    layoutKnobs (g5r, 3,
        { highCut_knob.get(), highXover_knob.get(), highMult_knob.get() },
        { highCut_label.get(), highXover_label.get(), highMult_label.get() });

    layoutKnobs (g4r, 3,
        { lowCut_knob.get(), lowXover_knob.get(), lowMult_knob.get() },
        { lowCut_label.get(), lowXover_label.get(), lowMult_label.get() });

    SynthSection::resized();
}
