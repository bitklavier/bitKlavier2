//
// Created by Dan Trueman on 6/28/25.
//

#include "SemitoneWidthSection.h"

SemitoneWidthSection::SemitoneWidthSection (
    juce::String name,
    SemitoneWidthParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent) : SynthSection(name)
{
    setComponentID(parent.getComponentID());

    widthSlider_ = std::make_unique<SynthSlider>(params.semitoneWidthSliderParam->paramID,params.semitoneWidthSliderParam->getModParam());
    addSlider(widthSlider_.get());
    widthSlider_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    widthSlider_->setPopupPlacement(juce::BubbleComponent::below);
    widthSlider_->setShowPopupOnHover(true);
    widthSliderAttachment = std::make_unique<chowdsp::SliderAttachment>(params.semitoneWidthSliderParam, listeners, *widthSlider_, nullptr);
    widthSlider_->addAttachment(widthSliderAttachment.get());

    fundamentalComboBox = std::make_unique<OpenGLComboBox>(params.reffundamental->paramID.toStdString());
    fundamentalComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.reffundamental, listeners, *fundamentalComboBox, nullptr);
    addComboBox(fundamentalComboBox.get(),true,true);

    octaveComboBox = std::make_unique<OpenGLComboBox>(params.octave->paramID.toStdString());
    octaveComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.octave, listeners, *octaveComboBox, nullptr);
    addComboBox(octaveComboBox.get(),true,true);
    sectionBorder.setName("semitonewidth");
    sectionBorder.setText("Semitone Width");
    sectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sectionBorder);
}

SemitoneWidthSection::~SemitoneWidthSection() { }

void SemitoneWidthSection::paintBackground(juce::Graphics& g) {

    setLabelFont(g);
    drawLabelForComponent(g, TRANS("cents"), widthSlider_.get());

    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
    sectionBorder.paint(g);
}

void SemitoneWidthSection::setAlpha(float newAlpha)
{
    widthSlider_->setAlpha(newAlpha);
    widthSlider_->redoImage();
    fundamentalComboBox->setAlpha(newAlpha);
    fundamentalComboBox->redoImage();
    octaveComboBox->setAlpha(newAlpha);
    octaveComboBox->redoImage();
}

void SemitoneWidthSection::resized() {

    juce::Rectangle<int> area (getLocalBounds());
    sectionBorder.setBounds(area);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    area.reduce(largepadding, largepadding);

    juce::Rectangle<int> widthKnobArea = area.removeFromLeft(getKnobSectionHeight());
    widthSlider_->setBounds(widthKnobArea);

    area.removeFromLeft(largepadding);
    area.removeFromTop(largepadding);

    int comboboxheight = findValue(Skin::kComboMenuHeight);

    juce::Rectangle<int> fundamentalComboBoxArea = area.removeFromTop(comboboxheight);
    fundamentalComboBox->setBounds(fundamentalComboBoxArea);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int> octaveComboBoxArea = area.removeFromTop(comboboxheight);
    octaveComboBox->setBounds(octaveComboBoxArea);

    SynthSection::resized();
}
