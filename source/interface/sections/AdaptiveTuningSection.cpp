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

    adaptiveIntervalScale_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveIntervalScale->paramID.toStdString());
    adaptiveIntervalScale_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveIntervalScale, listeners, *adaptiveIntervalScale_ComboBox, nullptr);
    addAndMakeVisible(adaptiveIntervalScale_ComboBox.get());
    addOpenGlComponent(adaptiveIntervalScale_ComboBox->getImageComponent());

    adaptiveAnchorScale_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveAnchorScale->paramID.toStdString());
    adaptiveAnchorScale_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveAnchorScale, listeners, *adaptiveAnchorScale_ComboBox, nullptr);
    addAndMakeVisible(adaptiveAnchorScale_ComboBox.get());
    addOpenGlComponent(adaptiveAnchorScale_ComboBox->getImageComponent());

    adaptiveAnchorFundamental_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveAnchorFundamental->paramID.toStdString());
    adaptiveAnchorFundamental_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveAnchorFundamental, listeners, *adaptiveAnchorFundamental_ComboBox, nullptr);
    addAndMakeVisible(adaptiveAnchorFundamental_ComboBox.get());
    addOpenGlComponent(adaptiveAnchorFundamental_ComboBox->getImageComponent());

    useInversionOfIntervalScale_Toggle = std::make_unique<SynthButton>(params.tAdaptiveInversional->paramID);
    useInversionOfIntervalScale_ToggleAttachment = std::make_unique<chowdsp::ButtonAttachment>(params.tAdaptiveInversional,listeners,*useInversionOfIntervalScale_Toggle,nullptr);
    useInversionOfIntervalScale_Toggle->setComponentID(params.tAdaptiveInversional->paramID);
    addSynthButton(useInversionOfIntervalScale_Toggle.get(), true);
    useInversionOfIntervalScale_Toggle->setText("invert?");

    sectionBorder.setName("adaptivetuning");
    sectionBorder.setText("Adaptive Tuning");
    sectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sectionBorder);
}

AdaptiveTuningSection::~AdaptiveTuningSection() { }

void AdaptiveTuningSection::paintBackground(juce::Graphics& g) {
    setLabelFont(g);
    //drawLabelForComponent(g, TRANS("Cents"), widthSlider_.get());

    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    sectionBorder.paint(g);
}

void AdaptiveTuningSection::resized() {

    juce::Rectangle<int> area (getLocalBounds());
    sectionBorder.setBounds(area);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    area.reduce(largepadding, largepadding);

//    juce::Rectangle<int> widthKnobArea = area.removeFromLeft(getKnobSectionHeight());
//    widthSlider_->setBounds(widthKnobArea);
//
//    area.removeFromLeft(largepadding);
//    area.removeFromTop(largepadding);
//
//    int comboboxheight = findValue(Skin::kComboMenuHeight);
//
//    juce::Rectangle<int> fundamentalComboBoxArea = area.removeFromTop(comboboxheight);
//    fundamentalComboBox->setBounds(fundamentalComboBoxArea);
//
//    area.removeFromTop(smallpadding);
//
//    juce::Rectangle<int> octaveComboBoxArea = area.removeFromTop(comboboxheight);
//    octaveComboBox->setBounds(octaveComboBoxArea);

    SynthSection::resized();
}

