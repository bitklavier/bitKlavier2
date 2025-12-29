//
// Created by Myra Norton on 11/14/25.
//

#include "footer_section.h"
#include "GainProcessor.h"
#include "sound_engine.h"
#include "synth_base.h"
#include "text_look_and_feel.h"
FooterSection::FooterSection(SynthGuiData *data) : SynthSection("footer_section"),
                                                           body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)),
                                                            gallery(data->tree)
{

    jassert(gallery.hasType(IDs::GALLERY));
    body_->setBounds(getLocalBounds());
    addOpenGlComponent(body_);

    keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(keymap_, false);
    addStateModulatedComponent(keyboard_component_.get());
    addAndMakeVisible(keyboard_component_.get());

    eqButton = std::make_unique<OpenGlTextButton>("footer_eq");
    addOpenGlComponent(eqButton->getGlComponent());
    addAndMakeVisible(eqButton.get());
    eqButton->addListener(this);
    eqButton->setLookAndFeel(TextLookAndFeel::instance());
    eqButton->setButtonText("EQ");

    compressorButton = std::make_unique<OpenGlTextButton>("footer_compressor");
    addOpenGlComponent(compressorButton->getGlComponent());
    addAndMakeVisible(compressorButton.get());
    compressorButton->addListener(this);
    compressorButton->setLookAndFeel(TextLookAndFeel::instance());
    compressorButton->setButtonText("Compressor");

    auto gainProcessor = data->synth->getEngine()->getMainVolumeProcessor();
    auto& gainParams = gainProcessor->getState().params;
    auto& listeners  = gainProcessor->getState().getParameterListeners();
    levelMeter = std::make_unique<PeakMeterSection>(
        "BusGain",
        gainParams.outputGain,
        listeners,
        &gainParams.outputLevels,
        true
    );
    addSubSection(levelMeter.get());
    // setAlwaysOnTop(true);
    setSkinOverride(Skin::kHeader);

    keyboard_component_->addMyListener(this);
}


void FooterSection::paintBackground(juce::Graphics &g) {
    paintContainer(g);
    g.setColour(juce::Colours::black);
    g.fillRect(getLocalBounds());
    paintChildrenBackgrounds(g);
    g.setColour(findColour(Skin::kBody, true));
    // int logo_section_width = 32.0 + getPadding();
    // g.fillRect(0, 0, logo_section_width, getHeight());
    // paintKnobShadows(g);
}

void FooterSection::resized() {
    auto bounds = getLocalBounds();
    const int keySelectorWidth = 1000;
    const int buttonWidth      = 120;
    const int padding          = findValue(Skin::kLargePadding);

    // body_->setRounding(findValue(Skin::kBodyRounding));
    // body_->setColor(juce::Colours::black);

    auto keyboardBounds = bounds.removeFromLeft(keySelectorWidth);
    keyboard_component_->setBounds(keyboardBounds);

    bounds.removeFromLeft(padding);
    auto postFX = bounds.removeFromLeft(buttonWidth);
    postFX.removeFromTop (padding);
    postFX.removeFromBottom (padding);
    auto halfHeight = postFX.getHeight() / 2;

    auto eqArea = postFX.removeFromTop(halfHeight);
    eqButton->setBounds(eqArea.withY(halfHeight/2).withHeight (findValue(Skin::kTextButtonHeight)*1.5));
    compressorButton->setBounds(postFX.withY((halfHeight/2)*3).withHeight (findValue(Skin::kTextButtonHeight)*1.5));

    bounds.removeFromLeft (padding*2);
    bounds.removeFromRight (padding*2);
    int meterHeight = 50;
    int y = bounds.getY() + (bounds.getHeight() - meterHeight) / 2 + 5;
    levelMeter->setBounds(bounds.getX(), y, bounds.getWidth(), meterHeight);
    SynthSection::resized();
}

void FooterSection::reset() {
    //  if (preset_selector_)
    //    //synth_preset_selector_->resetText();
}


void FooterSection::buttonClicked(juce::Button *clicked_button) {
    DBG("FooterSection::buttonClicked");
    if (clicked_button == compressorButton.get()) {
        auto interface = findParentComponentOfClass<SynthGuiInterface>();
        showPrepPopup(interface->getCompressorPopup(),gallery,bitklavier::BKPreparationType::PreparationTypeCompressor);
    }
    else if (clicked_button == eqButton.get()) {
        auto interface = findParentComponentOfClass<SynthGuiInterface>();
        showPrepPopup(interface->getEQPopup(),gallery,bitklavier::BKPreparationType::PreparationTypeEQ);
    }
    SynthSection::buttonClicked(clicked_button);
}

void FooterSection::sliderValueChanged(juce::Slider *slider) {
    SynthSection::sliderValueChanged(slider);
}