//
// Created by Dan Trueman on 8/5/25.
//

#include "SynchronicParametersView.h"
void SynchronicParametersView::timerCallback()
{
    /*
     * for updating the currently active sliders in the UI
     */
//    beatLengthsSlider->setCurrentSlider(bparams_.beatLengths_current.load());
//    delayLengthsSlider->setCurrentSlider(bparams_.delayLengths_current.load());
//    smoothingTimesSlider->setCurrentSlider(bparams_.smoothingTimes_current.load());
//    feedbackCoeffsSlider->setCurrentSlider(bparams_.feedbackCoeffs_current.load());
//
//    beatLengthsSlider->redoImage();
//    delayLengthsSlider->redoImage();
//    smoothingTimesSlider->redoImage();
//    feedbackCoeffsSlider->redoImage();
}

void SynchronicParametersView::resized()
{
    // width of the title at left, used in all preparations
    int title_width = getTitleWidth();

    // height for most of these components
    int knob_section_height = getKnobSectionHeight();

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);

    // get the prep area, with left/right border for title
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.removeFromLeft(title_width);

    // bounds for level meter on right side
    juce::Rectangle<int> meterArea = bounds.removeFromRight(title_width);
    levelMeter->setBounds(meterArea);

    bounds.removeFromRight(smallpadding);
    sendLevelMeter->setBounds(bounds.removeFromRight(title_width));

    bounds.reduce(largepadding, largepadding);

    // how much vertical space will we need for all the components?
    int verticalAreaNeeded = knob_section_height * 4;

    /*
     * todo: better spacing for these...
     */
    // how much vertical space is left, divided up so we have some buffer space between each component
    int bufferSpaceForEach = (bounds.getHeight() - verticalAreaNeeded) / 5;
    if (bufferSpaceForEach < 0 ) bufferSpaceForEach = 0;

    bounds.removeFromTop(bufferSpaceForEach);

    clusterMinMaxSlider->setBounds(bounds.removeFromTop(knob_section_height));

//    beatLengthsSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
//    bounds.removeFromTop(bufferSpaceForEach);
//    delayLengthsSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
//    bounds.removeFromTop(bufferSpaceForEach);
//    smoothingTimesSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));
//    bounds.removeFromTop(bufferSpaceForEach);
//    feedbackCoeffsSlider->setBounds(bounds.removeFromTop(knob_section_height * 1.5));

    SynthSection::resized();
}