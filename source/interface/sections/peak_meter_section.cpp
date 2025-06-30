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
    VolumeSlider(juce::String name) : SynthSlider(name)
    {
        paintToImage(true);
    }

    void paint(juce::Graphics& g) override {

        // draw cursor at current value location
        float y = getPositionOfValue(getValue());
        juce::Path arrow;
        arrow.startNewSubPath(8, y);
        int arrow_height = 8;
        arrow.lineTo(1, y + arrow_height / 2.0f);
        arrow.lineTo(1, y - arrow_height / 2.0f);
        arrow.closeSubPath();
        g.setColour(findColour(Skin::kLinearSliderThumb, true));
        g.fillPath(arrow);

        // draw log tick marks
        g.setColour(findColour(Skin::kLightenScreen, true));
        for (int decibel = 6; decibel >= -66; decibel -= 6) {
            int y = getPositionOfValue(decibel);
            g.drawRect((getWidth() / 2) - 4, y, 8, 2);
        }

        // draw bigger tick mark at 0 dBFS
        g.setColour(findColour(Skin::kLinearSliderThumb, true));
        g.drawRect((getWidth() / 2) - 6, (int)getPositionOfValue(0), 12, 2);
    }

//    void valueChanged() override {
//        //gainChangeDBFS->setParameterValue(getValue());
//    }
};

PeakMeterSection::PeakMeterSection(
    juce::String name,
    chowdsp::FloatParameter& outGainDB,
    chowdsp::ParameterListeners& listeners,
    const std::tuple<std::atomic<float>, std::atomic<float>> *outputLevels)
    : SynthSection(name)
{

   setComponentID(name); // sets the UUID for this component, inherits the UUID from the owning preparation

   peak_meter_left_ = std::make_shared<PeakMeterViewer>(true, outputLevels);
   addOpenGlComponent(peak_meter_left_);
   peak_meter_right_ = std::make_shared<PeakMeterViewer>(false, outputLevels);
   addOpenGlComponent(peak_meter_right_);

   volume_ = std::make_shared<VolumeSlider>("volume");
   volumeAttach_ = std::make_unique<chowdsp::SliderAttachment>(outGainDB, listeners, *volume_, nullptr);
   addSlider(volume_.get());
   volume_->setSliderStyle(juce::Slider::LinearBarVertical);
   volume_->setRange(-80.0, 6.0);
   volume_->setNumDecimalPlacesToDisplay(2);
   volume_->setValue(0.0, juce::dontSendNotification);
   volume_->setSkewFactor(2.0);
   volume_->setPopupPlacement(juce::BubbleComponent::right);
   volume_->setTextValueSuffix(" dBFS");
   volume_->setDoubleClickReturnValue(true, 0.0);
}

PeakMeterSection::~PeakMeterSection() { }

void PeakMeterSection::resized() {

   juce::Rectangle<int> bounds = getLocalBounds();
   volume_->setBounds(bounds);

   juce::Rectangle<int> leftMeterBounds = bounds.removeFromLeft(bounds.getWidth()/2);
   leftMeterBounds.reduce(2, 0);
   bounds.reduce(2, 0);
   peak_meter_left_->setBounds(leftMeterBounds);
   peak_meter_right_->setBounds(bounds);

   SynthSection::resized();
}

void PeakMeterSection::paintBackground(juce::Graphics& g) {
    SynthSection::paintChildrenBackgrounds(g);
}
