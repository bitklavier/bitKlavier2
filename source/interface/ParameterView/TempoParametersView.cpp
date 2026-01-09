//
// Created by Myra Norton on 8/20/25.
//

#include "TempoParametersView.h"

void TempoParametersView::resized()
{
    FullInterface *parent = findParentComponentOfClass<FullInterface>();
    if (parent)
        parent->hideSoundsetSelector();

    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();
    int labelsectionheight = findValue(Skin::kLabelHeight);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeSmall);
    auto comboboxheight = findValue(Skin::kComboMenuHeight);
    int large_padding = findValue(Skin::kLargePadding);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width + large_padding);
    bounds.removeFromRight(title_width);
    bounds.removeFromTop(large_padding * 2);

    // main params
    juce::Rectangle<int> primaryKnobsBounds = bounds.removeFromTop(knob_section_height + large_padding);
    primaryKnobsBorder->setBounds(primaryKnobsBounds);
    primaryKnobsBounds.reduce (large_padding, large_padding);
    juce::FlexBox fbl;
    fbl.flexDirection = juce::FlexBox::Direction::row;
    fbl.alignContent = juce::FlexBox::AlignContent::stretch;
    fbl.alignItems = juce::FlexBox::AlignItems::stretch;
    fbl.items.add(juce::FlexItem(*tempoMode)
        .withFlex(1.0f) // This keeps the width flexible
        .withHeight(primaryKnobsBorder->getHeight() * 0.7) // Set your desired height here
        .withAlignSelf(juce::FlexItem::AlignSelf::flexEnd)); // bottom align
    for (auto& slider : _sliders)
    {
        fbl.items.add(juce::FlexItem(*slider).withFlex(1.0f));
    }
    fbl.performLayout(primaryKnobsBounds);

    bounds.removeFromTop(large_padding * 4);

    // adaptive params
    juce::Rectangle<int> adaptiveKnobsBounds = bounds.removeFromTop(knob_section_height + large_padding * 3);
    adaptiveKnobsBorder->setBounds(adaptiveKnobsBounds);
    adaptiveKnobsBounds.reduce (large_padding, large_padding);
    juce::FlexBox fba;
    fba.flexDirection = juce::FlexBox::Direction::row;
    fba.alignContent = juce::FlexBox::AlignContent::stretch;
    fba.alignItems = juce::FlexBox::AlignItems::stretch;
    fba.items.add(juce::FlexItem(*historySlider).withFlex(1.0f));
    fba.items.add(juce::FlexItem(*timeWindowMinMaxSlider)
        .withFlex(1.0f) // This keeps the width flexible
        .withHeight(adaptiveKnobsBorder->getHeight() * 0.7) // Set your desired height here
        .withAlignSelf(juce::FlexItem::AlignSelf::flexEnd)); // bottom align
    fba.performLayout(adaptiveKnobsBounds);

    // labels
    int sl_counter = 0;
    for (auto& slider : _sliders)
    {
        juce::Rectangle<int> sl_label_rect (slider->getX(), slider->getBottom() - 10, slider->getWidth(), labelsectionheight );
        slider_labels[sl_counter]->setBounds(sl_label_rect);
        slider_labels[sl_counter++]->setTextSize (knobLabelSize);
    }

    juce::Rectangle<int> history_label_rect (historySlider->getX(), historySlider->getY() + historySlider->getHeight() / 2 + 20, historySlider->getWidth(), labelsectionheight );
    historyLabel->setBounds(history_label_rect);
    historyLabel->setTextSize (knobLabelSize);

    SynthSection::resized();
}