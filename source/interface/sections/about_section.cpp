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

#include "about_section.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "synth_section.h"
#include "text_look_and_feel.h"

namespace {
  void setColorRecursively(juce::Component *component, int color_id, const juce::Colour& color) {
    component->setColour(color_id, color);
    for (juce::Component *child : component->getChildren())
      setColorRecursively(child, color_id, color);
  }
}

AboutSection::AboutSection(const juce::String& name) : Overlay(name), body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)) {
    addOpenGlComponent (body_);
    name_text_ = std::make_shared<PlainTextComponent> ("plugin name", "bitKlavier");
    addOpenGlComponent (name_text_);
    name_text_->setFontType (PlainTextComponent::kTitle);
    name_text_->setTextSize (40.0f);
    version_text_ = std::make_shared<PlainTextComponent> ("version", juce::String ("version  ") + ProjectInfo::versionString);//ProjectInfo::versionString);
    addOpenGlComponent (version_text_);
    version_text_->setFontType (PlainTextComponent::kLight);
    version_text_->setTextSize (12.0f);
}

AboutSection::~AboutSection() = default;

void AboutSection::setLogoBounds() {
  juce::Rectangle<int> info_rect = getInfoRect();
  int left_buffer = kLeftLogoBuffer * size_ratio_;
  //logo_->setBounds(info_rect.getX() + left_buffer, info_rect.getY() + (kPaddingY + 12) * size_ratio_,
  //                 kLogoWidth * size_ratio_, kLogoWidth * size_ratio_);
}

void AboutSection::resized() {

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();

    if (parent && device_selector_ == nullptr) {
        juce::AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
        if (device_manager) {
          device_selector_ = std::make_unique<OpenGlDeviceSelector>(
              *device_manager, 0, 0, bitklavier::kNumChannels, bitklavier::kNumChannels, true, false, false, false, parent->getSynth()->user_prefs->tree);
          addAndMakeVisible(device_selector_.get());
          addOpenGlComponent(device_selector_->getImageComponent());
        }
    }

    juce::Rectangle<int> info_rect = getInfoRect();
    body_->setBounds(info_rect);
    body_->setRounding(findValue(Skin::kBodyRounding));
    body_->setColor(findColour(Skin::kBackground, true));

    int large_padding = findValue(Skin::kLargePadding);
    int small_padding = findValue(Skin::kPadding);

    info_rect.reduce(large_padding, small_padding);
    info_rect.removeFromTop (large_padding);
    name_text_->setBounds(info_rect.removeFromTop(50));
    version_text_->setBounds(info_rect.removeFromTop(20));

    if (device_selector_) {
    // device_selector_->setBounds(info_rect.getX(),info_rect.getY(),
    //                             info_rect.getWidth(), info_rect.getHeight() - padding_y);
        device_selector_->setBounds(info_rect);

        juce::Colour background = findColour(Skin::kPopupBackground, true);
        setColorRecursively(device_selector_.get(), juce::ListBox::backgroundColourId, background);
        setColorRecursively(device_selector_.get(), juce::ComboBox::backgroundColourId, background);
        setColorRecursively(device_selector_.get(), juce::PopupMenu::backgroundColourId, background);
        setColorRecursively(device_selector_.get(), juce::BubbleComponent::backgroundColourId, background);

        juce::Colour text = findColour(Skin::kBodyText, true);
        setColorRecursively(device_selector_.get(), juce::ListBox::textColourId, text);
        setColorRecursively(device_selector_.get(), juce::ComboBox::textColourId, text);

        setColorRecursively(device_selector_.get(), juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
        setColorRecursively(device_selector_.get(), juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        setColorRecursively(device_selector_.get(), juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
  }

//  name_text_->setTextSize(40.0f * size_ratio_);
//  version_text_->setTextSize(12.0f * size_ratio_);

  Overlay::resized();
}

void AboutSection::mouseUp(const juce::MouseEvent &e) {
  if (!getInfoRect().contains(e.getPosition()))
    setVisible(false);
}

void AboutSection::setVisible(bool should_be_visible) {
  if (should_be_visible) {
    //setLogoBounds();
    juce::Image image(juce::Image::ARGB, 1, 1, false);
    juce::Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }

  Overlay::setVisible(should_be_visible);
}

void AboutSection::buttonClicked(juce::Button* clicked_button) {

}

juce::Rectangle<int> AboutSection::getInfoRect() {
    int info_height = kBasicInfoHeight * size_ratio_;
    int info_width = kInfoWidth * size_ratio_;
    // if (device_selector_)
    //     info_height += device_selector_->getBounds().getHeight();

    int x = 20 * size_ratio_;
    int y = 75 * size_ratio_;
    info_height = getHeight() - 200;

    DBG("info_width = " << info_width << ", info_height = " << info_height << ", size_ratio_ = " << size_ratio_);
    return juce::Rectangle<int>(x, y, info_width, info_height);
}

void AboutSection::setGuiSize(float multiplier) {
  if (juce::Desktop::getInstance().getKioskModeComponent()) {
    juce::Desktop::getInstance().setKioskModeComponent(nullptr);
    return;
  }

  float percent = sqrtf(multiplier);
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->setGuiSize(percent);
}

void AboutSection::fullScreen() {
  if (juce::Desktop::getInstance().getKioskModeComponent())
    juce::Desktop::getInstance().setKioskModeComponent(nullptr);
  else
    juce::Desktop::getInstance().setKioskModeComponent(getTopLevelComponent());
}
