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

#include "fonts.h"

Fonts::Fonts() :
    proportional_regular_(juce::Font(juce::Font::plain)),
    proportional_light_(juce::Font(juce::Font::plain)),
    proportional_title_(juce::Font(juce::Font::plain)),
    proportional_title_regular_(juce::Font(juce::Font::plain)),
    monospace_(juce::Font(juce::Font::plain)) {
//
//  juce::Array<int> glyphs;
//  juce::Array<float> x_offsets;
//  proportional_regular_.getGlyphPositions("", glyphs, x_offsets);
//  proportional_light_.getGlyphPositions("test", glyphs, x_offsets);
//  proportional_title_.getGlyphPositions("test", glyphs, x_offsets);
//  monospace_.getGlyphPositions("test", glyphs, x_offsets);
}
