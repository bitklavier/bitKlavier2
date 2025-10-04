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
#include "load_save.h"
#include "SampleLoadManager.h"

LogoSection::LogoSection() : SynthSection("logo_section") {
#if !defined(NO_TEXT_ENTRY)
    logo_button_ = std::make_unique<LogoButton>("logo");

    addAndMakeVisible(logo_button_.get());
    addOpenGlComponent(logo_button_->getImageComponent());
    logo_button_->addListener(this);
#endif


    setSkinOverride(Skin::kLogo);
}

void LogoSection::resized() {
    int logo_padding_y = kLogoPaddingY * size_ratio_;
    int logo_height = getHeight() - 2 * logo_padding_y;
    int logo_padding_x = (getWidth() - logo_height) / 2;
    if (logo_button_)
        logo_button_->setBounds(logo_padding_x, logo_padding_y + 4, logo_height, logo_height);
}

void LogoSection::paintBackground(juce::Graphics &g) {
    if (logo_button_) {
        logo_button_->setRingColors(findColour(Skin::kWidgetPrimary1, true), findColour(Skin::kWidgetPrimary2, true));
        logo_button_->setLetterColors(findColour(Skin::kWidgetSecondary1, true),
                                      findColour(Skin::kWidgetSecondary2, true));
    }
}

void LogoSection::buttonClicked(juce::Button *clicked_button) {
    for (Listener *listener: listeners_)
        listener->showAboutSection();
}

HeaderSection::HeaderSection(const juce::ValueTree &gal) : SynthSection("header_section"), tab_offset_(0),
                                                           body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)),
                                                           gallery(gal) {

    jassert(gal.hasType(IDs::GALLERY));
    addOpenGlComponent(body_);
    logo_section_ = std::make_unique<LogoSection>();
    logo_section_->setAlwaysOnTop(true);
    logo_section_->addListener(this);
    addSubSection(logo_section_.get());
    sampleSelector = std::make_unique<juce::ShapeButton>("Selector", juce::Colour(0xff666666),
                                                         juce::Colour(0xffaaaaaa), juce::Colour(0xff888888));

    addAndMakeVisible(sampleSelector.get());
    sampleSelector->addListener(this);
    sampleSelector->setTriggeredOnMouseDown(true);
    sampleSelector->setShape(juce::Path(), true, true, true);
    currentSampleType = 0;
    sampleSelectText = std::make_shared<PlainTextComponent>("Sample Select Text", "---");
    addOpenGlComponent(sampleSelectText);

    pianoSelectText = std::make_shared<PlainTextComponent>("Piano", "---");
    addOpenGlComponent(pianoSelectText);
    pianoSelector = std::make_unique<juce::ShapeButton>("Selector", juce::Colour(0xff666666),
                                                        juce::Colour(0xffaaaaaa), juce::Colour(0xff888888));
    addAndMakeVisible(pianoSelector.get());
    pianoSelector->addListener(this);
    pianoSelector->setTriggeredOnMouseDown(true);
    pianoSelector->setShape(juce::Path(), true, true, true);
    currentPianoIndex = 0;

    addPianoButton = std::make_unique<OpenGlTextButton>("header_add_piano");
    addOpenGlComponent(addPianoButton->getGlComponent());
    addAndMakeVisible(addPianoButton.get());
    addPianoButton->addListener(this);
    addPianoButton->setLookAndFeel(TextLookAndFeel::instance());
    addPianoButton->setButtonText("add piano");

    sampleSelectText->setText("---");
    pianoSelectText->setText(getAllPianoNames().at(0));
    setAlwaysOnTop(true);
    setSkinOverride(Skin::kHeader);
}

const juce::ValueTree &HeaderSection::getActivePiano() {
    for (const auto &vt: gallery) {
        if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::isActive)) {
            return vt;
        }
    }
}

std::vector<std::string> HeaderSection::getAllPianoNames() {
    std::vector<std::string> names;
    for (const auto &vt: gallery) {
        if (vt.hasType(IDs::PIANO)) {
            names.push_back(vt.getProperty(IDs::name).toString().toStdString());
        }
    }
    return names;
}

void HeaderSection::paintBackground(juce::Graphics &g) {
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

void HeaderSection::resized() {
    static constexpr float kTextHeightRatio = 0.3f;
    static constexpr float kPaddingLeft = 0.25f;
    juce::Colour body_text = findColour(Skin::kBodyText, true);
    sampleSelectText->setColor(body_text);
    pianoSelectText->setColor(body_text);
    //  oscilloscope_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
    //  spectrogram_->setColour(Skin::kBody, findColour(Skin::kBackground, true));
    int height = getHeight();
    int width = getWidth();

    body_->setBounds(getLocalBounds());
    body_->setRounding(findValue(Skin::kBodyRounding));
    body_->setColor(findColour(Skin::kBody, true));
    int widget_margin = findValue(Skin::kWidgetMargin);
    int large_padding = findValue(Skin::kLargePadding);

    int logo_width = findValue(Skin::kModulationButtonWidth);
    int label_height = findValue(Skin::kLabelBackgroundHeight);
    logo_section_->setBounds(0, -10, logo_width, height);
    sampleSelector->setBounds(logo_width + widget_margin, logo_section_->getBottom() / 2, 100, label_height);
    sampleSelectText->setBounds(sampleSelector->getBounds());

    pianoSelector->setBounds(sampleSelector->getRight() + 10, sampleSelector->getY(), sampleSelector->getWidth(),
                             label_height);
    pianoSelectText->setBounds(pianoSelector->getBounds());
    float label_text_height = findValue(Skin::kLabelHeight);
    sampleSelectText->setTextSize(label_text_height);
    pianoSelectText->setTextSize(label_text_height);

    addPianoButton->setBounds(sampleSelector->getRight() + 10, sampleSelector->getY() -label_height,
                              sampleSelector->getWidth(), label_height);

    SynthSection::resized();
}

void HeaderSection::reset() {
    //  if (preset_selector_)
    //    //synth_preset_selector_->resetText();
}


void HeaderSection::buttonClicked(juce::Button *clicked_button) {
    if (clicked_button == exit_temporary_button_.get()) {
    } else if (clicked_button == addPianoButton.get()) {
        auto interface = findParentComponentOfClass<SynthGuiInterface>();
        // interface->addPiano(
        // create new piano with active parameter true or 1
        juce::ValueTree piano{IDs::PIANO};
        piano.setProperty(IDs::isActive, true, nullptr);
        piano.setProperty(IDs::name, "new piano", nullptr);
        juce::ValueTree preparations(IDs::PREPARATIONS);
        juce::ValueTree connections(IDs::CONNECTIONS);
        juce::ValueTree modconnections(IDs::MODCONNECTIONS);

        piano.appendChild(preparations, nullptr);
        piano.appendChild(connections, nullptr);
        piano.appendChild(modconnections, nullptr);
        interface->setPianoSwitchTriggerThreadMessage();
        //set all current pianos to be inactive or isActive parameter 0
        for (auto vt: gallery) {
            if (vt.hasType(IDs::PIANO)) {
                vt.setProperty(IDs::isActive, 0, nullptr);
            }
        }

        gallery.appendChild(piano, nullptr);
        pianoSelectText->setText(getActivePiano().getProperty(IDs::name));
        interface->allNotesOff();
        resized();
    } else if (clicked_button == sampleSelector.get()) {
        PopupItems options;
        SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
        auto string_names = parent->getSampleLoadManager()->getAllSampleSets();
        for (int i = 0; i < string_names.size(); i++) {
            options.addItem(i, string_names[i]);

        }

        juce::Point<int> position(sampleSelector->getX(), sampleSelector->getBottom());
        showPopupSelector(this, position, options, [=](int selection, int) {
            SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
            _parent->getSampleLoadManager()->loadSamples(selection, true);
            sampleSelectText->setText(_parent->getSampleLoadManager()->getAllSampleSets()[selection]);
            resized();
        });
    } else if (clicked_button == pianoSelector.get()) {
        PopupItems options;
        auto names = getAllPianoNames();
        for (int i = 0; i < names.size(); i++) {
            options.addItem(i, names[i]);
        }

        juce::Point<int> position(pianoSelector->getX(), pianoSelector->getBottom());
        showPopupSelector(this, position, options, [=](int selection, int) {
            pianoSelectText->setText(names[selection]);

            auto interface = findParentComponentOfClass<SynthGuiInterface>();
            interface->setPianoSwitchTriggerThreadMessage();
            interface->allNotesOff();
            for (auto vt: gallery) {
                if (vt.hasType(IDs::PIANO)) {
                    vt.setProperty(IDs::isActive, 0, nullptr);
                }
            }
            for (auto vt: gallery) {
                if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::name) == pianoSelectText->getText()) {
                    vt.setProperty(IDs::isActive, 1, nullptr);
                    break;
                }
            }


            resized();
        });
    } else if (clicked_button == loadButton.get()) {
        SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
        parent->openLoadDialog();
    } else if (clicked_button == saveButton.get()) {
        SynthGuiInterface *interface = findParentComponentOfClass<SynthGuiInterface>();
        interface->openSaveDialog();
    } else
        SynthSection::buttonClicked(clicked_button);
}

void HeaderSection::sliderValueChanged(juce::Slider *slider) {
    //  if (slider == tab_selector_.get()) {
    //    int index = tab_selector_->getValue();
    //    for (Listener* listener : listeners_)
    //      listener->tabSelected(index);
    //  }
    //  else
    SynthSection::sliderValueChanged(slider);
}

//void HeaderSection::setPresetBrowserVisibility(bool visible) {
//  for (Listener* listener : listeners_)
//    listener->setPresetBrowserVisibility(visible, tab_selector_->getValue());
//}
//
//void HeaderSection::setBankExporterVisibility(bool visible) {
//  for (Listener* listener : listeners_)
//    listener->setBankExporterVisibility(visible, tab_selector_->getValue());
//}
//
//void HeaderSection::deleteRequested(juce::File preset) {
//  for (Listener* listener : listeners_)
//    listener->deleteRequested(preset);
//}
//
//void HeaderSection::bankImported() {
//  for (Listener* listener : listeners_)
//    listener->bankImported();
//}
//
//void HeaderSection::save(juce::File preset) {
//  synth_preset_selector_->resetText();
//  synth_preset_selector_->setModified(false);
//}
//
//void HeaderSection::setTemporaryTab(juce::String name) {
//  temporary_tab_->setText(name);
//  tab_selector_->setVisible(name.isEmpty());
//  exit_temporary_button_->setVisible(!name.isEmpty());
//  repaint();
//  repaintBackground();
//}
//
//void HeaderSection::setOscilloscopeMemory(const vital::poly_float* memory) {
//  oscilloscope_->setOscilloscopeMemory(memory);
//}
//
//void HeaderSection::setAudioMemory(const vital::StereoMemory* memory) {
//  spectrogram_->setAudioMemory(memory);
//}

//void HeaderSection::notifyChange() {
//  synth_preset_selector_->setModified(true);
//}
//
//void HeaderSection::notifyFresh() {
//  synth_preset_selector_->resetText();
//  synth_preset_selector_->setModified(false);
//}
