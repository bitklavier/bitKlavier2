//
// Created by Myra Norton on 11/14/25.
//

#include "CompressorParameterView.h"

void CompressorParameterView::timerCallback()
{
    compressorMeter->update (compressorParams_.maxGainReduction.load());
}

void CompressorParameterView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int labelsectionheight = findValue(Skin::kLabelHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeSmall);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);
    bounds.removeFromLeft(smallpadding);

    inLevelMeter->setBounds(bounds.removeFromLeft(title_width));
    bounds.removeFromRight(title_width*.85);
    levelMeter->setBounds(bounds.removeFromRight(title_width));

    bounds.removeFromTop (largepadding * 6);
    bounds.reduce(largepadding * 6, largepadding);

    auto presets_rect = bounds.removeFromTop (knobsectionheight * 1.);
    presets_rect.reduce(presets_rect.getWidth() / 5., 0);
    presetsBorder->setBounds (presets_rect);
    presets_rect.reduce(largepadding, largepadding * 1.5);
    presets_rect.removeFromTop (largepadding);

    activeCompressor_toggle->setBounds(presets_rect.removeFromLeft(presets_rect.getWidth() / 3.));
    reset_button->setBounds(presets_rect.removeFromRight(presets_rect.getWidth() / 2.));
    presets_rect.reduce(largepadding, 0);
    presets_combo_box->setBounds(presets_rect);

    bounds.removeFromTop (largepadding);
    auto topHalf = bounds.removeFromTop (bounds.getHeight()/1.75);
    topHalf.removeFromLeft (200);
    topHalf.removeFromRight (200);
    compressorMeter->setBounds(topHalf);
    bounds.removeFromTop (20);

    auto knobSection = bounds.removeFromTop (knobsectionheight*1.7);
    compressorControlsBorder->setBounds (knobSection);
    knobSection.removeFromTop (knobsectionheight/3.);
    knobSection.removeFromBottom (knobsectionheight/3.);

    // Padding between knobs
    constexpr int colPadding = 30;

    // Total width available
    const int totalWidth = knobSection.getWidth();
    const int colWidth = (totalWidth - 5 * colPadding) / 6;

    attack_knob->setBounds(knobSection.removeFromLeft(colWidth));
    knobSection.removeFromLeft (colPadding);
    release_knob->setBounds(knobSection.removeFromLeft(colWidth));
    knobSection.removeFromLeft (colPadding);
    threshold_knob->setBounds(knobSection.removeFromLeft(colWidth));
    knobSection.removeFromLeft (colPadding);
    makeup_knob->setBounds(knobSection.removeFromLeft(colWidth));
    knobSection.removeFromLeft (colPadding);
    // mix_knob->setBounds(knobSection.removeFromLeft(colWidth));
    // knobSection.removeFromLeft (colPadding);
    ratio_knob->setBounds(knobSection.removeFromLeft(colWidth));
    knobSection.removeFromLeft (colPadding);
    knee_knob->setBounds(knobSection.removeFromLeft(colWidth));
    knobSection.removeFromLeft (colPadding);

    juce::Rectangle<int> attack_label_rect (attack_knob->getX(), attack_knob->getBottom() - largepadding, attack_knob->getWidth(), labelsectionheight );
    attack_knob_label->setBounds(attack_label_rect);
    juce::Rectangle<int> release_label_rect (release_knob->getX(), release_knob->getBottom() - largepadding, release_knob->getWidth(), labelsectionheight );
    release_knob_label->setBounds(release_label_rect);
    juce::Rectangle<int> threshold_label_rect (threshold_knob->getX(), threshold_knob->getBottom() - largepadding, threshold_knob->getWidth(), labelsectionheight );
    threshold_knob_label->setBounds(threshold_label_rect);
    juce::Rectangle<int> makeup_label_rect (makeup_knob->getX(), makeup_knob->getBottom() - largepadding, makeup_knob->getWidth(), labelsectionheight );
    makeup_knob_label->setBounds(makeup_label_rect);
    // juce::Rectangle<int> mix_label_rect (mix_knob->getX(), mix_knob->getBottom() - largepadding, mix_knob->getWidth(), labelsectionheight );
    // mix_knob_label->setBounds(mix_label_rect);
    juce::Rectangle<int> ratio_label_rect (ratio_knob->getX(), ratio_knob->getBottom() - largepadding, ratio_knob->getWidth(), labelsectionheight );
    ratio_knob_label->setBounds(ratio_label_rect);
    juce::Rectangle<int> knee_label_rect (knee_knob->getX(), knee_knob->getBottom() - largepadding, knee_knob->getWidth(), labelsectionheight );
    knee_knob_label->setBounds(knee_label_rect);

    attack_knob_label->setTextSize (knobLabelSize);
    release_knob_label->setTextSize (knobLabelSize);
    threshold_knob_label->setTextSize (knobLabelSize);
    makeup_knob_label->setTextSize (knobLabelSize);
    // mix_knob_label->setTextSize (knobLabelSize);
    ratio_knob_label->setTextSize (knobLabelSize);
    knee_knob_label->setTextSize (knobLabelSize);

    SynthSection::resized();
}