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

PeakMeterSection::PeakMeterSection(juce::String name, const std::tuple<std::atomic<float>, std::atomic<float>> *outputLevels) : SynthSection(name) {
   peak_meter_left_ = std::make_shared<PeakMeterViewer>(true, outputLevels);
   addOpenGlComponent(peak_meter_left_);
   peak_meter_right_ = std::make_shared<PeakMeterViewer>(false, outputLevels);
   addOpenGlComponent(peak_meter_right_);
}

PeakMeterSection::~PeakMeterSection() { }

int PeakMeterSection::getMeterHeight() {
   return getHeight() / 8;
}

int PeakMeterSection::getBuffer() {
   return getHeight() / 2 - getMeterHeight();
}

void PeakMeterSection::resized() {
//   int meter_height = getMeterHeight();
//   int volume_height = meter_height * 6.0f;
//   int end_volume = meter_height * 3.5f;
//   int padding = 1;
//   int buffer = getBuffer();
//
//   peak_meter_left_->setBounds(0, buffer, getWidth(), meter_height);
//   peak_meter_right_->setBounds(0, peak_meter_left_->getBottom() + padding, getWidth(), meter_height);

   juce::Rectangle<int> bounds = getLocalBounds();
   bounds.reduce(0, 20);
   juce::Rectangle<int> leftMeterBounds = bounds.removeFromLeft(bounds.getWidth()/2);
   leftMeterBounds.reduce(2, 0);
   bounds.reduce(2, 0);

   peak_meter_left_->setBounds(leftMeterBounds);
   peak_meter_right_->setBounds(bounds);

   SynthSection::resized();
}

void PeakMeterSection::paintBackground(juce::Graphics& g) {
   SynthSection::paintKnobShadows(g);
   SynthSection::paintChildrenBackgrounds(g);

   int ticks_y = peak_meter_right_->getBottom() + getPadding();
   int tick_height = peak_meter_right_->getHeight() / 2;
   //vital::ValueDetails details = vital::Parameters::getDetails("volume");

   g.setColour(findColour(Skin::kLightenScreen, true));
   for (int decibel = -66; decibel <= 6; decibel += 6) {
       //float offset = decibel - details.post_offset;
       float offset = decibel;
       //float percent = offset * offset / (details.max - details.min);
       float percent = offset * offset;
       int x = percent * getWidth();
       g.drawRect(x, ticks_y, 1, tick_height);
   }
}
