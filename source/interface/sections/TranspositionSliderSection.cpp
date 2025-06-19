//
// Created by Dan Trueman on 6/19/25.
//

#include "TranspositionSliderSection.h"

void TranspositionSliderSection::resized()
{
    slider->setBounds(0, 0, getWidth(), getHeight());
    slider->redoImage();

    int onWidth = 100;
    int onHeight = 20;
    on->setBounds(getWidth() - onWidth, getHeight() - onHeight, onWidth, onHeight);
    on->toFront(false);

    SynthSection::resized();
}