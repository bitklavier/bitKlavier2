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
#include <juce_graphics/juce_graphics.h>

class Fonts {
  public:
    virtual ~Fonts() { }

    juce::Font& proportional_regular() { return proportional_regular_; }
    juce::Font& proportional_light() { return proportional_light_; }
    juce::Font& proportional_title() { return proportional_title_; }
    juce::Font& proportional_title_regular() { return proportional_title_regular_; }
    juce::Font& monospace() { return monospace_; }

    static Fonts* instance() {
      static Fonts instance;
      return &instance;
    }

  private:
    Fonts();

    juce::Font proportional_regular_;
    juce::Font proportional_light_;
    juce::Font proportional_title_;
    juce::Font proportional_title_regular_;
    juce::Font monospace_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fonts)
};

