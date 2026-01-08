//
// Created by Myra Norton on 12/22/25.
//

#include "BKCompressorMeter.h"

BKCompressorMeter::BKCompressorMeter(CompressorParams *_params) :
params(_params)
{
    //Init vars
    startAngle = static_cast<float>(5.0f / 3.0f * juce::MathConstants<float>::pi);
    endAngle = static_cast<float>(7.0f / 3.0f * juce::MathConstants<float>::pi);

    meterBg.prepare(startAngle, endAngle);
    needle.prepare(startAngle, endAngle);

    addAndMakeVisible(meterBg);
    addAndMakeVisible(needle);

    backgroundDarkGrey = juce::Colour(meterBg.bg_DarkGrey);
}

void BKCompressorMeter::paint(juce::Graphics& g)
{
    g.setColour(backgroundDarkGrey);
    //g.fillRoundedRectangle(getLocalBounds().toFloat(), 3);
}

void BKCompressorMeter::resized()
{
    auto bounds = getLocalBounds();
    meterBg.setBounds(bounds);
    needle.setBounds(bounds);
}

void BKCompressorMeter::update(const float& val)
{
    if (val != valueInDecibel)
    {
        needle.update(val);
    }

    triggerAsyncUpdate();
}

float BKCompressorMeter::getValue()
{
    return valueInDecibel;
}

void BKCompressorMeter::setGUIEnabled(bool state)
{
    needle.setNeedleEnabled(state);
    meterBg.setIndicatorEnabled(state);
}
