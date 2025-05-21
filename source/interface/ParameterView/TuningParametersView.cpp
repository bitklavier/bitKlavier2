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
    SynthSection::resized();
}