// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "default_look_and_feel.h"

class CurveLookAndFeel : public DefaultLookAndFeel {
  public:
    virtual ~CurveLookAndFeel() { }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float slider_t, float start_angle, float end_angle,
                          juce::Slider& slider) override;

    void drawCurve(juce::Graphics& g, juce::Slider& slider, int x, int y, int width, int height, bool active, bool bipolar);

    static CurveLookAndFeel* instance() {
      static CurveLookAndFeel instance;
      return &instance;
    }

  private:
    CurveLookAndFeel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveLookAndFeel)
};

