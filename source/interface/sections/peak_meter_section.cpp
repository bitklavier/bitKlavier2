/* Copyright 2013-2019 Matt Tytel
*
* vital is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* vital is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with vital.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "peak_meter_section.h"
#include "fonts.h"
#include "skin.h"
#include "synth_slider.h"

class VolumeSlider : public SynthSlider {
public:
    VolumeSlider(juce::String name, const juce::ValueTree& mod_param) : SynthSlider(name,mod_param)
    {
        paintToImage(true);
    }

    void paint(juce::Graphics& g) override {

        // draw cursor at current value location
        float pos = getPositionOfValue(getValue());

        if (getSliderStyle() == juce::Slider::LinearBar)
        {
            juce::Path arrow;
            arrow.startNewSubPath(pos, 8);
            int arrow_width = 8;
            arrow.lineTo(pos - arrow_width / 2.0f, 1);
            arrow.lineTo(pos + arrow_width / 2.0f, 1);
            arrow.closeSubPath();
            g.setColour(findColour(Skin::kLinearSliderThumb, true));
            g.fillPath(arrow);

            // draw bigger tick mark at 0 dBFS
            g.setColour(findColour(Skin::kLinearSliderThumb, true));
            float x = getPositionOfValue(0);
            float y = (getHeight() / 2) - 1;
            float tickWidth = 2;
            float tickHeight = 8;
            g.drawRect(x - tickWidth/2, y - tickHeight/2, tickWidth, tickHeight);
            // g.drawRect( (int)getPositionOfValue(0), (getWidth() / 2) - 6, 12, 2);
        }
        else
        {
            juce::Path arrow;
            arrow.startNewSubPath(8, pos);
            int arrow_height = 8;
            arrow.lineTo(1, pos + arrow_height / 2.0f);
            arrow.lineTo(1, pos - arrow_height / 2.0f);
            arrow.closeSubPath();
            g.setColour(findColour(Skin::kLinearSliderThumb, true));
            g.fillPath(arrow);

            // draw bigger tick mark at 0 dBFS
            g.setColour(findColour(Skin::kLinearSliderThumb, true));
            g.drawRect((getWidth() / 2) - 6, (int)getPositionOfValue(0), 12, 2);
        }

        // draw log tick marks
        g.setColour(findColour(Skin::kLightenScreen, true));
        for (int decibel = 6; decibel >= -66; decibel -= 6) {

            int pos = getPositionOfValue(decibel);
            if (getSliderStyle() == juce::Slider::LinearBar)
                g.drawRect(pos, (getHeight() / 2) - 4, 2, 8);
            else
                g.drawRect((getWidth() / 2) - 4, pos, 8, 2);
        }
        // g.addTransform(juce::AffineTransform::rotation(-bitklavier::kPi / 2.0f));
    }
};

PeakMeterSection::PeakMeterSection(
    juce::String name,
    chowdsp::FloatParameter& outGainDB,
    chowdsp::ParameterListeners& listeners,
    const std::tuple<std::atomic<float>, std::atomic<float>> *outputLevels,
    bool horizontal)
    : SynthSection(name), horizontal_(horizontal)
{

   setComponentID(name); // sets the UUID for this component, inherits the UUID from the owning preparation

   peak_meter_left_ = std::make_shared<PeakMeterViewer>(true, outputLevels, horizontal_);
   addOpenGlComponent(peak_meter_left_);
   peak_meter_right_ = std::make_shared<PeakMeterViewer>(false, outputLevels, horizontal_);
   addOpenGlComponent(peak_meter_right_);

   volume_ = std::make_shared<VolumeSlider>(outGainDB.paramID,outGainDB.getModParam());
   volumeAttach_ = std::make_unique<chowdsp::SliderAttachment>(outGainDB, listeners, *volume_, nullptr);
   addSlider(volume_.get());
   volume_->addAttachment(volumeAttach_.get());
   volume_->setNumDecimalPlacesToDisplay(2);
   volume_->setPopupPlacement(juce::BubbleComponent::right);
   volume_->setDoubleClickReturnValue(true, 0.0);



    if (horizontal_)
    {
        volume_->setSliderStyle(juce::Slider::LinearBar);
        // peak_meter_label->setRotation(0);
    }
    else
    {
        volume_->setSliderStyle(juce::Slider::LinearBarVertical);
        peak_meter_label = std::make_shared<PlainTextComponent>("peak_meter", "Gain");
        addOpenGlComponent(peak_meter_label);
        peak_meter_label->setTextSize (12.0f);
        peak_meter_label->setJustification(juce::Justification::centred);
        peak_meter_label->setRotation(-90);
    }
}

PeakMeterSection::~PeakMeterSection() { }

void PeakMeterSection::setLabel(juce::String newLabel)
{
    peak_meter_label->setText(newLabel);
}

void PeakMeterSection::setColor(juce::Colour color)
{
    peak_meter_label->setColor (color);
}

void PeakMeterSection::resized() {

   juce::Rectangle<int> bounds = getLocalBounds();
    juce::Rectangle<int> labelRect;
    if (horizontal_)
    {
        // labelRect = bounds.removeFromLeft(50);
        // peak_meter_label->setBounds(labelRect);
        volume_->setBounds(bounds);
        volume_->setPopupPlacement (juce::BubbleComponent::above);
        juce::Rectangle<int> leftMeterBounds = bounds.removeFromTop(bounds.getHeight()/2);
        leftMeterBounds.reduce(0, 2);
        bounds.reduce(0, 2);
        peak_meter_left_->setBounds(leftMeterBounds);
        peak_meter_right_->setBounds(bounds);
    }
    else
    {
        labelRect = bounds.removeFromTop(50);
        peak_meter_label->setBounds(labelRect);
        volume_->setBounds(bounds);
        juce::Rectangle<int> leftMeterBounds = bounds.removeFromLeft(bounds.getWidth()/2);
        leftMeterBounds.reduce(2, 0);
        bounds.reduce(2, 0);
        peak_meter_left_->setBounds(leftMeterBounds);
        peak_meter_right_->setBounds(bounds);
    }

   SynthSection::resized();
}

void PeakMeterSection::paintBackground(juce::Graphics& g) {
    SynthSection::paintChildrenBackgrounds(g);
}
