//
// Created by Dan Trueman on 7/4/25.
//
#include "AdaptiveTuningSection.h"

AdaptiveTuningSection::AdaptiveTuningSection (
    juce::String name,
    AdaptiveTuningParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent) : SynthSection(name)
{
    setComponentID(parent.getComponentID());

    clusterThreshold_Slider = std::make_unique<SynthSlider>(params.tAdaptiveClusterThresh->paramID);
    addSlider(clusterThreshold_Slider.get());
    clusterThreshold_Slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    clusterThreshold_Slider->setPopupPlacement(juce::BubbleComponent::below);
    clusterThreshold_SliderAttachment = std::make_unique<chowdsp::SliderAttachment>(params.tAdaptiveClusterThresh, listeners, *clusterThreshold_Slider, nullptr);

    history_Slider = std::make_unique<SynthSlider>(params.tAdaptiveHistory->paramID);
    addSlider(history_Slider.get());
    history_Slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    history_Slider->setPopupPlacement(juce::BubbleComponent::below);
    history_SliderAttachment = std::make_unique<chowdsp::SliderAttachment>(params.tAdaptiveHistory, listeners, *history_Slider, nullptr);

    if (auto* tuningParams = dynamic_cast<AdaptiveTuningParams*>(&params)) {
        auto index = tuningParams->tAdaptiveIntervalScale->getIndex();
        adaptiveIntervalScale_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveIntervalScale->paramID.toStdString());
        adaptiveIntervalScale_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveIntervalScale, listeners, *adaptiveIntervalScale_ComboBox, nullptr);
        addAndMakeVisible(adaptiveIntervalScale_ComboBox.get());
        addOpenGlComponent(adaptiveIntervalScale_ComboBox->getImageComponent());
        setupTuningSystemMenu(adaptiveIntervalScale_ComboBox);
        adaptiveIntervalScale_ComboBox->setSelectedItemIndex(index,juce::sendNotificationSync);

        index = tuningParams->tAdaptiveAnchorScale->getIndex();
        adaptiveAnchorScale_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveAnchorScale->paramID.toStdString());
        adaptiveAnchorScale_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveAnchorScale, listeners, *adaptiveAnchorScale_ComboBox, nullptr);
        addAndMakeVisible(adaptiveAnchorScale_ComboBox.get());
        addOpenGlComponent(adaptiveAnchorScale_ComboBox->getImageComponent());
        setupTuningSystemMenu(adaptiveAnchorScale_ComboBox);
        adaptiveAnchorScale_ComboBox->setSelectedItemIndex(index,juce::sendNotificationSync);
    }

    adaptiveAnchorFundamental_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveAnchorFundamental->paramID.toStdString());
    adaptiveAnchorFundamental_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveAnchorFundamental, listeners, *adaptiveAnchorFundamental_ComboBox, nullptr);
    addAndMakeVisible(adaptiveAnchorFundamental_ComboBox.get());
    addOpenGlComponent(adaptiveAnchorFundamental_ComboBox->getImageComponent());

    useInversionOfIntervalScale_Toggle = std::make_unique<SynthButton>(params.tAdaptiveInversional->paramID);
    useInversionOfIntervalScale_ToggleAttachment = std::make_unique<chowdsp::ButtonAttachment>(params.tAdaptiveInversional,listeners,*useInversionOfIntervalScale_Toggle,nullptr);
    useInversionOfIntervalScale_Toggle->setComponentID(params.tAdaptiveInversional->paramID);
    addSynthButton(useInversionOfIntervalScale_Toggle.get(), true);
    useInversionOfIntervalScale_Toggle->setText("invert?");

    resetButton = std::make_unique<SynthButton>("reset");
    resetButton_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.tReset,listeners,*resetButton,nullptr);
    resetButton->setComponentID("reset");
    addSynthButton(resetButton.get(), true);
    resetButton->setText("reset!");
    resetButton->setToggleable(true); // this one is just to trigger a reset to the adaptive system

    currentFundamental = std::make_shared<PlainTextComponent>("currentfundamental", "Current Fundamental = C");
    addOpenGlComponent(currentFundamental);
    currentFundamental->setTextSize (12.0f);
    currentFundamental->setJustification(juce::Justification::centred);

    intervalsLabel = std::make_shared<PlainTextComponent>("intervals", "Intervals");
    addOpenGlComponent(intervalsLabel);
    intervalsLabel->setTextSize (12.0f);
    intervalsLabel->setJustification(juce::Justification::centred);

    anchorsLabel = std::make_shared<PlainTextComponent>("anchors", "Anchors");
    addOpenGlComponent(anchorsLabel);
    anchorsLabel->setTextSize (12.0f);
    anchorsLabel->setJustification(juce::Justification::centred);

    sectionBorder.setName("adaptivetuning");
    sectionBorder.setText("Adaptive Tuning");
    sectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sectionBorder);

}

AdaptiveTuningSection::~AdaptiveTuningSection() { }

void AdaptiveTuningSection::paintBackground(juce::Graphics& g) {
    setLabelFont(g);
    drawLabelForComponent(g, TRANS("Cluster Threshold (ms)"), clusterThreshold_Slider.get());
    drawLabelForComponent(g, TRANS("History (notes)"), history_Slider.get());

    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    sectionBorder.paint(g);
}

void AdaptiveTuningSection::resized() {

    juce::Rectangle<int> area (getLocalBounds());
    sectionBorder.setBounds(area);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int labelsectionheight = findValue(Skin::kLabelHeight);

    area.reduce(largepadding, largepadding);

    juce::Rectangle<int> tuningSystemLabelsBox = area.removeFromTop(comboboxheight);
    intervalsLabel->setBounds(tuningSystemLabelsBox.removeFromLeft(tuningSystemLabelsBox.getWidth() * 0.5));
    anchorsLabel->setBounds(tuningSystemLabelsBox);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int>comboBoxArea = area.removeFromTop(comboboxheight);
    juce::Rectangle<int>intervalComboBoxArea = comboBoxArea.removeFromLeft(comboBoxArea.getWidth() * 0.5);
    comboBoxArea.reduce(20,0);
    intervalComboBoxArea.reduce(20,0);
    adaptiveIntervalScale_ComboBox->setBounds(intervalComboBoxArea.removeFromLeft(intervalComboBoxArea.getWidth() * 0.5));
    useInversionOfIntervalScale_Toggle->setBounds(intervalComboBoxArea);
    adaptiveAnchorScale_ComboBox->setBounds(comboBoxArea.removeFromLeft(comboBoxArea.getWidth() * 0.5));
    adaptiveAnchorFundamental_ComboBox->setBounds(comboBoxArea);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int> knobsBox = area.removeFromTop(knobsectionheight + largepadding);
    clusterThreshold_Slider->setBounds(knobsBox.removeFromLeft(knobsBox.getWidth() * 0.5));
    history_Slider->setBounds(knobsBox);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int> resetBox = area.removeFromTop(comboboxheight);
    resetButton->setBounds(resetBox.removeFromLeft(resetBox.getWidth() * 0.5));
    currentFundamental->setBounds(resetBox);

    SynthSection::resized();
}
