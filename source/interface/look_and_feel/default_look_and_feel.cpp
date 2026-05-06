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

#include "default_look_and_feel.h"
#include "skin.h"
#include "fonts.h"
#include "paths.h"

#include "synth_section.h"

DefaultLookAndFeel::DefaultLookAndFeel() {
  setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff111111));
  setColour(juce::PopupMenu::textColourId, juce::Colour(0xffcccccc));
  setColour(juce::PopupMenu::headerTextColourId, juce::Colour(0xffffffff));
  setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff8458b7));
  setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffffff));
  setColour(juce::BubbleComponent::backgroundColourId, juce::Colour(0xff111111));
  setColour(juce::BubbleComponent::outlineColourId, juce::Colour(0xff333333));
  setColour(juce::TooltipWindow::textColourId, juce::Colour(0xffdddddd));
}

void DefaultLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& text_editor) {
  if (width <= 0 || height <= 0)
    return;

  float rounding = 5.0f;
  SynthSection* parent = text_editor.findParentComponentOfClass<SynthSection>();
  if (parent)
    rounding = parent->findValue(Skin::kWidgetRoundedCorner);

  g.setColour(text_editor.findColour(Skin::kTextEditorBackground, true));
  g.fillRoundedRectangle(0, 0, width, height, rounding);
  g.setColour(text_editor.findColour(Skin::kTextEditorBorder, true));
  g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, rounding, 1.0f);
}

void DefaultLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
  g.setColour(findColour(juce::PopupMenu::backgroundColourId));
  g.fillRoundedRectangle(0, 0, width, height, kPopupMenuBorder);
  g.setColour(findColour(juce::BubbleComponent::outlineColourId));
  g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, kPopupMenuBorder, 1.0f);
}

void DefaultLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scroll_bar, int x, int y, int width, int height,
                                       bool vertical, int thumb_position, int thumb_size,
                                       bool mouse_over, bool mouse_down) {
  if (thumb_size >= height)
    return;

  bool right_aligned = dynamic_cast<LeftAlignedScrollBar*>(&scroll_bar) == nullptr;

  int draw_width = width / 2 - 2;
  if (mouse_down || mouse_over)
    draw_width = width - 2;

  int draw_times = 2;
  if (mouse_down)
    draw_times = 4;

  int draw_x = 1;
  if (right_aligned)
    draw_x = width - 1 - draw_width;

  g.setColour(scroll_bar.findColour(Skin::kLightenScreen, true));
  for (int i = 0 ; i < draw_times; ++i)
    g.fillRoundedRectangle(draw_x, thumb_position, draw_width, thumb_size, draw_width / 2.0f);
}

void DefaultLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, const bool button_down,
                                      int button_x, int button_y, int button_w, int button_h, juce::ComboBox& box) {
  static constexpr float kRoundness = 4.0f;
  g.setColour(findColour(juce::BubbleComponent::backgroundColourId));
  g.fillRoundedRectangle(box.getLocalBounds().toFloat(), kRoundness);
  juce::Path path = Paths::downTriangle();

  g.setColour(box.findColour(Skin::kTextComponentText, true));
  juce::Rectangle<int> arrow_bounds = box.getLocalBounds().removeFromRight(height);
  g.fillPath(path, path.getTransformToScaleToFit(arrow_bounds.toFloat(), true));
}

void DefaultLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component& component,
                                     float x, float y, float w, float h, bool ticked,
                                     bool enabled, bool mouse_over, bool button_down) {
  static constexpr float kBorderPercent = 0.15f;
  if (ticked)
    g.setColour(component.findColour(Skin::kIconButtonOn, true));
  else
    g.setColour(component.findColour(Skin::kLightenScreen, true));

  float border_width = h * kBorderPercent;
  g.fillRect(x + border_width, y + border_width, w - 2 * border_width, h - 2 * border_width);
}

void DefaultLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox& call_out_box, juce::Graphics& g, const juce::Path& path, juce::Image&) {
  g.setColour(call_out_box.findColour(Skin::kBody, true));
  g.fillPath(path);

  g.setColour(call_out_box.findColour(Skin::kPopupBorder, true));
  g.strokePath(path, juce::PathStrokeType(1.0f));
}

void DefaultLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& background_color,
                                              bool hover, bool down) {
  g.setColour(button.findColour(Skin::kPopupSelectorBackground, true));
  g.fillRoundedRectangle(button.getLocalBounds().toFloat(), 5.0f);
}

//int DefaultLookAndFeel::getSliderPopupPlacement(juce::Slider& slider) {
//  SynthSlider* s_slider = dynamic_cast<SynthSlider*>(&slider);
//  if (s_slider)
//    return s_slider->getPopupPlacement();
//
//  return juce::LookAndFeel_V3::getSliderPopupPlacement(slider);
//}

juce::Font DefaultLookAndFeel::getPopupMenuFont() {
  return Fonts::instance()->proportional_regular().withPointHeight(14.0f);
}

juce::Font DefaultLookAndFeel::getSliderPopupFont(juce::Slider& slider) {
  return Fonts::instance()->proportional_regular().withPointHeight(14.0f);
}
