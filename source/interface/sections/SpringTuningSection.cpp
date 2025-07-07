//
// Created by Dan Trueman on 7/7/25.
//

#include "SpringTuningSection.h"

SpringTuningSection::SpringTuningSection (
    juce::String name,
    SpringTuningParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent) : SynthSection(name)
{
    setComponentID(parent.getComponentID());
    setName("springtuning");
    setLookAndFeel(DefaultLookAndFeel::instance());

    for ( auto &param_ : *params.getFloatParams())
    {
        auto slider = std::make_unique<SynthSlider>(param_->paramID);
        auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
        addSlider(slider.get());
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        floatAttachments.emplace_back(std::move(attachment));
        _sliders.emplace_back(std::move(slider));
    }

//
//    if (auto* tuningParams = dynamic_cast<AdaptiveTuningParams*>(&params)) {
//        auto index = tuningParams->tAdaptiveIntervalScale->getIndex();
//        adaptiveIntervalScale_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveIntervalScale->paramID.toStdString());
//        adaptiveIntervalScale_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveIntervalScale, listeners, *adaptiveIntervalScale_ComboBox, nullptr);
//        addAndMakeVisible(adaptiveIntervalScale_ComboBox.get());
//        addOpenGlComponent(adaptiveIntervalScale_ComboBox->getImageComponent());
//        setupTuningSystemMenu(adaptiveIntervalScale_ComboBox);
//        adaptiveIntervalScale_ComboBox->setSelectedItemIndex(index,juce::sendNotificationSync);
//
//        index = tuningParams->tAdaptiveAnchorScale->getIndex();
//        adaptiveAnchorScale_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveAnchorScale->paramID.toStdString());
//        adaptiveAnchorScale_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveAnchorScale, listeners, *adaptiveAnchorScale_ComboBox, nullptr);
//        addAndMakeVisible(adaptiveAnchorScale_ComboBox.get());
//        addOpenGlComponent(adaptiveAnchorScale_ComboBox->getImageComponent());
//        setupTuningSystemMenu(adaptiveAnchorScale_ComboBox);
//        adaptiveAnchorScale_ComboBox->setSelectedItemIndex(index,juce::sendNotificationSync);
//    }
//
//    adaptiveAnchorFundamental_ComboBox = std::make_unique<OpenGLComboBox>(params.tAdaptiveAnchorFundamental->paramID.toStdString());
//    adaptiveAnchorFundamental_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tAdaptiveAnchorFundamental, listeners, *adaptiveAnchorFundamental_ComboBox, nullptr);
//    addAndMakeVisible(adaptiveAnchorFundamental_ComboBox.get());
//    addOpenGlComponent(adaptiveAnchorFundamental_ComboBox->getImageComponent());


    currentFundamental = std::make_shared<PlainTextComponent>("currentfundamental", "Current Fundamental = C");
    addOpenGlComponent(currentFundamental);
    currentFundamental->setTextSize (12.0f);
    currentFundamental->setJustification(juce::Justification::left);

    sectionBorder.setName("adaptivetuning");
    sectionBorder.setText("Adaptive Tuning");
    sectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sectionBorder);
}

SpringTuningSection::~SpringTuningSection() { }

void SpringTuningSection::paintBackground(juce::Graphics& g) {
//    setLabelFont(g);
//    drawLabelForComponent(g, TRANS("Cluster Threshold (ms)"), clusterThreshold_Slider.get());
//    drawLabelForComponent(g, TRANS("History (notes)"), history_Slider.get());
//
//    paintKnobShadows(g);
//    paintChildrenBackgrounds(g);
//
//    sectionBorder.paint(g);
}

void SpringTuningSection::resized() {

    juce::Rectangle<int> area (getLocalBounds());
    sectionBorder.setBounds(area);

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int labelsectionheight = findValue(Skin::kLabelHeight);

    area.reduce(largepadding, largepadding);

//
//    juce::Rectangle<int> tuningSystemLabelsBox = area.removeFromTop(comboboxheight);
////    intervalsLabel->setBounds(tuningSystemLabelsBox.removeFromLeft(tuningSystemLabelsBox.getWidth() * 0.5));
////    anchorsLabel->setBounds(tuningSystemLabelsBox);
//
//    area.removeFromTop(smallpadding);
//
//    juce::Rectangle<int>comboBoxArea = area.removeFromTop(comboboxheight);
//    juce::Rectangle<int>intervalComboBoxArea = comboBoxArea.removeFromLeft(comboBoxArea.getWidth() * 0.5);
//    adaptiveIntervalScale_ComboBox->setBounds(intervalComboBoxArea.removeFromLeft(intervalComboBoxArea.getWidth() * 0.5));
//    useInversionOfIntervalScale_Toggle->setBounds(intervalComboBoxArea);
//    adaptiveAnchorScale_ComboBox->setBounds(comboBoxArea.removeFromLeft(comboBoxArea.getWidth() * 0.5));
//    adaptiveAnchorFundamental_ComboBox->setBounds(comboBoxArea);
//
//    area.removeFromTop(smallpadding);
//
//    juce::Rectangle<int> knobsBox = area.removeFromTop(knobsectionheight + largepadding);
//    //    knobsBox.reduce(knobsBox.getWidth() * 0.5, 0);
//    //    placeKnobsInArea(knobsBox, sliderVec, false);
//    clusterThreshold_Slider->setBounds(knobsBox.removeFromLeft(knobsBox.getWidth() * 0.5));
//    history_Slider->setBounds(knobsBox);
//
//    area.removeFromTop(smallpadding);
//
//    juce::Rectangle<int> resetBox = area.removeFromTop(comboboxheight);
//    resetButton->setBounds(resetBox.removeFromLeft(resetBox.getWidth() * 0.5));
//    currentFundamental->setBounds(resetBox);

    SynthSection::resized();
}