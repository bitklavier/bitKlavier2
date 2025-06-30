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

#include "skin.h"
#include "BinaryData.h"
#include "default_look_and_feel.h"
#include "FullInterface.h"
#include "synth_section.h"

namespace {
    const std::string kOverrideNames[Skin::kNumSectionOverrides] = {
            "All",
            "Logo",
            "Header",
            "Overlays",
            "Popup Browser",
            "Preset Browser",
            "Construction Site",
            "direct",
            "keymap",
            "nostalgic",
            "synchronic",
            "blendronic",
            "resonance",
            "tuning",
            "tempo",
            "modulation",
            "Modulation Drag Drop"

    };

    const std::string kValueNames[Skin::kNumSkinValueIds] = {
            "Body Rounding",
            "Label Height",
            "Label Background Height",
            "Label Rounding",
            "Label Offset",
            "Text Component Label Offset",
            "Rotary Option X Offset",
            "Rotary Option Y Offset",
            "Rotary Option Width",
            "Title Width",
            "Padding",
            "Large Padding",
            "Slider Width",
            "Text Component Height",
            "Text Component Offset",
            "Text Component Font Size",
            "Text Button Height",
            "Button Font Size",
            "Knob Arc Size",
            "Knob Arc Thickness",
            "Knob Body Size",
            "Knob Handle Length",
            "Knob Mod Amount Arc Size",
            "Knob Mod Amount Arc Thickness",
            "Knob Mod Meter Arc Size",
            "Knob Mod Meter Arc Thickness",
            "Knob Offset",
            "Knob Section Height",
            "Knob Shadow Width",
            "Knob Shadow Offset",
            "Modulation Button Width",
            "Modulation Font Size",
            "Widget Margin",
            "Widget Rounded Corner",
            "Widget Line Width",
            "Widget Line Boost",
            "Widget Fill Center",
            "Widget Fill Fade",
            "Widget Fill Boost",
            "Wavetable Horizontal Angle",
            "Wavetable Vertical Angle",
            "Wavetable Draw Width",
            "Wavetable Draw Height",
            "Wavetable Y Offset",
            "Combo Menu Height"
    };

    const std::string kColorNames[Skin::kNumColors] = {
            "Background",
            "Body",
            "Body Heading Background",
            "Heading Text",
            "Preset Text",
            "Body Text",
            "Border",
            "Label Background",
            "Label Connection",
            "Power Button On",
            "Power Button Off",

            "Overlay Screen",
            "Lighten Screen",
            "Shadow",
            "Popup Selector Background",
            "Popup Background",
            "Popup Border",

            "Text Component Background",
            "Text Component Text",

            "Rotary Arc",
            "Rotary Arc Disabled",
            "Rotary Arc Unselected",
            "Rotary Arc Unselected Disabled",
            "Rotary Hand",
            "Rotary Body",
            "Rotary Body Border",

            "Linear Slider",
            "Linear Slider Disabled",
            "Linear Slider Unselected",
            "Linear Slider Thumb",
            "Linear Slider Thumb Disabled",

            "Widget Center Line",
            "Widget Primary 1",
            "Widget Primary 2",
            "Widget Primary Disabled",
            "Widget Secondary 1",
            "Widget Secondary 2",
            "Widget Secondary Disabled",
            "Widget Accent 1",
            "Widget Accent 2",
            "Widget Background",

            "Modulation Meter",
            "Modulation Meter Left",
            "Modulation Meter Right",
            "Modulation Meter Control",
            "Modulation Button Selected",
            "Modulation Button Dragging",
            "Modulation Button Unselected",

            "Icon Selector Icon",

            "Icon Button Off",
            "Icon Button Off Hover",
            "Icon Button Off Pressed",
            "Icon Button On",
            "Icon Button On Hover",
            "Icon Button On Pressed",

            "UI Button",
            "UI Button Text",
            "UI Button Hover",
            "UI Button Press",
            "UI Action Button",
            "UI Action Button Hover",
            "UI Action Button Press",

            "Text Editor Background",
            "Text Editor Border",
            "Text Editor Caret",
            "Text Editor Selection"

    };
} // namespace

bool Skin::shouldScaleValue(ValueId value_id) {
    return
            value_id != kFrequencyDisplay;
}

Skin::Skin() {

    loadDefaultSkin();

    copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
}

void Skin::clearSkin() {
    for (int i = 0; i < kNumSectionOverrides; ++i)
        color_overrides_[i].clear();
    for (int i = 0; i < kNumSectionOverrides; ++i)
        value_overrides_[i].clear();
}

void Skin::loadDefaultSkin() {
    juce::MemoryInputStream skin((const void*)BinaryData::default_bitklavierskin, BinaryData::default_bitklavierskinSize, false);

    std::string skin_string = skin.readEntireStreamAsString().toStdString();

    try {
        json data = json::parse(skin_string, nullptr, false);
        jsonToState(data);
    }
    catch (const json::exception& e) {
    }
}

void Skin::setComponentColors(juce::Component* component) const {
    for (int i = 0; i < Skin::kNumColors; ++i) {
        int color_id = i + Skin::kInitialColor;
        juce::Colour color = getColor(static_cast<Skin::ColorId>(color_id));
        component->setColour(color_id, color);
    }
}

void Skin::setComponentColors(juce::Component* component, SectionOverride section_override, bool top_level) const {
    if (top_level) {
        setComponentColors(component);
        return;
    }

    for (int i = kInitialColor; i < kFinalColor; ++i)
        component->removeColour(i);

    for (const auto& color : color_overrides_[section_override])
        component->setColour(color.first, color.second);
}

void Skin::setComponentValues(SynthSection* component) const {
    std::map<ValueId, float> values;
    for (int i = 0; i < kNumSkinValueIds; ++i)
        values[(ValueId)i] = values_[i];

    component->setSkinValues(values);
}

void Skin::setComponentValues(SynthSection* component, SectionOverride section_override, bool top_level) const {
    if (top_level) {
        setComponentValues(component);
        return;
    }
    component->setSkinValues(value_overrides_[section_override]);
}

bool Skin::overridesColor(int section, ColorId color_id) const {
    if (section == kNone)
        return true;

    return color_overrides_[section].count(color_id) > 0;
}

bool Skin::overridesValue(int section, ValueId value_id) const {
    if (section == kNone)
        return true;

    return value_overrides_[section].count(value_id) > 0;
}

void Skin::copyValuesToLookAndFeel(juce::LookAndFeel* look_and_feel) const {
    look_and_feel->setColour(juce::PopupMenu::backgroundColourId, getColor(Skin::kPopupBackground));
    look_and_feel->setColour(juce::PopupMenu::textColourId, getColor(Skin::kBodyText));
    look_and_feel->setColour(juce::TooltipWindow::textColourId, getColor(Skin::kBodyText));

    look_and_feel->setColour(juce::BubbleComponent::backgroundColourId, getColor(Skin::kPopupBackground));
    look_and_feel->setColour(juce::BubbleComponent::outlineColourId, getColor(Skin::kPopupBorder));

    for (int i = kInitialColor; i < kFinalColor; ++i)
        look_and_feel->setColour(i, getColor(static_cast<Skin::ColorId>(i)));
}

juce::Colour Skin::getColor(int section, ColorId color_id) const {
    if (section == kNone)
        return getColor(color_id);

    if (color_overrides_[section].count(color_id))
        return color_overrides_[section].at(color_id);

    return juce::Colours::black;
}

float Skin::getValue(int section, ValueId value_id) const {
    if (value_overrides_[section].count(value_id))
        return value_overrides_[section].at(value_id);

    return getValue(value_id);
}

void Skin::addOverrideColor(int section, ColorId color_id, juce::Colour color) {
    if (section == kNone)
        setColor(color_id, color);
    else
        color_overrides_[section][color_id] = color;
}

void Skin::removeOverrideColor(int section, ColorId color_id) {
    if (section != kNone)
        color_overrides_[section].erase(color_id);
}

void Skin::addOverrideValue(int section, ValueId value_id, float value) {
    if (section == kNone)
        setValue(value_id, value);
    else
        value_overrides_[section][value_id] = value;
}

void Skin::removeOverrideValue(int section, ValueId color_id) {
    if (section != kNone)
        value_overrides_[section].erase(color_id);
}

json Skin::stateToJson() {
    json data;
    for (int i = 0; i < kNumColors; ++i)
        data[kColorNames[i]] = colors_[i].toString().toStdString();

    for (int i = 0; i < kNumSkinValueIds; ++i)
        data[kValueNames[i]] = values_[i];

    json overrides;
    for (int override_index = 0; override_index < kNumSectionOverrides; ++override_index) {
        json override_section;
        for (const auto& color : color_overrides_[override_index]) {
            int index = color.first - Skin::kInitialColor;
            override_section[kColorNames[index]] = color.second.toString().toStdString();
        }

        for (const auto& value : value_overrides_[override_index]) {
            int index = value.first;
            override_section[kValueNames[index]] = value.second;
        }

        overrides[kOverrideNames[override_index]] = override_section;
    }

    data["overrides"] = overrides;
    data["synth_version"] = 2;//ProjectInfo::versionNumber;

    return data;
}

juce::String Skin::stateToString() {
    return stateToJson().dump();
}

void Skin::saveToFile(juce::File destination) {
    destination.replaceWithText(stateToString());
}

json Skin::updateJson(json data) {
    int version = 0;

    return data;
}

void Skin::jsonToState(json data) {
    clearSkin();
    data = updateJson(data);

    if (data.count("overrides")) {
        json overrides = data["overrides"];
        for (int override_index = 0; override_index < kNumSectionOverrides; ++override_index) {
            std::string name = kOverrideNames[override_index];
            color_overrides_[override_index].clear();
            value_overrides_[override_index].clear();

            if (overrides.count(name) == 0)
                continue;

            json override_section = overrides[name];
            for (int i = 0; i < kNumColors; ++i) {
                if (override_section.count(kColorNames[i])) {
                    ColorId color_id = static_cast<Skin::ColorId>(i + Skin::kInitialColor);

                    std::string color_string = override_section[kColorNames[i]];
                    color_overrides_[override_index][color_id] = juce::Colour::fromString(color_string);
                }
            }

            for (int i = 0; i < kNumSkinValueIds; ++i) {
                if (override_section.count(kValueNames[i])) {
                    Skin::ValueId value_id = static_cast<Skin::ValueId>(i);
                    float value = override_section[kValueNames[i]];
                    value_overrides_[override_index][value_id] = value;
                }
            }
        }
    }

    for (int i = 0; i < kNumColors; ++i) {
        if (data.count(kColorNames[i])) {
            std::string color_string = data[kColorNames[i]];
            colors_[i] = juce::Colour::fromString(color_string);
        }
    }

    for (int i = 0; i < kNumSkinValueIds; ++i) {
        if (data.count(kValueNames[i]))
            values_[i] = data[kValueNames[i]];
        else
            values_[i] = 0.0f;
    }
}

bool Skin::stringToState(juce::String skin_string) {
    try {
        json data = json::parse(skin_string.toStdString(), nullptr, false);
        jsonToState(data);
    }
    catch (const json::exception& e) {
        return false;
    }
    return true;
}

bool Skin::loadFromFile(juce::File source) {
    return stringToState(source.loadFileAsString());
}

