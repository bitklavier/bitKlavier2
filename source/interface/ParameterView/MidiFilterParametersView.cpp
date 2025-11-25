//
// Created by Dan Trueman on 7/11/25.
//

#include "MidiFilterParametersView.h"
MidiFilterParametersView::MidiFilterParametersView(chowdsp::PluginState& pluginState, MidiFilterParams& param, juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
{
    setName ("midifilter");
    setLookAndFeel (DefaultLookAndFeel::instance());
    setComponentID (name);

    auto& listeners = pluginState.getParameterListeners();

    /**
     * todo: if toggleNoteMessages is set to true, the grey out other toggles, and similarly
     *          grey out toggleNoteMessages if any of the others are true
     */

    for (auto& param_ : *params.getBoolParams())
    {
        // create and add each button
        auto button = std::make_unique<SynthButton> (param_->paramID);
        auto attachment = std::make_unique<chowdsp::ButtonAttachment> (*param_.get(), listeners, *button.get(), nullptr);
        addSynthButton(button.get());

        // we need to keep these around, so we stick them in these vectors that are part of the class
        _paramToggles_attachments.emplace_back (std::move (attachment));
        _paramToggles.emplace_back (std::move (button));
    }
}

void MidiFilterParametersView::resized()
{
    juce::Rectangle<int> area (getLocalBounds());
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int title_width = getTitleWidth();
    area.reduce(title_width, 0);
    int column_width = area.getWidth() / 4.;
    juce::Rectangle<int> buttonsColumn = area.removeFromLeft(column_width);
    buttonsColumn.reduce(smallpadding, smallpadding);

    for (auto& pt : _paramToggles)
    {
        pt->setBounds(buttonsColumn.removeFromTop(comboboxheight));
        buttonsColumn.removeFromTop(smallpadding);
    }

    SynthSection::resized();
}
