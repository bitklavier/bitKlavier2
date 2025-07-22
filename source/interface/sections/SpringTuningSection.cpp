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
        slider->setShowPopupOnHover(true);
        slider->setName(param_->name); // will this break the mods?

        /*
         * put the intervalWeight knobs in their own vector, so we can group them in the UI
         *      - the rest of the knobs will go in the vector of more general controls
         */
        if(param_->paramID.startsWith("intervalWeight_"))
        {
            intervalWeightsSliders_sliderAttachments.emplace_back(std::move(attachment));
            intervalWeightSliders.emplace_back(std::move(slider));
        }
        else {
            floatAttachments.emplace_back(std::move(attachment));
            _sliders.emplace_back(std::move(slider));
        }
    }

    // create setup the tuning system combo boxes
    if (auto* tuningParams = dynamic_cast<SpringTuningParams*>(&params))
    {
        auto index = tuningParams->scaleId->getIndex();
        scaleId_ComboBox = std::make_unique<OpenGLComboBox>(params.scaleId->paramID.toStdString());
        scaleId_ComboBox_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.scaleId, listeners, *scaleId_ComboBox, nullptr);
        addAndMakeVisible(scaleId_ComboBox.get());
        addOpenGlComponent(scaleId_ComboBox->getImageComponent());
        setupTuningSystemMenu(scaleId_ComboBox);
        scaleId_ComboBox->setSelectedItemIndex(index,juce::sendNotificationSync);

        index = tuningParams->scaleId_tether->getIndex();
        scaleId_tether_ComboBox = std::make_unique<OpenGLComboBox>(params.scaleId_tether->paramID.toStdString());
        scaleId_tether_ComboBox_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.scaleId_tether, listeners, *scaleId_tether_ComboBox, nullptr);
        addAndMakeVisible(scaleId_tether_ComboBox.get());
        addOpenGlComponent(scaleId_tether_ComboBox->getImageComponent());
        setupTuningSystemMenu(scaleId_tether_ComboBox);
        scaleId_tether_ComboBox->setSelectedItemIndex(index,juce::sendNotificationSync);
    }

    // create and gather the useLocalOrFundamental toggle buttons
    for ( auto &param_ : *params.getBoolParams())
    {
        if(param_->paramID.startsWith("useLocalOrFundamental_")){
            auto button = std::make_unique<SynthButton>(param_->paramID);
//            SynthButton* currentButtonPtr = button.get(); // for use in the onStateChange lambda below
            auto button_ToggleAttachment = std::make_unique<chowdsp::ButtonAttachment>(param_,listeners, *button, nullptr);
            button->setComponentID(param_->paramID);
            addSynthButton(button.get(), true);
            button->setButtonText("F");
            button->setHelpText("use the Current Fundamental to determine default interval spring lengths?");

            /*
             * this all seems to work as expected, but the setButtonText never actually does its thing
             */
//            // Attach the lambda, capturing the raw pointer to the current button
//            button->onClick = [currentButtonPtr]()
//            {
//                // Now, currentState refers to the state of THIS specific button
//                // For a ToggleButton, getToggleState() is more relevant for 'on'/'off' status
//                bool isToggledOn = currentButtonPtr->getToggleState();
//                juce::Logger::writeToLog("Button " + currentButtonPtr->getComponentID() + " is toggled: " + juce::String((int)isToggledOn));
//
//                // Change text based on toggle state
//                if (isToggledOn)
//                {
//                    DBG("setting button text to On");
//                    currentButtonPtr->setButtonText("On");
//                }
//                else
//                {
//                    DBG("setting button text to Off");
//                    currentButtonPtr->setButtonText("Off");
//                }
//            };

            useLocalOrFundamentalToggles_sliderAttachments.emplace_back(std::move(button_ToggleAttachment));
            useLocalOrFundamentalToggles.emplace_back(std::move(button));
        }
    }

    // the tuning system fundamental combo boxes
    intervalFundamental_ComboBox = std::make_unique<OpenGLComboBox>(params.intervalFundamental->paramID.toStdString());
    intervalFundamental_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.intervalFundamental, listeners, *intervalFundamental_ComboBox, nullptr);
    addAndMakeVisible(intervalFundamental_ComboBox.get());
    addOpenGlComponent(intervalFundamental_ComboBox->getImageComponent());

    tetherFundamental_ComboBox = std::make_unique<OpenGLComboBox>(params.intervalFundamental->paramID.toStdString());
    tetherFundamental_ComboBoxAttachment = std::make_unique<chowdsp::ComboBoxAttachment>(params.tetherFundamental, listeners, *tetherFundamental_ComboBox, nullptr);
    addAndMakeVisible(tetherFundamental_ComboBox.get());
    addOpenGlComponent(tetherFundamental_ComboBox->getImageComponent());

    // labels...
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

    sectionBorder.setName("springtuning");
    sectionBorder.setText("Spring Tuning");
    sectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sectionBorder);
}

SpringTuningSection::~SpringTuningSection() {}

void SpringTuningSection::paintBackground(juce::Graphics& g) {

    setLabelFont(g);

    for (auto& slider : _sliders)
    {
        drawLabelForComponent (g, slider->getName(), slider.get());
    }

    for (auto& slider : intervalWeightSliders)
    {
        drawLabelForComponent (g, slider->getName(), slider.get());
    }

    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    sectionBorder.paint(g);
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

    juce::Rectangle<int> tuningSystemLabelsBox = area.removeFromTop(comboboxheight);
    intervalsLabel->setBounds(tuningSystemLabelsBox.removeFromLeft(tuningSystemLabelsBox.getWidth() * 0.5));
    anchorsLabel->setBounds(tuningSystemLabelsBox);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int> menusBox = area.removeFromTop(comboboxheight);
    juce::Rectangle<int> scaleIdMenuBox = menusBox.removeFromLeft(menusBox.getWidth() * 0.5);
    scaleIdMenuBox.reduce(20, 0);
    menusBox.reduce(20, 0);
    juce::Rectangle<int> intervalFundamental_ComboBoxBox = scaleIdMenuBox.removeFromRight(scaleIdMenuBox.getWidth() * 0.5);
    juce::Rectangle<int> scaleIdTetherMenuBox = menusBox.removeFromLeft(menusBox.getWidth() * 0.5);
    juce::Rectangle<int> tetherFundamental_ComboBoxBox = menusBox;

    scaleId_ComboBox->setBounds(scaleIdMenuBox);
    intervalFundamental_ComboBox->setBounds(intervalFundamental_ComboBoxBox);
    scaleId_tether_ComboBox->setBounds(scaleIdTetherMenuBox);
    tetherFundamental_ComboBox->setBounds(tetherFundamental_ComboBoxBox);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int> knobsBox = area.removeFromTop(knobsectionheight);
    placeKnobsInArea(knobsBox, _sliders, false);

    area.removeFromTop(smallpadding);

    juce::Rectangle<int> intervalknobsBox = area.removeFromTop(knobsectionheight);
    placeKnobsInArea(intervalknobsBox, intervalWeightSliders);
    juce::Rectangle<int> fundamentalToggles = area.removeFromTop(comboboxheight);
    placeButtonsInArea(fundamentalToggles, useLocalOrFundamentalToggles);

    area.removeFromTop(smallpadding);

    currentFundamental->setBounds(area.removeFromTop(labelsectionheight));

    SynthSection::resized();
}