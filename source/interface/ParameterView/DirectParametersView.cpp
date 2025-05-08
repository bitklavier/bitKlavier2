//
// Created by Dan Trueman on 11/5/24.
//

#include "DirectParametersView.h"


void DirectParametersView::resized()
{

    int knob_section_height = getKnobSectionHeight();
    int knob_y = getHeight() - knob_section_height;

    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int area_width = getWidth() - 2 * title_width;
    int envelope_height = knob_y - widget_margin;

    juce::Rectangle<int> knobs_area(title_width, knob_y, area_width, knob_section_height);
    knobs_area.setSize(knobs_area.getWidth() , knobs_area.getHeight() );
    placeKnobsInArea(knobs_area, _sliders);

    envSection->setBounds(title_width, knob_section_height, area_width, knob_section_height * 2);

//     transpose_uses_tuning->setBounds(area_width - title_width, 0, 100, knob_section_height);
    transpositionSlider->setBounds(title_width,0,   area_width, knob_section_height );
    velocityMinMaxSlider->setBounds(0,envSection->getBottom(),area_width,40 );


    SynthSection::resized();
}