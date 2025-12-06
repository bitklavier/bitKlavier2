//
// Created by Dan Trueman on 7/11/25.
//

#include "MidiFilterParametersView.h"
MidiFilterParametersView::MidiFilterParametersView(chowdsp::PluginState& pluginState, MidiFilterParams& param, juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
{
    setName ("midifilter");
    setLookAndFeel (DefaultLookAndFeel::instance());
    setComponentID (name);

    prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
    addOpenGlComponent(prepTitle);
    prepTitle->setJustification(juce::Justification::centredLeft);
    prepTitle->setFontType (PlainTextComponent::kTitle);
    prepTitle->setRotation (-90);

    auto& listeners = pluginState.getParameterListeners();

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

    midiFilterCallbacks += {listeners.addParameterListener(
        params.toggleNoteMessages,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            for (auto& pt : _paramToggles)
            {
                if(pt->getName() != "toggleNoteMessages")
                {
                    if (*params.toggleNoteMessages) pt->setEnabled(false);
                    else pt->setEnabled(true);
                }
            }
        })
    };

    midiFilterCallbacks += {listeners.addParameterListener(
        params.allNotesOff,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            for (auto& pt : _paramToggles)
            {
                if( pt->getName() != "allNotesOff" &&
                    pt->getName() != "ignoreNoteOn" &&
                    pt->getName() != "ignoreNoteOff" &&
                    pt->getName() != "ignoreSustainPedal")
                {
                    if (*params.allNotesOff) pt->setEnabled(false);
                    else pt->setEnabled(true);
                }
            }
        })
    };

    midiFilterCallbacks += {listeners.addParameterListener(
        params.notesAreSustainPedal,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            for (auto& pt : _paramToggles)
            {
                if( pt->getName() != "notesAreSustainPedal" &&
                    pt->getName() != "ignoreSustainPedal")
                {
                    if (*params.notesAreSustainPedal) pt->setEnabled(false);
                    else pt->setEnabled(true);
                }
            }
        })
    };

    midiFilterCallbacks += {listeners.addParameterListener(
        params.notesAreSostenutoPedal,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            for (auto& pt : _paramToggles)
            {
                if( pt->getName() != "notesAreSostenutoPedal"&&
                    pt->getName() != "ignoreSustainPedal")
                {
                    if (*params.notesAreSostenutoPedal) pt->setEnabled(false);
                    else pt->setEnabled(true);
                }
            }
        })
    };

    midiFilterCallbacks += {listeners.addParameterListener(
        params.sostenutoMode,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            for (auto& pt : _paramToggles)
            {
                if( pt->getName() == "ignoreSustainPedal")
                {
                    if (*params.sostenutoMode) pt->setEnabled(false);
                    else pt->setEnabled(true);
                }
            }
        })
    };
}


void MidiFilterParametersView::resized()
{
    juce::Rectangle<int> area (getLocalBounds());
    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int title_width = getTitleWidth();

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize (findValue(Skin::kPrepTitleSize));

    area.reduce(title_width, 0);
    int column_width = area.getWidth() / 3.;
    juce::Rectangle<int> buttonsColumn = area.removeFromLeft(column_width);
    buttonsColumn.reduce(smallpadding, smallpadding);

    for (auto& pt : _paramToggles)
    {
        pt->setBounds(buttonsColumn.removeFromTop(comboboxheight));
        buttonsColumn.removeFromTop(smallpadding);
    }

    SynthSection::resized();
}
