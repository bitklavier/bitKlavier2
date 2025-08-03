//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetParametersView.h"
MidiTargetParametersView::MidiTargetParametersView(chowdsp::PluginState& pluginState, MidiTargetParams& param, juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
{
    setName ("miditarget");
    setLookAndFeel (DefaultLookAndFeel::instance());
    setComponentID (name);

    auto& listeners = pluginState.getParameterListeners();

    // make the buttons and attachments
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

    // do the same for the note mode menus
    for (auto& mparam_ : *params.getChoiceParams())
    {
        auto menu = std::make_unique<OpenGLComboBox>(mparam_->paramID.toStdString());
        auto menu_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*mparam_.get(), listeners, *menu, nullptr);
        addAndMakeVisible(menu.get());
        addOpenGlComponent(menu->getImageComponent());

        _noteModeMenus_attachments.emplace_back (std::move(menu_attachment));
        _noteModeMenus.emplace_back (std::move (menu));
    }
}

void MidiTargetParametersView::resized()
{
    juce::Rectangle<int> area (getLocalBounds());

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int title_width = getTitleWidth();

    area.reduce(title_width, 0);
    int column_width = area.getWidth() / 4.;

    juce::Rectangle<int> blendronicColumn = area.removeFromLeft(column_width);
    juce::Rectangle<int> blendronicButtonsColumn = blendronicColumn.removeFromLeft(blendronicColumn.getWidth() / 2);
    blendronicColumn.reduce(smallpadding, smallpadding);
    blendronicButtonsColumn.reduce(smallpadding, smallpadding);

    for(int i = BlendronicTargetPatternSync; i<BlendronicTargetNil; i++)
    {
        _paramToggles[i - BlendronicTargetPatternSync]->setBounds(blendronicButtonsColumn.removeFromTop(comboboxheight));
        blendronicButtonsColumn.removeFromTop(smallpadding);

        _noteModeMenus[i - BlendronicTargetPatternSync]->setBounds(blendronicColumn.removeFromTop(comboboxheight));
        blendronicColumn.removeFromTop(smallpadding);
    }

    SynthSection::resized();
}