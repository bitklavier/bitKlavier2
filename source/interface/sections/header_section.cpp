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

const juce::ValueTree &HeaderSection::getActivePiano()
{
    for (const auto &vt: gallery) {
        if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::isActive)) {
            return vt;
        }
    }
}

juce::ValueTree HeaderSection::getActivePianoCopy()
{
    for (auto vt : gallery) {
        if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::isActive))
            return vt; // returned by value (safe)
    }
    return {}; // invalid ValueTree if none active
}

std::vector<std::string> HeaderSection::getAllPianoNames()
{
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
    //addPianoButton->setBounds(headerArea.removeFromRight(100));

    SynthSection::resized();
}

void HeaderSection::reset() {
    //  if (preset_selector_)
    //    //synth_preset_selector_->resetText();
}

void HeaderSection::addPiano()
{
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

    auto interface = findParentComponentOfClass<SynthGuiInterface>();
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

void HeaderSection::duplicatePiano (const juce::ValueTree pianoToCopy)
{
    if (! pianoToCopy.isValid() || ! pianoToCopy.hasType(IDs::PIANO))
        return;

    // 0) Flush live processor state into the ValueTree so the copy keeps current parameter values
    if (auto preps = pianoToCopy.getChildWithName(IDs::PREPARATIONS); preps.isValid())
    {
        // Triggers PreparationList::valueTreePropertyChanged() to write back proc states
        preps.setProperty(IDs::sync, 1, nullptr);
    }

    // 1) Deep copy the whole subtree and remap IDs & connections for the duplicate
    juce::ValueTree newPiano = pianoToCopy.createCopy();
    remapPianoUUIDsAndConnections(newPiano);

    // 2) Make the copy active and give it a unique name
    newPiano.setProperty(IDs::isActive, true, nullptr);

    if (newPiano.hasProperty(IDs::name))
    {
        juce::String baseName = pianoToCopy.getProperty(IDs::name).toString();
        juce::String candidate = baseName;
        int suffix = 2;
        auto names = getAllPianoNames();
        while (std::find(names.begin(), names.end(), candidate.toStdString()) != names.end())
            candidate = baseName + " (" + juce::String(suffix++) + ")";
        newPiano.setProperty(IDs::name, candidate, nullptr);
    }

    // 3) Deactivate current pianos and append the duplicate
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    interface->setPianoSwitchTriggerThreadMessage();

    for (auto vt : gallery)
        if (vt.hasType(IDs::PIANO))
            vt.setProperty(IDs::isActive, 0, nullptr);

    gallery.appendChild(newPiano, nullptr);

    // 4) UI updates
    pianoSelectText->setText(newPiano.getProperty(IDs::name));
    interface->allNotesOff();
    resized();

    // Optional: ensure lines are built after components exist
    // juce::MessageManager::callAsync([safeInterface = juce::Component::SafePointer<SynthGuiInterface>(interface)]{
    //     if (safeInterface != nullptr)
    //     {
    //         // If you have explicit rebuild hooks, call them here
    //         // safeInterface->getGui()->connection_list->rebuildAllGui();
    //         // safeInterface->getGui()->modulation_manager->added();
    //     }
    // });
}

void HeaderSection::remapPianoUUIDsAndConnections (juce::ValueTree& piano)
{
    auto preps = piano.getChildWithName(IDs::PREPARATIONS);
    if (! preps.isValid()) return;

    std::map<juce::String, juce::String> uuidMap;
    std::map<juce::AudioProcessorGraph::NodeID, juce::AudioProcessorGraph::NodeID> nodeIdMap;

    for (int i = 0; i < preps.getNumChildren(); ++i)
    {
        auto prep = preps.getChild(i);
        if (! prep.isValid()) continue;

        auto oldUuid = prep.getProperty(IDs::uuid).toString();
        if (oldUuid.isEmpty()) continue;

        auto newUuid = juce::Uuid().toString();
        uuidMap[oldUuid] = newUuid;
        prep.setProperty(IDs::uuid, newUuid, nullptr);

        auto oldNodeId = juce::AudioProcessorGraph::NodeID(juce::Uuid(oldUuid).getTimeLow());
        auto newNodeId = juce::AudioProcessorGraph::NodeID(juce::Uuid(newUuid).getTimeLow());
        nodeIdMap[oldNodeId] = newNodeId;

        prep.setProperty(IDs::nodeID,
                         juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(newNodeId),
                         nullptr);
    }

    if (uuidMap.empty()) return;

    // CONNECTIONS remap (unchanged)
    if (auto conns = piano.getChildWithName(IDs::CONNECTIONS); conns.isValid())
    {
        for (int i = 0; i < conns.getNumChildren(); ++i)
        {
            auto c = conns.getChild(i);
            if (! c.isValid()) continue;

            auto srcId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(c.getProperty(IDs::src));
            auto dstId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(c.getProperty(IDs::dest));

            if (auto it = nodeIdMap.find(srcId); it != nodeIdMap.end())
                c.setProperty(IDs::src, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(it->second), nullptr);
            if (auto it = nodeIdMap.find(dstId); it != nodeIdMap.end())
                c.setProperty(IDs::dest, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(it->second), nullptr);
        }
    }

    // MODCONNECTIONS remap (NodeID-only)
    if (auto mconns = piano.getChildWithName(IDs::MODCONNECTIONS); mconns.isValid())
    {
        for (int i = 0; i < mconns.getNumChildren(); ++i)
        {
            auto c = mconns.getChild(i);
            if (! c.isValid()) continue;

            if (! c.hasProperty(IDs::src) || ! c.hasProperty(IDs::dest))
                continue;

            auto srcId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(c.getProperty(IDs::src));
            auto dstId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(c.getProperty(IDs::dest));

            if (auto it = nodeIdMap.find(srcId); it != nodeIdMap.end())
                c.setProperty(IDs::src, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(it->second), nullptr);
            if (auto it = nodeIdMap.find(dstId); it != nodeIdMap.end())
                c.setProperty(IDs::dest, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(it->second), nullptr);
        }
    }
}

void HeaderSection::deletePiano()
{
    // Count pianos and locate the active one (by child index in gallery)
    int numPianos = 0;
    int activeChildIdx = -1;

    const int numChildren = gallery.getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        auto child = gallery.getChild(i);
        if (! child.hasType(IDs::PIANO))
            continue;

        ++numPianos;
        if ((bool) child.getProperty(IDs::isActive))
            activeChildIdx = i;
    }

    // Need at least 2 pianos to delete one, and we require a valid active piano
    if (numPianos <= 1 || activeChildIdx < 0)
        return;

    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (interface != nullptr)
        interface->setPianoSwitchTriggerThreadMessage();

    // Choose which piano to activate after deletion: prefer next, otherwise previous
    int targetChildIdx = -1;
    for (int i = activeChildIdx + 1; i < numChildren; ++i)
    {
        auto c = gallery.getChild(i);
        if (c.hasType(IDs::PIANO)) { targetChildIdx = i; break; }
    }
    if (targetChildIdx < 0)
    {
        for (int i = activeChildIdx - 1; i >= 0; --i)
        {
            auto c = gallery.getChild(i);
            if (c.hasType(IDs::PIANO)) { targetChildIdx = i; break; }
        }
    }

    // If we couldn't find another piano (shouldn’t happen because numPianos > 1), bail safely
    if (targetChildIdx < 0)
        return;

    // Capture the node to activate BEFORE removal (indices are valid now)
    juce::ValueTree pianoToActivate = gallery.getChild(targetChildIdx);

    // Remove the active piano
    gallery.removeChild(activeChildIdx, nullptr);

    // Deactivate all pianos, then activate the target
    for (int i = 0; i < gallery.getNumChildren(); ++i)
    {
        auto c = gallery.getChild(i);
        if (! c.hasType(IDs::PIANO))
            continue;

        const bool setActive = (c == pianoToActivate);
        c.setProperty(IDs::isActive, setActive ? 1 : 0, nullptr);
    }

    // Update UI text to reflect the new active piano
    if (pianoToActivate.isValid())
        pianoSelectText->setText(pianoToActivate.getProperty(IDs::name).toString());

    if (interface != nullptr)
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

        PopupItems disabledItem ("separator");
        disabledItem.enabled = false; // This makes it non-selectable
        disabledItem.id = -1; // will be a separator line

        options.addItem(itemCounter++, "Add");
        options.addItem(itemCounter++, "Rename");
        options.addItem(itemCounter++, "Duplicate");
        options.addItem(itemCounter++, "Delete");
        options.addItem(disabledItem); // create separator line

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
                        duplicatePiano(getActivePianoCopy());
                        break;
                    case 3:
                        DBG("delete piano");
                        deletePiano();
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
