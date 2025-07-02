//
// Created by Dan Trueman on 11/5/24.
//

#include "TuningParametersView.h"


void TuningParametersView::resized()
{
    int knob_section_height = getKnobSectionHeight();
    int knob_y = getHeight() - knob_section_height;

    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int area_width = getWidth() - 2 * title_width;
    int envelope_height = knob_y - widget_margin;
    adaptive_combo_box->setBounds(10,10, 100, 25);
    tuning_combo_box->setBounds(115,10,100, 25);
    fundamental_combo_box->setBounds(225, 10, 50,25);
    keyboard->setBounds(50, 200, 500, 100);
    circular_keyboard->setBounds(50, 50, 500, 100);

    //semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 50, getKnobSectionHeight() + 50 + getTextComponentHeight() * 2);
    semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 105, getKnobSectionHeight() + 10);

    juce::Rectangle<int> outputKnobsArea = {50, semitoneSection->getBottom() + knob_section_height, 100, 100};
        //bounds.removeFromTop(knob_section_height);
    placeKnobsInArea(outputKnobsArea, _sliders, true);



    SynthSection::resized();
}
void TuningParametersView::keyboardSliderChanged(juce::String name) {

    if (name == "circular")
        params.tuningSystem->setParameterValue(TuningSystem::Custom);
}
