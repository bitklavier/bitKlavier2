// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Davis Polito on 9/24/24.
//

#include "library_section.h"
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

#include "header_section.h"

#include "fonts.h"
#include <memory>
#include "text_look_and_feel.h"



LibrarySection::LibrarySection() : SynthSection("header_section"), body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)) {
    //logo_section_ = std::make_unique<LogoSection>();
    //addSubSection(logo_section_.get());
    //logo_section_->addListener(this);

    addOpenGlComponent(body_);


    //inspectButton = std::make_unique<OpenGlToggleButton>("Inspect the UI");
    //inspectButton->setUiButton(false);
    //inspectButton->
    //addAndMakeVisible(inspectButton.get());
    //addChildComponent(inspectButton.get());

//  inspectButton->setText("Inspect");
//  // this chunk of code instantiates and opens the melatonin inspector
//  inspectButton->onClick = [&] {
//      if (!inspector)
//      {
//          inspector = std::make_unique<melatonin::Inspector> (*this);
//          inspector->onClose = [this]() { inspector.reset(); };
//      }
//
//      inspector->setVisible (true);
//  };
    setAlwaysOnTop(true);
    //setSkinOverride(Skin::kLibrary);
}

void LibrarySection::paintBackground(juce::Graphics& g) {
    paintContainer(g);
    paintChildrenBackgrounds(g);
    g.setColour(findColour(Skin::kBody, true));
    int logo_section_width = 32.0 + getPadding();
    g.fillRect(0, 0, logo_section_width, getHeight());

    int label_rounding = findValue(Skin::kLabelBackgroundRounding);
    //g.setColour(findColour(Skin::kTextComponentBackground, true));
    g.setColour(juce::Colours::white);
    //g.fillRoundedRectangle(sampleSelector->getBounds().toFloat(), label_rounding);
    g.fillRect(100, 50, 100, 100);
    paintKnobShadows(g);



}

void LibrarySection::resized() {
    static constexpr float kTextHeightRatio = 0.3f;
    static constexpr float kPaddingLeft = 0.25f;
    juce::Colour body_text = findColour(Skin::kBodyText, true);
    //sampleSelectText->setColor(body_text);
//  oscilloscope_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
//  spectrogram_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
    int height = getHeight();
    int width = getWidth();

    body_->setBounds(getLocalBounds());
    body_->setRounding(findValue(Skin::kBodyRounding));
    body_->setColor(findColour(Skin::kBody, true));

    SynthSection::resized();
}

void LibrarySection::reset() {
//  if (preset_selector_)
//    //synth_preset_selector_->resetText();
}

void LibrarySection::buttonClicked(juce::Button* clicked_button) {

        SynthSection::buttonClicked(clicked_button);
}

void LibrarySection::sliderValueChanged(juce::Slider* slider) {

    SynthSection::sliderValueChanged(slider);
}

