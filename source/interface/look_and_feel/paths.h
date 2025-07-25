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
#include "utils.h"
#include "BinaryData.h"

class Paths {
  public:
    static constexpr int kLogoWidth = 1701;

    Paths() = delete;

    static juce::Path fromSvgData(const void* data, size_t data_size) {
      std::unique_ptr<juce::Drawable> drawable(juce::Drawable::createFromImageData(data, data_size));
      return drawable->getOutlineAsPath();
    }

    static juce::Path fromPngData(const void* data, size_t data_size) {
      std::unique_ptr<juce::Drawable> drawable(juce::Drawable::createFromImageData(data, data_size));
      return drawable->getOutlineAsPath();
    }

    // Returns the paths for a keymap preparation window
    static juce::Array<juce::Path> keymapPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_keymap_svg,BinaryData::Layer_1_keymap_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_keymap_svg,BinaryData::Layer_2_keymap_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_3_keymap_svg,BinaryData::Layer_3_keymap_svgSize));

        return arr;
    }

    static juce::Array<juce::Path> portPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::port_outline_svg,BinaryData::port_outline_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::port_fill_svg,BinaryData::port_fill_svgSize));

        return arr;
    }
    // Returns the paths for a nostalgic preparation window
    static juce::Array<juce::Path> nostalgicPaths()
    {
      juce::Array<juce::Path> arr;
      arr.add(fromSvgData((const void*)BinaryData::Layer_1_nostalgic_svg,BinaryData::Layer_1_nostalgic_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_2_nostalgic_svg,BinaryData::Layer_2_nostalgic_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_3_nostalgic_svg,BinaryData::Layer_3_nostalgic_svgSize));

      return arr;
    }

    // Returns the paths for a direct preparation window
    static juce::Array<juce::Path> directPaths()
    {
      juce::Array<juce::Path> arr;
      arr.add(fromSvgData((const void*)BinaryData::Layer_1_direct_svg,BinaryData::Layer_1_direct_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_2_direct_svg,BinaryData::Layer_2_direct_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_3_direct_svg,BinaryData::Layer_3_direct_svgSize));

      return arr;
    }

    // Returns the paths for a synchronic preparation window
    static juce::Array<juce::Path> synchronicPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_synchronic_svg,BinaryData::Layer_1_synchronic_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_synchronic_svg,BinaryData::Layer_2_synchronic_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_3_synchronic_svg,BinaryData::Layer_3_synchronic_svgSize));

        return arr;
    }

    // Returns the paths for a blendronic preparation window
    static juce::Array<juce::Path> blendronicPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_blendronic_svg,BinaryData::Layer_1_blendronic_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_blendronic_svg,BinaryData::Layer_2_blendronic_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_3_blendronic_svg,BinaryData::Layer_3_blendronic_svgSize));

        return arr;
    }
  // Returns the paths for a vst preparation window
  static juce::Array<juce::Path> vstPaths()
    {
      juce::Array<juce::Path> arr;
      arr.add(fromSvgData((const void*)BinaryData::Layer_1_vst_svg,BinaryData::Layer_1_vst_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_2_vst_svg,BinaryData::Layer_2_vst_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_3_vst_svg,BinaryData::Layer_3_vst_svgSize));
      arr.add(fromSvgData((const void*)BinaryData::Layer_4_vst_svg,BinaryData::Layer_4_vst_svgSize));

      return arr;
    }

    /**
     * todo replace vst icon stuff here
     * @return
     */
    static juce::Array<juce::Path> midiFilterPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_vst_svg,BinaryData::Layer_1_vst_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_vst_svg,BinaryData::Layer_2_vst_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_3_vst_svg,BinaryData::Layer_3_vst_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_4_vst_svg,BinaryData::Layer_4_vst_svgSize));

        return arr;
    }

    // Returns the paths for a resonance preparation window
    static juce::Array<juce::Path> resonancePaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_resonance_svg,BinaryData::Layer_1_resonance_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_resonance_svg,BinaryData::Layer_2_resonance_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_3_resonance_svg,BinaryData::Layer_3_resonance_svgSize));

        return arr;
    }

    // Returns the paths for a tuning preparation window
    static juce::Array<juce::Path> tuningPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_tuning_svg,BinaryData::Layer_1_tuning_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_tuning_svg,BinaryData::Layer_2_tuning_svgSize));

        return arr;
    }

    // Returns the paths for a tempo preparation window
    static juce::Array<juce::Path> tempoPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::Layer_1_tempo_svg,BinaryData::Layer_1_tempo_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::Layer_2_tempo_svg,BinaryData::Layer_2_tempo_svgSize));

        return arr;
    }

    static juce::Array<juce::Path> logoPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(fromSvgData((const void*)BinaryData::layer_1_logo_svg,BinaryData::layer_1_logo_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::layer_2_logo_svg,BinaryData::layer_2_logo_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::layer_3_logo_svg,BinaryData::layer_3_logo_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::layer_4_logo_svg,BinaryData::layer_4_logo_svgSize));
        arr.add(fromSvgData((const void*)BinaryData::layer_5_logo_svg,BinaryData::layer_5_logo_svgSize));

        return arr;
    }

    static juce::Array<juce::Path> modulationPaths()
    {
        juce::Array<juce::Path> arr;
        arr.add(upTriangle());
        arr.add(upTriangle());
        arr.add(upTriangle());
        arr.add(upTriangle());
        return arr;
    }

    static juce::Path prev() {
      static const juce::PathStrokeType arrow_stroke(0.1f, juce::PathStrokeType::JointStyle::curved,
                                               juce::PathStrokeType::EndCapStyle::rounded);

      juce::Path prev_line, prev_shape;
      prev_line.startNewSubPath(0.65f, 0.3f);
      prev_line.lineTo(0.35f, 0.5f);
      prev_line.lineTo(0.65f, 0.7f);

      arrow_stroke.createStrokedPath(prev_shape, prev_line);
      prev_shape.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      prev_shape.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return prev_shape;
    }

    static juce::Path next() {
      static const juce::PathStrokeType arrow_stroke(0.1f, juce::PathStrokeType::JointStyle::curved,
                                               juce::PathStrokeType::EndCapStyle::rounded);

      juce::Path next_line, next_shape;
      next_line.startNewSubPath(0.35f, 0.3f);
      next_line.lineTo(0.65f, 0.5f);
      next_line.lineTo(0.35f, 0.7f);

      arrow_stroke.createStrokedPath(next_shape, next_line);
      next_shape.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      next_shape.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return next_shape;
    }

    static juce::Path clock() {
      static const float kClockAngle = 1.0f;
      static const float kClockWidth = 0.4f;
      static const float kBuffer = (1.0f - kClockWidth) / 2.0f;

      juce::Path path;
      path.addPieSegment(kBuffer, kBuffer, kClockWidth, kClockWidth, 0.0f, kClockAngle - 2.0f * bitklavier::kPi, 0.0f);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path dragDropArrows() {
      return fromSvgData((const void*)BinaryData::drag_drop_arrows_svg, BinaryData::drag_drop_arrows_svgSize);
    }

    static juce::Path note() {
      static constexpr float kLeftAdjustment = 1.0f / 32.0f;
      static constexpr float kNoteWidth = 1.0f / 4.0f;
      static constexpr float kNoteHeight = 1.0f / 5.0f;
      static constexpr float kBarHeight = 2.0f / 5.0f;
      static constexpr float kBarWidth = 1.0f / 20.0f;
      static constexpr float kY = 1.0f - (1.0f - kBarHeight + kNoteHeight / 2.0f) / 2.0f;
      static constexpr float kX = (1.0f - kNoteWidth) / 2.0f - kLeftAdjustment;

      juce::Path path;
      path.addEllipse(kX, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.closeSubPath();

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path tripletNotes() {
      static constexpr float kNoteWidth = 1.0f / 5.0f;
      static constexpr float kNoteHeight = 1.0f / 6.0f;
      static constexpr float kX = (1.0f - 3.0f * kNoteWidth) / 2.0f;
      static constexpr float kBarWidth = 1.0f / 20.0f;
      static constexpr float kBarHeight = 1.0f / 4.0f;
      static constexpr float kY = 1.0f - (1.0f - kBarHeight) / 2.0f;

      juce::Path path;
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight - kBarWidth,
                        2.0f * kNoteWidth + kBarWidth, kBarWidth);

      path.addEllipse(kX, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.addEllipse(kX + kNoteWidth, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth + kNoteWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.addEllipse(kX + 2.0f * kNoteWidth, kY - kNoteHeight / 2.0f, kNoteWidth, kNoteHeight);
      path.addRectangle(kX + kNoteWidth - kBarWidth + 2.0f * kNoteWidth, kY - kBarHeight, kBarWidth, kBarHeight);
      path.closeSubPath();

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path menu() {
      juce::Path path;
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(0.2f, 0.3f, 0.8f, 0.3f), 0.05f);
      path.addLineSegment(juce::Line<float>(0.2f, 0.5f, 0.8f, 0.5f), 0.05f);
      path.addLineSegment(juce::Line<float>(0.2f, 0.7f, 0.8f, 0.7f), 0.05f);
      return path;
    }

    static juce::Path menu(int width) {
      static constexpr float kBuffer = 0.2f;
      static constexpr float kLineWidth = 0.04f;
      static constexpr float kSpacing = 0.2f;

      int buffer = std::round(width * kBuffer);
      int line_width = std::max<int>(1, width * kLineWidth);
      int spacing = width * kSpacing;

      float center = (line_width % 2) * 0.5f + (width / 2);
      juce::Path path;
      path.addLineSegment(juce::Line<float>(buffer, center - spacing, width - buffer, center - spacing), line_width);
      path.addLineSegment(juce::Line<float>(buffer, center, width - buffer, center), line_width);
      path.addLineSegment(juce::Line<float>(buffer, center + spacing, width - buffer, center + spacing), line_width);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static juce::Path plus(int width) {
      static constexpr float kBuffer = 0.35f;
      static constexpr float kLineWidth = 0.04f;

      int buffer = std::round(width * kBuffer);
      int line_width = std::max<int>(1, width * kLineWidth);

      float center = (line_width % 2) * 0.5f + (width / 2);
      juce::Path path;
      path.addLineSegment(juce::Line<float>(buffer, center, width - buffer, center), line_width);
      path.addLineSegment(juce::Line<float>(center, buffer, center, width - buffer), line_width);

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static juce::Path plusOutline() {
      static constexpr float kBuffer = 0.2f;
      static constexpr float kLineWidth = 0.15f;

      float start = kBuffer;
      float end = 1.0f - kBuffer;
      float close = 0.5f - kLineWidth * 0.5f;
      float far = 0.5f + kLineWidth * 0.5f;

      juce::Path path;
      path.startNewSubPath(start, close);
      path.lineTo(start, far);
      path.lineTo(close, far);
      path.lineTo(close, end);
      path.lineTo(far, end);
      path.lineTo(far, far);
      path.lineTo(end, far);
      path.lineTo(end, close);
      path.lineTo(far, close);
      path.lineTo(far, start);
      path.lineTo(close, start);
      path.lineTo(close, close);
      path.lineTo(start, close);
      path.closeSubPath();

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path save(int width) {
      static constexpr float kLineWidth = 0.04f;
      static constexpr float kSpacingX = 0.3f;
      static constexpr float kSpacingY = 0.25f;
      static constexpr float kArrowMovement = 0.14f;
      static constexpr float kBoxWrap = 0.07f;

      int line_width = std::max<int>(1, width * kLineWidth);
      int spacing_x = width * kSpacingX;
      int spacing_y = width * kSpacingY;
      float movement = width * kArrowMovement / 2.0f;
      float wrap = width * kBoxWrap;

      float center = (line_width % 2) * 0.5f + (width / 2);

      juce::Path outline;
      outline.startNewSubPath(center, center - spacing_y);
      outline.lineTo(center, center + movement);

      outline.startNewSubPath(center - 2 * movement, center - movement);
      outline.lineTo(center, center + movement);
      outline.lineTo(center + 2 * movement, center - movement);

      outline.startNewSubPath(center - spacing_x + wrap, center);
      outline.lineTo(center - spacing_x, center);
      outline.lineTo(center - spacing_x, center + spacing_y);
      outline.lineTo(center + spacing_x, center + spacing_y);
      outline.lineTo(center + spacing_x, center);
      outline.lineTo(center + spacing_x - wrap, center);

      juce::Path path;
      juce::PathStrokeType stroke(line_width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(width, width, width, width), 0.2f);
      return path;
    }

    static juce::Path downTriangle() {
      juce::Path path;

      path.startNewSubPath(0.33f, 0.4f);
      path.lineTo(0.66f, 0.4f);
      path.lineTo(0.5f, 0.6f);
      path.closeSubPath();

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path upTriangle() {
      juce::Path path;

      path.startNewSubPath(0.0f, 1.0f);
      path.lineTo(1.f, 1.0f);
      path.lineTo(0.5f, 0.f);
      path.closeSubPath();

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path exitX() {
      juce::Path outline;
      outline.startNewSubPath(0.25f, 0.25f);
      outline.lineTo(0.75f, 0.75f);
      outline.startNewSubPath(0.25f, 0.75f);
      outline.lineTo(0.75f, 0.25f);

      juce::Path path;
      juce::PathStrokeType stroke(0.03f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path thickX() {
      juce::Path outline;
      outline.startNewSubPath(0.25f, 0.25f);
      outline.lineTo(0.75f, 0.75f);
      outline.startNewSubPath(0.25f, 0.75f);
      outline.lineTo(0.75f, 0.25f);

      juce::Path path;
      juce::PathStrokeType stroke(0.1f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path star() {
      juce::Path path;
      path.addStar(juce::Point<float>(0.5f, 0.5f), 5, 0.2f, 0.4f);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path keyboard() {
      juce::Path path;
      path.addLineSegment(juce::Line<float>(0.2f, 0.2f, 0.2f, 0.2f), 0.01f);
      path.addLineSegment(juce::Line<float>(0.8f, 0.8f, 0.8f, 0.8f), 0.01f);

      path.startNewSubPath(0.24f, 0.35f);
      path.lineTo(0.24f, 0.65f);
      path.lineTo(0.41f, 0.65f);
      path.lineTo(0.41f, 0.5f);
      path.lineTo(0.35f, 0.5f);
      path.lineTo(0.35f, 0.35f);
      path.closeSubPath();

      path.startNewSubPath(0.48f, 0.65f);
      path.lineTo(0.48f, 0.35f);
      path.lineTo(0.52f, 0.35f);
      path.lineTo(0.52f, 0.5f);
      path.lineTo(0.58f, 0.5f);
      path.lineTo(0.58f, 0.65f);
      path.lineTo(0.42f, 0.65f);
      path.lineTo(0.42f, 0.5f);
      path.lineTo(0.48f, 0.5f);
      path.closeSubPath();

      path.startNewSubPath(0.65f, 0.35f);
      path.lineTo(0.76f, 0.35f);
      path.lineTo(0.76f, 0.65f);
      path.lineTo(0.59f, 0.65f);
      path.lineTo(0.59f, 0.5f);
      path.lineTo(0.65f, 0.5f);
      path.lineTo(0.65f, 0.35f);
      path.closeSubPath();

      return path;
    }

    static juce::Path keyboardBordered() {
      juce::Path board = keyboard();

      board.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.01f);
      board.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.01f);
      return board;
    }

    static juce::Path fullKeyboard() {
      juce::Path path;
      path.startNewSubPath(1, 0);
      path.lineTo(1, 2);
      path.lineTo(23, 2);
      path.lineTo(23, 1);
      path.lineTo(15, 1);
      path.lineTo(15, 0);
      path.closeSubPath();

      path.startNewSubPath(29, 0);
      path.lineTo(29, 1);
      path.lineTo(25, 1);
      path.lineTo(25, 2);
      path.lineTo(47, 2);
      path.lineTo(47, 1);
      path.lineTo(43, 1);
      path.lineTo(43, 0);
      path.closeSubPath();

      path.startNewSubPath(57, 0);
      path.lineTo(57, 1);
      path.lineTo(49, 1);
      path.lineTo(49, 2);
      path.lineTo(71, 2);
      path.lineTo(71, 0);
      path.closeSubPath();

      path.startNewSubPath(73, 0);
      path.lineTo(73, 2);
      path.lineTo(95, 2);
      path.lineTo(95, 1);
      path.lineTo(83, 1);
      path.lineTo(83, 0);
      path.closeSubPath();

      path.startNewSubPath(99, 0);
      path.lineTo(99, 1);
      path.lineTo(97, 1);
      path.lineTo(97, 2);
      path.lineTo(119, 2);
      path.lineTo(119, 1);
      path.lineTo(112, 1);
      path.lineTo(112, 0);
      path.closeSubPath();

      path.startNewSubPath(128, 0);
      path.lineTo(128, 1);
      path.lineTo(121, 1);
      path.lineTo(121, 2);
      path.lineTo(143, 2);
      path.lineTo(143, 1);
      path.lineTo(141, 1);
      path.lineTo(141, 0);
      path.closeSubPath();

      path.startNewSubPath(157, 0);
      path.lineTo(157, 1);
      path.lineTo(145, 1);
      path.lineTo(145, 2);
      path.lineTo(167, 2);
      path.lineTo(167, 0);
      path.closeSubPath();

      return path;
    }

    static juce::Path gear() {
      static constexpr float kRadius = 0.3f;
      static constexpr float kGearOuterRatio = 1.2f;
      static constexpr int kNumGearTeeth = 8;

      juce::Path path;
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      float offset = 0.5f - kRadius;
      float diameter = 2.0f * kRadius;
      path.addPieSegment(offset, offset, diameter, diameter, 0.0f, 2.0f * bitklavier::kPi, 0.5f);
      for (int i = 0; i < kNumGearTeeth; ++i) {
        float phase = 2.0f * i * bitklavier::kPi / kNumGearTeeth;
        float x_offset = kRadius * cosf(phase);
        float y_offset = kRadius * sinf(phase);
        juce::Line<float> line(0.5f + x_offset, 0.5f + y_offset,
                         0.5f + kGearOuterRatio * x_offset, 0.5f + kGearOuterRatio * y_offset);
        path.addLineSegment(line, 0.13f);
      }

      return path;
    }

    static juce::Path magnifyingGlass() {
      static constexpr float kRadius = 0.22f;
      static constexpr float kWidthRatio = 0.3f;
      static constexpr float kGlassOffset = 0.2f;
      static constexpr float kSqrt2 = 1.41421356237f;

      juce::Path path;
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      float diameter = 2.0f * kRadius;
      path.addPieSegment(kGlassOffset, kGlassOffset, diameter, diameter, 0.0f, 2.0f * bitklavier::kPi, 1.0f - kWidthRatio);

      float line_width = kWidthRatio * kRadius;
      float line_start = kGlassOffset + kSqrt2 * kRadius + line_width / 2.0f;
      path.addLineSegment(juce::Line<float>(line_start, line_start, 1.0f - kGlassOffset, 1.0f - kGlassOffset), line_width);

      return path;
    }

    static juce::Path save() {
      juce::Path outline;
      outline.startNewSubPath(0.5f, 0.25f);
      outline.lineTo(0.5f, 0.6f);
      outline.startNewSubPath(0.35f, 0.45f);
      outline.lineTo(0.5f, 0.6f);
      outline.lineTo(0.65f, 0.45f);

      outline.startNewSubPath(0.25f, 0.5f);
      outline.lineTo(0.2f, 0.5f);
      outline.lineTo(0.2f, 0.75f);
      outline.lineTo(0.8f, 0.75f);
      outline.lineTo(0.8f, 0.5f);
      outline.lineTo(0.75f, 0.5f);

      juce::Path path;
      juce::PathStrokeType stroke(0.05f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
      stroke.createStrokedPath(path, outline);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path loop() {
      juce::Path outline;
      outline.startNewSubPath(0.68f, 0.3f);
      outline.lineTo(0.85f, 0.3f);
      outline.lineTo(0.85f, 0.7f);
      outline.lineTo(0.15f, 0.7f);
      outline.lineTo(0.15f, 0.3f);
      outline.lineTo(0.61f, 0.3f);
      juce::PathStrokeType outer_stroke(0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

      juce::Path path;
      outer_stroke.createStrokeWithArrowheads(path, outline, 0.0f, 0.0f, 0.4f, 0.26f);
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path bounce() {
      juce::Path left_outline, right_outline;
      left_outline.startNewSubPath(0.5f, 0.5f);
      left_outline.lineTo(0.0f, 0.5f);
      left_outline.startNewSubPath(0.5f, 0.5f);
      left_outline.lineTo(1.0f, 0.5f);

      juce::PathStrokeType stroke(0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
      juce::Path left_path, right_path;
      stroke.createStrokeWithArrowheads(left_path, left_outline, 0.0f, 0.0f, 0.4f, 0.26f);
      stroke.createStrokeWithArrowheads(right_path, right_outline, 0.0f, 0.0f, 0.4f, 0.26f);

      juce::Path path;
      path.addPath(left_path);
      path.addPath(right_path);

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path bipolar() {
      juce::Path left_outline, right_outline;
      left_outline.startNewSubPath(0.3f, 0.5f);
      left_outline.lineTo(0.0f, 0.5f);
      left_outline.startNewSubPath(0.7f, 0.5f);
      left_outline.lineTo(1.0f, 0.5f);

      juce::PathStrokeType stroke(0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
      juce::Path left_path, right_path;
      stroke.createStrokeWithArrowheads(left_path, left_outline, 0.0f, 0.0f, 0.4f, 0.26f);
      stroke.createStrokeWithArrowheads(right_path, right_outline, 0.0f, 0.0f, 0.4f, 0.26f);

      juce::Path path;
      path.addEllipse(0.4f, 0.4f, 0.2f, 0.2f);
      path.addPath(left_path);
      path.addPath(right_path);

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path shuffle() {
      juce::Path over;
      over.startNewSubPath(0.1f, 0.7f);
      over.lineTo(0.25f, 0.7f);
      over.lineTo(0.55f, 0.3f);
      over.lineTo(0.95f, 0.3f);

      juce::Path under_first;
      under_first.startNewSubPath(0.1f, 0.3f);
      under_first.lineTo(0.25f, 0.3f);
      under_first.lineTo(0.325f, 0.4f);

      juce::Path under_second;
      under_second.startNewSubPath(0.475f, 0.6f);
      under_second.lineTo(0.55f, 0.7f);
      under_second.lineTo(0.95f, 0.7f);

      juce::PathStrokeType stroke(0.12f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
      juce::Path over_path;
      stroke.createStrokeWithArrowheads(over_path, over, 0.0f, 0.0f, 0.35f, 0.26f);
      juce::Path under_path_first;
      stroke.createStrokedPath(under_path_first, under_first);
      juce::Path under_path_second;
      stroke.createStrokeWithArrowheads(under_path_second, under_second, 0.0f, 0.0f, 0.35f, 0.26f);

      juce::Path path;
      path.addPath(over_path);
      path.addPath(under_path_first);
      path.addPath(under_path_second);

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path pencil() {
      static constexpr float kPencilDimension = 0.14f;
      static constexpr float kEraserLength = 0.6f * kPencilDimension;
      static constexpr float kSeparatorWidth = 0.15f * kPencilDimension;
      static constexpr float kPencilRemoval = kEraserLength + kSeparatorWidth;
      static constexpr float kBorder = 0.22f;

      juce::Path path;
      path.startNewSubPath(kBorder, 1.0f - kBorder);
      path.lineTo(kBorder + kPencilDimension, 1.0f - kBorder);
      path.lineTo(1.0f - kBorder - kPencilRemoval, kBorder + kPencilDimension + kPencilRemoval);
      path.lineTo(1.0f - kBorder - kPencilRemoval - kPencilDimension, kBorder + kPencilRemoval);
      path.lineTo(kBorder, 1.0f - kBorder - kPencilDimension);
      path.closeSubPath();

      path.startNewSubPath(1.0f - kBorder - kPencilDimension, kBorder);
      path.lineTo(1.0f - kBorder, kBorder + kPencilDimension);
      path.lineTo(1.0f - kBorder - kEraserLength, kBorder + kPencilDimension + kEraserLength);
      path.lineTo(1.0f - kBorder - kEraserLength - kPencilDimension, kBorder + kEraserLength);
      path.closeSubPath();
      
      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path halfSinCurve() {
      static constexpr float kBorder = 0.15f;
      static constexpr float kLineWidth = 0.1f;
      static constexpr float kEndpointStrokeWidth = 0.08f;
      static constexpr float kEndpointRadius = 0.09f;
      static constexpr int kNumCurvePoints = 16;
      static constexpr float kFullRadians = bitklavier::kPi * 2.0f;
      static constexpr float kBumpIn = kEndpointRadius;
      static constexpr float kAdjustXIn = kBumpIn + kEndpointRadius / 2.0f;

      juce::Path curve;
      float start_x = kBorder + kAdjustXIn;
      float start_y = 1.0f - kBorder - kBumpIn;
      float end_x = 1.0f - kBorder - kAdjustXIn;
      float end_y = kBorder + kBumpIn;

      curve.startNewSubPath(start_x, end_x);
      for (int i = 0; i < kNumCurvePoints; ++i) {
        float t = (1.0f + i) / kNumCurvePoints;
        float x = t * end_x + (1.0f - t) * start_x;
        float y_t = sinf((t - 0.5f) * bitklavier::kPi) * 0.5f + 0.5f;
        float y = y_t * end_y + (1.0f - y_t) * start_y;
        curve.lineTo(x, y);
      }

      juce::Path path;
      juce::PathStrokeType line_stroke(kLineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
      juce::PathStrokeType endpoint_stroke(kEndpointStrokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
      line_stroke.createStrokedPath(path, curve);

      juce::Path arc;
      arc.addCentredArc(end_x + kBumpIn, end_y, kEndpointRadius, kEndpointRadius, 0.0f, 0.0f, kFullRadians, true);
      arc.addCentredArc(start_x - kBumpIn, start_y, kEndpointRadius, kEndpointRadius, 0.0f, 0.0f, kFullRadians, true);
      juce::Path end_points;
      endpoint_stroke.createStrokedPath(end_points, arc);
      path.addPath(end_points);

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path ellipses() {
      juce::Path path;
      path.addEllipse(0.1f, 0.4f, 0.2f, 0.2f);
      path.addEllipse(0.4f, 0.4f, 0.2f, 0.2f);
      path.addEllipse(0.7f, 0.4f, 0.2f, 0.2f);

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path paintBrush() {
      static constexpr float kBrushWideDimension = 0.3f;
      static constexpr float kBrushHandleDimension = 0.08f;
      static constexpr float kBrushLength = 0.6f * kBrushWideDimension;
      static constexpr float kSeparatorWidth = 0.15f * kBrushWideDimension;
      static constexpr float kConnectionDistance = kBrushLength + kSeparatorWidth;
      static constexpr float kHandleDistance = kConnectionDistance + kBrushWideDimension * 0.8f;
      static constexpr float kBorder = 0.15f;

      juce::Path path;
      path.startNewSubPath(kBorder, 1.0f - kBorder);
      path.lineTo(kBorder + kBrushHandleDimension, 1.0f - kBorder);
      path.lineTo(1.0f - kBorder - kHandleDistance, kBorder + kBrushHandleDimension + kHandleDistance);
      path.lineTo(1.0f - kBorder - kConnectionDistance, kBorder + kBrushWideDimension + kConnectionDistance);
      path.lineTo(1.0f - kBorder - kConnectionDistance - kBrushWideDimension, kBorder + kConnectionDistance);
      path.lineTo(1.0f - kBorder - kHandleDistance - kBrushHandleDimension, kBorder + kHandleDistance);
      path.lineTo(kBorder, 1.0f - kBorder - kBrushHandleDimension);
      path.closeSubPath();

      path.startNewSubPath(1.0f - kBorder - kBrushWideDimension, kBorder);
      path.lineTo(1.0f - kBorder, kBorder + kBrushWideDimension);
      path.lineTo(1.0f - kBorder - kBrushLength, kBorder + kBrushWideDimension + kBrushLength);
      path.lineTo(1.0f - kBorder - kBrushLength - kBrushWideDimension, kBorder + kBrushLength);
      path.closeSubPath();

      path.addLineSegment(juce::Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
      path.addLineSegment(juce::Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
      return path;
    }

    static juce::Path createFilterStroke(const juce::Path& outline, float line_thickness = 0.1f) {
      juce::PathStrokeType stroke(line_thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

      juce::Path path;
      stroke.createStrokedPath(path, outline);
      return path;
    }

    static juce::Path lowPass(float line_thickness = 0.1f) {
      juce::Path outline;
      outline.startNewSubPath(1.0f, 0.8f);
      outline.lineTo(0.7f, 0.3f);
      outline.lineTo(0.5f, 0.5f);
      outline.lineTo(0.0f, 0.5f);
      juce::Path path = createFilterStroke(outline, line_thickness);
      path.addLineSegment(juce::Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static juce::Path highPass() {
      juce::Path outline;
      outline.startNewSubPath(0.0f, 0.8f);
      outline.lineTo(0.3f, 0.3f);
      outline.lineTo(0.5f, 0.5f);
      outline.lineTo(1.0f, 0.5f);
      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static juce::Path bandPass() {
      static constexpr float kMiddle = 3.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      juce::Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kMiddle);
      outline.lineTo(0.5f, 0.25f);
      outline.lineTo(2.0f / 3.0f, kMiddle);
      outline.lineTo(1.0f, kBottom);

      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.2f);
      return path;
    }

    static juce::Path leftArrow() {
      static constexpr float kArrowAmount = 1.0f / 3.0f;
      static constexpr float kBuffer = 0.0f;

      juce::Path outline;
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(kBuffer, 0.5f);
      outline.startNewSubPath(kBuffer, 0.5f);
      outline.lineTo(kBuffer + kArrowAmount, 0.5f - kArrowAmount);
      outline.startNewSubPath(kBuffer, 0.5f);
      outline.lineTo(kBuffer + kArrowAmount, 0.5f + kArrowAmount);
      return createFilterStroke(outline);
    }

    static juce::Path rightArrow() {
      static constexpr float kArrowAmount = 1.0f / 3.0f;
      static constexpr float kBuffer = 0.0f;

      juce::Path outline;
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(kBuffer, 0.5f);
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(1.0f - kArrowAmount - kBuffer, 0.5f - kArrowAmount);
      outline.startNewSubPath(1.0f - kBuffer, 0.5f);
      outline.lineTo(1.0f - kArrowAmount - kBuffer, 0.5f + kArrowAmount);
      return createFilterStroke(outline);
    }

    static juce::Path phaser1() {
      juce::Path outline;
      outline.startNewSubPath(0.0f, 0.5f);
      outline.lineTo(1.0f / 3.0f, 3.0f / 4.0f);
      outline.lineTo(1.0f / 2.0f, 1.0f / 4.0f);
      outline.lineTo(2.0f / 3.0f, 3.0f / 4.0f);
      outline.lineTo(1.0f, 0.5f);

      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.15f, 1.0f, 0.15f), 0.1f);
      path.addLineSegment(juce::Line<float>(1.0f, 0.85f, 1.0f, 0.85f), 0.1f);
      return path;
    }

    static juce::Path phaser3() {
      static constexpr int kNumHumps = 5;

      static const float kUp = 1.0f / 4.0f;
      static const float kDown = 3.0f / 4.0f;

      float delta = 1.0f / (2 * kNumHumps + 2);
      juce::Path outline;
      outline.startNewSubPath(0.0f, 0.5f);

      float position = 0.0f;
      for (int i = 0; i < kNumHumps; ++i) {
        position += delta;
        outline.lineTo(position, kDown);
        position += delta;
        outline.lineTo(position, kUp);
      }
      position += delta;
      outline.lineTo(position, kDown);
      outline.lineTo(1.0f, 0.5f);

      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.15f, 1.0f, 0.15f), 0.1f);
      path.addLineSegment(juce::Line<float>(1.0f, 0.85f, 1.0f, 0.85f), 0.1f);
      return path;
    }

    static juce::Path notch() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      juce::Path outline;
      outline.startNewSubPath(0.0f, kTop);
      outline.lineTo(1.0f / 3.0f, kTop);
      outline.lineTo(1.0f / 2.0f, kBottom);
      outline.lineTo(2.0f / 3.0f, kTop);
      outline.lineTo(1.0f, kTop);

      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static juce::Path narrowBand() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      juce::Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kBottom);
      outline.lineTo(0.5f, kTop);
      outline.lineTo(2.0f / 3.0f, kBottom);
      outline.lineTo(1.0f, kBottom);

      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static juce::Path wideBand() {
      static constexpr float kTop = 2.0f / 5.0f;
      static constexpr float kBottom = 4.0f / 5.0f;

      juce::Path outline;
      outline.startNewSubPath(0.0f, kBottom);
      outline.lineTo(1.0f / 3.0f, kTop);
      outline.lineTo(2.0f / 3.0f, kTop);
      outline.lineTo(1.0f, kBottom);

      juce::Path path = createFilterStroke(outline);
      path.addLineSegment(juce::Line<float>(1.0f, 0.2f, 1.0f, 0.2f), 0.1f);
      return path;
    }

    static juce::Path createCustomQuadrilateral(float x, float y, float bottomWidth, float topWidth, float height)
    {
        juce::Path p;

        // Point 1: Bottom-left corner
        juce::Point<float> p1(x, y + height);

        // Point 2: Bottom-right corner
        juce::Point<float> p2(x + bottomWidth, y + height);

        // Point 3: Top-right corner (angled)
        // The top is horizontal, so its y-coordinate is 'y'.
        juce::Point<float> p3(x + topWidth, y);

        // Point 4: Top-left corner
        juce::Point<float> p4(x, y);

        // Start a new subpath at the bottom-left corner
        p.startNewSubPath(p1);

        // Draw to the bottom-right corner (horizontal bottom)
        p.lineTo(p2);

        // Draw to the top-right corner (angled right side)
        p.lineTo(p3);

        // Draw to the top-left corner (horizontal top)
        p.lineTo(p4);

        // Close the subpath, which will draw the vertical left side
        p.closeSubPath();

        return p;
    }

};

