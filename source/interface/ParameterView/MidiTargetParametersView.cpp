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

    if (auto* param_ = dynamic_cast<MidiTargetParams*> (&params))
    {
//        mf_button = std::make_unique<SynthButton> (param_->mftoggle->paramID);
//        mf_button_attachment = std::make_unique<chowdsp::ButtonAttachment> (*param_->mftoggle, listeners, *mf_button, nullptr);
//        addAndMakeVisible (mf_button.get());
//        addSynthButton (mf_button.get(), true);
    }
}

void MidiTargetParametersView::resized()
{
    int knob_section_height = getKnobSectionHeight();
    int knob_y = getHeight() - knob_section_height;

    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int area_width = getWidth() - 2 * title_width;
    int envelope_height = knob_y - widget_margin;

    mf_button->setBounds(115,10,100, 25);

    SynthSection::resized();
}