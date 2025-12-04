//
// Created by Dan Trueman on 7/26/25.
//

#include "BlendronicParametersView.h"

void BlendronicParametersView::timerCallback()
{
    /*
     * for updating the currently active sliders in the UI
     */

    if(!beatLengthsSlider->hovering_)
    {
        beatLengthsSlider->setCurrentSlider(bparams_.beatLengths_current.load());
        beatLengthsSlider->redoImage();
    }
    if(!delayLengthsSlider->hovering_)
    {
        delayLengthsSlider->setCurrentSlider (bparams_.delayLengths_current.load());
        delayLengthsSlider->redoImage();
    }
    if(!smoothingTimesSlider->hovering_)
    {
        smoothingTimesSlider->setCurrentSlider (bparams_.smoothingTimes_current.load());
        smoothingTimesSlider->redoImage();
    }
    if(!feedbackCoeffsSlider->hovering_)
    {
        feedbackCoeffsSlider->setCurrentSlider (bparams_.feedbackCoeffs_current.load());
        feedbackCoeffsSlider->redoImage();
    }
}

void BlendronicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);
    bounds.removeFromLeft(smallpadding);
    inLevelMeter->setBounds(bounds.removeFromLeft(title_width));

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);

    bounds.removeFromRight(smallpadding);
    sendLevelMeter->setBounds(bounds.removeFromRight(title_width));

    bounds.reduce(largepadding, largepadding);

    // how much vertical space will we need for all the components?
    float sliderHeightScale = 2.;
    int verticalAreaNeeded = knob_section_height * 4 * sliderHeightScale;

    /*
     * todo: better spacing for these...
     */
    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 5;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    bounds.removeFromTop(bufferSpaceForEach);

    beatLengthsSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));
    bounds.removeFromTop(bufferSpaceForEach);
    delayLengthsSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));
    bounds.removeFromTop(bufferSpaceForEach);
    smoothingTimesSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));
    bounds.removeFromTop(bufferSpaceForEach);
    feedbackCoeffsSlider->setBounds(bounds.removeFromTop(knob_section_height * sliderHeightScale));

    SynthSection::resized();
}