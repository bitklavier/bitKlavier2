//
// Created by Junie on 3/4/26.
//

#include "TempoAdaptiveSection.h"

TempoAdaptiveSection::TempoAdaptiveSection (chowdsp::PluginState& pluginState, TempoParams& params, juce::String name, TempoProcessor* proc)
    : SynthSection (""), processor(proc), params(params)
{
    setLookAndFeel (DefaultLookAndFeel::instance());
    setComponentID (name);
    setSkinOverride(Skin::kDirect);

    auto& listeners = pluginState.getParameterListeners();

    historySlider = std::make_unique<SynthSlider> (params.historyParam->paramID, params.historyParam->getModParam());
    historyAttachment= std::make_unique<chowdsp::SliderAttachment> (*params.historyParam.get(), listeners, *historySlider.get(), nullptr);
    historySlider->addAttachment(historyAttachment.get());
    addSlider (historySlider.get());
    historySlider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    historySlider->setShowPopupOnHover(true);
    historyLabel = std::make_shared<PlainTextComponent>(historySlider->getName(), params.historyParam->getName(20));
    addOpenGlComponent(historyLabel);
    historyLabel->setJustification(juce::Justification::centred);

    timeWindowMinMaxSlider = std::make_unique<OpenGL_HoldTimeMinMaxSlider>(&params.timeWindowMinMaxParams, listeners);
    timeWindowMinMaxSlider->setComponentID ("timewindow_min_max");
    timeWindowMinMaxSlider->setTitletext ("Time Window min/max (ms)");
    addStateModulatedComponent (timeWindowMinMaxSlider.get());

    adaptiveKnobsBorder = std::make_shared<OpenGL_LabeledBorder>("adaptive knobs border", "Adaptive Parameters");
    addBorder(adaptiveKnobsBorder.get());

    currentTempoDisplay = std::make_shared<PlainTextComponent>("currenttempo", "Current Tempo = 120.00bpm");
    addOpenGlComponent(currentTempoDisplay);
    currentTempoDisplay->setTextSize (12.0f);
    currentTempoDisplay->setJustification(juce::Justification::centred);

    adaptiveMultiplierDisplay = std::make_shared<PlainTextComponent>("adaptivemultiplier", "Multiplier = 1.00");
    addOpenGlComponent(adaptiveMultiplierDisplay);
    adaptiveMultiplierDisplay->setTextSize (12.0f);
    adaptiveMultiplierDisplay->setJustification(juce::Justification::centred);

    resetButton = std::make_unique<SynthButton>("reset");
    resetButton->setButtonText("Reset");
    resetButton->onClick = [this] { if (processor) processor->adaptiveReset(); };
    addSynthButton(resetButton.get());
}

void TempoAdaptiveSection::paintBackground (juce::Graphics& g)
{
    setLabelFont(g);
    if (isVisible())
    {
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }
}

void TempoAdaptiveSection::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds();
    adaptiveKnobsBorder->setBounds(bounds);

    int large_padding = findValue(Skin::kLargePadding);
    int knob_section_height = getKnobSectionHeight();
    int labelsectionheight = findValue(Skin::kLabelHeight);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeSmall);

    bounds.reduce (large_padding, large_padding);
    bounds.removeFromTop(large_padding);

    juce::Rectangle<int> knobsArea = bounds.removeFromTop(knob_section_height + large_padding * 3);
    
    // Adjust bounds for sliders to align their centers
    // historySlider is a rotary knob, timeWindowMinMaxSlider is a horizontal range slider.
    // We want their horizontal center lines to coincide.
    int historySliderHeight = knob_section_height;
    int timeWindowHeight = knob_section_height * 1.1;
    
    int historySliderY = knobsArea.getY() + (knobsArea.getHeight() - historySliderHeight) / 2;
    int timeWindowY = knobsArea.getY() + (knobsArea.getHeight() - timeWindowHeight) / 2;

    historySlider->setBounds(knobsArea.removeFromLeft(knobsArea.getWidth() / 3).withSizeKeepingCentre(knob_section_height, historySliderHeight).withY(historySliderY));
    timeWindowMinMaxSlider->setBounds(knobsArea.withSizeKeepingCentre(knobsArea.getWidth(), timeWindowHeight).withY(timeWindowY));

    juce::Rectangle<int> history_label_rect (historySlider->getX(), historySlider->getY() + historySlider->getHeight() / 2 + 20, historySlider->getWidth(), labelsectionheight );
    historyLabel->setBounds(history_label_rect);
    historyLabel->setTextSize (knobLabelSize);

    auto bottomArea = bounds.removeFromBottom(labelsectionheight * 3).withSizeKeepingCentre(400, labelsectionheight * 3);
    bottomArea.removeFromBottom(large_padding);
    auto currentTempoBox = bottomArea.removeFromRight(150).reduced(10, 2);
    currentTempoDisplay->setBounds(currentTempoBox);
    auto resetBox = bottomArea.removeFromRight(100).reduced(10, 2);
    resetButton->setBounds(resetBox);
    adaptiveMultiplierDisplay->setBounds(bottomArea.reduced(10, 2));

    SynthSection::resized();
}

void TempoAdaptiveSection::updateDisplays()
{
    if (processor)
    {
        auto& p = processor->getState().params;
        if (p.subdivisionsParam == nullptr || p.tempoModeOptions == nullptr)
            return;

        currentTempoDisplay->setText("Current Tempo = " + juce::String(60.f / (processor->getCurrentPulseLength_seconds() * *p.subdivisionsParam), 2) + "bpm");
        adaptiveMultiplierDisplay->setText("Multiplier = " + juce::String(1. / processor->adaptiveTempoPeriodMultiplier, 2));

        resetButton->setToggleState(false, juce::dontSendNotification);
    }
}
