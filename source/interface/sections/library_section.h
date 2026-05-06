// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Davis Polito on 9/24/24.
//

#ifndef BITKLAVIER2_LIBRARY_SECTION_H
#define BITKLAVIER2_LIBRARY_SECTION_H


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



#include "synth_section.h"
#include "open_gl_background.h"



class LibrarySection : public SynthSection {
public:
    class Listener {
    public:
        virtual ~Listener() { }

        virtual void showAboutSection() = 0;
    };

    LibrarySection();

    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void reset() override;

    void buttonClicked(juce::Button* clicked_button) override;
    void sliderValueChanged(juce::Slider* slider) override;



//    void showAboutSection() override {
//      for (Listener* listener : listeners_)
//        listener->showAboutSection();
//    }

    void notifyChange();
    void notifyFresh();

    void addListener(Listener* listener) { listeners_.push_back(listener); }

private:
    std::vector<Listener*> listeners_;
    std::shared_ptr<OpenGlQuad> body_;

    //juce::Image background_image_;
    //std::unique_ptr<SynthButton> click_me;
    // juce::TextButton inspectButton { "Inspect the UI" };
    //std::unique_ptr<melatonin::Inspector> inspector;
    //std::unique_ptr<OpenGlToggleButton> inspectButton;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibrarySection)
};




#endif //BITKLAVIER2_LIBRARY_SECTION_H
