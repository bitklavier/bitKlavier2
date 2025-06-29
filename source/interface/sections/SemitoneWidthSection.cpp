//
// Created by Dan Trueman on 6/28/25.
//

#include "SemitoneWidthSection.h"
#include "synth_slider.h"

SemiToneWidthSection::SemiToneWidthSection (
    juce::String name,
    SemitoneWidthParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent) : SynthSection(name)
{
    setComponentID(parent.getComponentID());

    widthSlider_ = std::make_unique<SynthSlider>("width");
    addSlider(widthSlider_.get());
    widthSlider_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    widthSlider_->setPopupPlacement(juce::BubbleComponent::below);
    //widthSlider_->parentHierarchyChanged();
    widthSliderAttachment = std::make_unique<chowdsp::SliderAttachment>(params.semitoneWidthSliderParam, listeners, *widthSlider_, nullptr);

    fundamentalComboBox = std::make_unique<OpenGLComboBox>(params.reffundamental->paramID.toStdString());
    fundamentalComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.reffundamental, listeners, *fundamentalComboBox, nullptr);
    addAndMakeVisible(fundamentalComboBox.get());
    addOpenGlComponent(fundamentalComboBox->getImageComponent());

    octaveComboBox = std::make_unique<OpenGLComboBox>(params.octave->paramID.toStdString());
    octaveComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.octave, listeners, *octaveComboBox, nullptr);
    addAndMakeVisible(octaveComboBox.get());
    addOpenGlComponent(octaveComboBox->getImageComponent());

    sectionBorder.setName("semitonewidth");
    sectionBorder.setText("Semitone Width");
    sectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sectionBorder);
}

SemiToneWidthSection::~SemiToneWidthSection() { }

void SemiToneWidthSection::paintBackground(juce::Graphics& g) {
    setLabelFont(g);
    drawLabelForComponent(g, TRANS("Cents"), widthSlider_.get());

    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    sectionBorder.paint(g);
}

void SemiToneWidthSection::resized() {

    juce::Rectangle<int> area (getLocalBounds());
    sectionBorder.setBounds(area);

    //area.reduce(getKnobSectionHeight(), 0);
    area.reduce(10, 10);

    juce::Rectangle<int> widthKnobArea = area.removeFromLeft(getKnobSectionHeight());
    widthSlider_->setBounds(widthKnobArea);

    area.removeFromLeft(10);
    area.removeFromTop(10);

    juce::Rectangle<int> fundamentalComboBoxArea = area.removeFromTop(25);
    fundamentalComboBox->setBounds(fundamentalComboBoxArea);

    area.removeFromTop(2);

    juce::Rectangle<int> octaveComboBoxArea = area.removeFromTop(25);
    octaveComboBox->setBounds(octaveComboBoxArea);




//
//    juce::Rectangle<int> envArea = area.removeFromTop(area.getHeight() - getKnobSectionHeight());
//    envelope_->setBounds(envArea);
//
//    juce::Rectangle<int> knobs_area = area.removeFromTop(getKnobSectionHeight());
//    placeKnobsInArea(knobs_area, { attack_.get(), decay_.get(), sustain_.get(), release_.get() }, true);
//
//    envelope_->setSizeRatio(getSizeRatio());
//
//    static constexpr float kMagnifyingHeightRatio = 0.2f;
//    int magnify_height = envelope_->getHeight() * kMagnifyingHeightRatio;
//    drag_magnifying_glass_->setBounds(envelope_->getRight() - magnify_height, envelope_->getY(),
//        magnify_height, magnify_height);
//
//    envelope_->magnifyReset();

    SynthSection::resized();
}
