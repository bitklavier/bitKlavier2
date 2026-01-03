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
#include "synth_base.h"

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

    soundfontPresetSelector = std::make_unique<juce::ShapeButton>("Selector", juce::Colour(0xff666666),
                                                                  juce::Colour(0xffaaaaaa), juce::Colour(0xff888888));
    addChildComponent(soundfontPresetSelector.get());
    soundfontPresetSelector->addListener(this);
    soundfontPresetSelector->setTriggeredOnMouseDown(true);
    soundfontPresetSelector->setShape(juce::Path(), true, true, true);
    currentSoundfontPreset = 0;
    soundfontPresetSelectText = std::make_shared<PlainTextComponent>("Preset Select Text", "---");
    addOpenGlComponent(soundfontPresetSelectText);
    soundfontPresetSelectText->setVisible(false);

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

    globalSoundset_label = std::make_shared<PlainTextComponent>("globalsamples", "Global Soundset");
    addOpenGlComponent(globalSoundset_label);
    globalSoundset_label->setTextSize (12.0f);
    globalSoundset_label->setFontType (PlainTextComponent::kTitle);
    globalSoundset_label->setJustification(juce::Justification::centred);

    soundfontPreset_label = std::make_shared<PlainTextComponent>("soundfontpreset", "Soundfont Preset");
    addOpenGlComponent(soundfontPreset_label);
    soundfontPreset_label->setTextSize (12.0f);
    soundfontPreset_label->setFontType (PlainTextComponent::kTitle);
    soundfontPreset_label->setJustification(juce::Justification::centred);
    soundfontPreset_label->setVisible(false);

    currentPiano_label = std::make_shared<PlainTextComponent>("currentpiano", "Current Piano");
    addOpenGlComponent(currentPiano_label);
    currentPiano_label->setTextSize (12.0f);
    currentPiano_label->setFontType (PlainTextComponent::kTitle);
    currentPiano_label->setJustification(juce::Justification::centred);

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

void HeaderSection::renamePiano(juce::String newname)
{
    for (auto vt: gallery) {
        if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::isActive)) {
            vt.setProperty(IDs::name, newname, nullptr);
        }
    }
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
    int widget_margin = findValue(Skin::kWidgetMargin);
    int large_padding = findValue(Skin::kLargePadding);
    int small_padding = findValue(Skin::kPadding);
    float label_text_height = findValue(Skin::kLabelHeight);
    int logo_width = findValue(Skin::kModulationButtonWidth);
    int label_height = findValue(Skin::kLabelBackgroundHeight);
    juce::Colour body_text = findColour(Skin::kBodyText, true);
    int height = getHeight();
    int width = getWidth();

    body_->setBounds(getLocalBounds());
    body_->setRounding(findValue(Skin::kBodyRounding));
    body_->setColor(juce::Colours::black);
    //logo_section_->setBounds(0, -10, logo_width, height);
    logo_section_->setBounds(0, -10, logo_width, height + 10);

    juce::Rectangle<int> headerArea = getLocalBounds();
    headerArea.removeFromLeft(logo_width + large_padding);
    juce::Rectangle<int> headerLabelArea = headerArea.removeFromTop(label_height + small_padding);
    headerLabelArea.removeFromTop(small_padding * 2);

    sampleSelectText->setColor(body_text);
    pianoSelectText->setColor(body_text);
    soundfontPresetSelectText->setColor(body_text);

    //sampleSelector->setBounds(logo_width + widget_margin, logo_section_->getBottom() / 2, 100, label_height);
    sampleSelector->setBounds(headerArea.removeFromLeft(100));
    sampleSelectText->setTextSize(label_text_height);
    sampleSelectText->setBounds(sampleSelector->getBounds());
    globalSoundset_label->setBounds(headerLabelArea.removeFromLeft(100));

    headerArea.removeFromLeft(large_padding * 2);
    headerLabelArea.removeFromLeft(large_padding * 2);

    //soundfontPresetSelector->setBounds(pianoSelector->getRight() + 10, pianoSelector->getY(), pianoSelector->getWidth(), label_height);
    soundfontPresetSelector->setBounds(headerArea.removeFromLeft(100));
    soundfontPresetSelectText->setTextSize(label_text_height);
    soundfontPresetSelectText->setBounds(soundfontPresetSelector->getBounds());
    soundfontPreset_label->setBounds(headerLabelArea.removeFromLeft(100));

    //pianoSelector->setBounds(sampleSelector->getRight() + 10, sampleSelector->getY(), sampleSelector->getWidth(), label_height);
    headerArea.removeFromLeft(large_padding * 2);
    headerLabelArea.removeFromLeft(large_padding * 2);

    pianoSelector->setBounds(headerArea.removeFromLeft(100));
    pianoSelectText->setBounds(pianoSelector->getBounds());
    pianoSelectText->setTextSize(label_text_height);
    currentPiano_label->setBounds(headerLabelArea.removeFromLeft(100));

    //addPianoButton->setBounds(sampleSelector->getRight() + 10, sampleSelector->getY() - label_height, sampleSelector->getWidth(), label_height);
    addPianoButton->setBounds(headerArea.removeFromRight(100));

    SynthSection::resized();
}

void HeaderSection::reset() {
    //  if (preset_selector_)
    //    //synth_preset_selector_->resetText();
}

void HeaderSection::addPiano()
{
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    // interface->addPiano(
    // create new piano with active parameter true or 1
    juce::ValueTree piano{IDs::PIANO};
    piano.setProperty(IDs::isActive, true, nullptr);
    juce::String pianoName = "piano " + juce::String(howManyOfThisPrepTypeInVT(gallery, IDs::PIANO) + 1);
    piano.setProperty(IDs::name, pianoName, nullptr);
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
}

void HeaderSection::buttonClicked(juce::Button *clicked_button) {
    if (clicked_button == exit_temporary_button_.get()) {
    } else if (clicked_button == addPianoButton.get()) {
        addPiano();
    } else if (clicked_button == sampleSelector.get()) {
        PopupItems options;
        SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
        auto string_names = parent->getSampleLoadManager()->getAllSampleSets();
        for (int i = 0; i < string_names.size(); i++) {
            options.addItem(i, string_names[i]);
        }
        juce::Point<int> position(sampleSelector->getX(), sampleSelector->getBottom());

        /*
         * todo: it looks like the global sample set if being set with the gallery, but it should
         *          be set (saved and restored) with each piano
         */
        showPopupSelector(this, position, options, [=](int selection, int) {
            SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
            _parent->getSampleLoadManager()->loadSamples(_parent->getSampleLoadManager()->getAllSampleSets()[selection], _parent->getSynth()->getValueTree());
            sampleSelectText->setText(_parent->getSampleLoadManager()->getAllSampleSets()[selection]);
            resized();
            notifyFresh();
        });
    } else if (clicked_button == pianoSelector.get()) {
        PopupItems options;
        auto names = getAllPianoNames();
        int itemCounter = 0;
        options.addItem(itemCounter++, "Add");
        options.addItem(itemCounter++, "Rename");
        options.addItem(itemCounter++, "Duplicate");
        options.addItem(itemCounter++, "Delete");
        options.addItem(itemCounter++, "-----------");

        for (int i = 0; i < names.size(); i++) {
            options.addItem(itemCounter + i, names[i]);
        }

        juce::Point<int> position(pianoSelector->getX(), pianoSelector->getBottom());
        showPopupSelector(this, position, options, [=](int selection, int) {
            if (selection >= itemCounter)
            {
                pianoSelectText->setText(names[selection - itemCounter]);
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
            }
            else
            {
                // get name of current piano
                juce::String currentName;
                for (auto vt: gallery) {
                    if (vt.hasType(IDs::PIANO) && vt.getProperty (IDs::isActive).equals(1)) {
                        currentName = vt.getProperty(IDs::name).toString();
                    }
                }

                switch (selection)
                {
                    case 0:
                        DBG("add piano");
                        addPiano();
                        break;
                    case 1:
                        DBG("rename piano");
                        showTextInputBox("Piano Name", "", currentName, [this](juce::String userInput) {
                            if (userInput.isNotEmpty()) {
                                DBG("User entered: " + userInput);
                                this->renamePiano(userInput);
                                this->pianoSelectText->setText(userInput);
                            }
                        });
                        break;
                    case 2:
                        DBG("duplicate piano");
                        break;
                    case 3:
                        DBG("delete piano");
                        break;
                    default:
                        DBG("no action");
                }
            }


        });
    } else if (clicked_button == soundfontPresetSelector.get()) {
        const juce::String sfzName = gallery.getProperty("soundset").toString();
        auto* parent = findParentComponentOfClass<SynthGuiInterface>();
        auto* slm = parent->getSampleLoadManager();
        // No presets → nothing to show
        if (!slm->sfzHasPresets(sfzName))
            return;

        PopupItems options;

        auto presetNames = slm->getSFZPresetNames(sfzName);
        const auto selectedPreset = slm->getSelectedSFZPreset(sfzName);
        const juce::juce_wchar bellChar = 7; // '\a'

        for (auto& s : presetNames)
            s = s.removeCharacters (juce::String::charToString (bellChar));
        for (int i = 0; i < presetNames.size(); ++i)
            options.addItem(i, presetNames.getReference(i).toStdString());

        juce::Point<int> position(soundfontPresetSelector->getX(),
                                  soundfontPresetSelector->getBottom());

        showPopupSelector(this, position, options,
                          [this, presetNames, sfzName](int selection, int) {
                              if (selection < 0 || selection >= presetNames.size())
                                  return;
                              SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();

                              // Update UI
                              soundfontPresetSelectText->setText(presetNames[selection]);
                              resized();

                              // // Persist per-piano (ValueTree-driven)
                              // if (gallery.isValid())
                              //     gallery.setProperty(IDs::soundfont_preset,
                              //                         presetNames[selection],
                              //                         nullptr);

                              parent->getSampleLoadManager()->changeSFZPresetAndUpdateTree(sfzName, selection,gallery);
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

void HeaderSection::notifyChange() {
}

void HeaderSection::notifyFresh() {
    if (!gallery.isValid())
        return;
    // gallery.getProperty("soundset").upToFirstOccurrenceOf("||", false, false);
    sampleSelectText->setText(gallery.getProperty(IDs::soundset).toString().upToFirstOccurrenceOf("||", false, false));
    resized();
    auto sfzName = gallery.getProperty(IDs::soundset).toString();
    auto parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent->getSampleLoadManager()->sfzHasPresets(gallery.getProperty(IDs::soundset).toString())) {
        soundfontPresetSelector->setVisible(true);
        soundfontPresetSelectText->setVisible(true);
        soundfontPreset_label->setVisible (true);
    } else {
        soundfontPresetSelector->setVisible(false);
        soundfontPresetSelectText->setVisible(false);
        soundfontPreset_label->setVisible (false);
    }
    static juce::var nullVar;
    // auto val = gallery.getProperty(IDs::soundfont_preset);
    auto val = gallery.getProperty(IDs::soundset).toString().
    fromFirstOccurrenceOf("||", false, false);
    if (val.isNotEmpty()) {
        // const auto presetName = val.toString();
        soundfontPresetSelectText->setText(val);
    }
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
