//
// Created by Dan Trueman on 6/28/25.
//

#include "SemitoneWidthSection.h"

SemitoneWidthSection::SemitoneWidthSection (
    juce::String name,
    SemitoneWidthParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent,
    chowdsp::PluginState& pluginState) : SynthSection(name)
{
    setComponentID(parent.getComponentID());

    widthSlider_ = std::make_unique<SynthSlider>(params.semitoneWidthSliderParam->paramID,params.semitoneWidthSliderParam->getModParam());
    addSlider(widthSlider_.get());
    widthSlider_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    widthSlider_->setPopupPlacement(juce::BubbleComponent::below);
    widthSlider_->setShowPopupOnHover(true);
    widthSliderAttachment = std::make_unique<chowdsp::SliderAttachment>(params.semitoneWidthSliderParam, listeners, *widthSlider_, pluginState.undoManager);
    widthSlider_->addAttachment(widthSliderAttachment.get());

    fundamentalComboBox = std::make_unique<OpenGLComboBox>(params.reffundamental->paramID.toStdString());
    fundamentalComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.reffundamental, listeners, *fundamentalComboBox, pluginState.undoManager);
    addComboBox(fundamentalComboBox.get(),true,true);

    octaveComboBox = std::make_unique<OpenGLComboBox>(params.octave->paramID.toStdString());
    octaveComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.octave, listeners, *octaveComboBox, pluginState.undoManager);
    addComboBox(octaveComboBox.get(),true,true);

    width_label = std::make_shared<PlainTextComponent>(widthSlider_->getName(), params.semitoneWidthSliderParam->getName(20));
    addOpenGlComponent(width_label);
    width_label->setTextSize (10.0f);
    width_label->setJustification(juce::Justification::centred);

    sectionBorder = std::make_shared<OpenGL_LabeledBorder>("semitonewidth", "Semitone Width");
    addBorder(sectionBorder.get());
}

SemitoneWidthSection::~SemitoneWidthSection() { }

void SemitoneWidthSection::paintBackground(juce::Graphics& g)
{
    setLabelFont(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
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
    sectionBorder->setBounds(area);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    area.reduce(largepadding, largepadding);

    juce::Rectangle<int> widthKnobArea = area.removeFromLeft(getKnobSectionHeight());
    widthSlider_->setBounds(widthKnobArea);
    int labelsectionheight = findValue(Skin::kLabelHeight);
    //juce::Rectangle<int> label_rect (widthSlider_->getX(), widthSlider_->getBottom() - 10, widthSlider_->getWidth(), labelsectionheight );
    juce::Rectangle<int> label_rect (widthSlider_->getX(), widthSlider_->getY() + widthSlider_->getHeight() / 2 + 20, widthSlider_->getWidth(), labelsectionheight );
    width_label->setBounds(label_rect);

    area.removeFromLeft(largepadding);
    area.removeFromTop(largepadding);

    int comboboxheight = findValue(Skin::kComboMenuHeight);

    // area.reduce(0, area.getHeight() - (comboboxheight + smallpadding));
    // juce::Rectangle<int> fundamentalComboBoxArea = area.removeFromTop(comboboxheight);
    // fundamentalComboBox->setBounds(fundamentalComboBoxArea);
    //
    // area.removeFromTop(smallpadding);
    //
    // juce::Rectangle<int> octaveComboBoxArea = area.removeFromTop(comboboxheight);
    // octaveComboBox->setBounds(octaveComboBoxArea);

    juce::FlexBox fb;

    // 1. Stack items vertically
    fb.flexDirection = juce::FlexBox::Direction::column;

    // 2. Center the stack vertically within the area
    fb.justifyContent = juce::FlexBox::JustifyContent::center;

    // 3. Stretch items to fill the full width of the 'area'
    fb.alignItems = juce::FlexBox::AlignItems::stretch;

    // 4. Add the items
    // Note: We don't need .withWidth() if alignItems is set to stretch
    fb.items.add(juce::FlexItem(*fundamentalComboBox)
                    .withHeight(comboboxheight));

    // Fixed spacer for padding
    fb.items.add(juce::FlexItem()
                    .withHeight(static_cast<float>(smallpadding)));

    fb.items.add(juce::FlexItem(*octaveComboBox)
                    .withHeight(comboboxheight));

    // 5. Execute layout
    // This will automatically use area.getWidth() for the items' width
    // because of the 'stretch' alignment.
    fb.performLayout(area.toFloat());

    SynthSection::resized();
}
