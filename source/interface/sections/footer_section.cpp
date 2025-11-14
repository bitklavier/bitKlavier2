//
// Created by Myra Norton on 11/14/25.
//

#include "footer_section.h"
#include "text_look_and_feel.h"
#include "synth_base.h"

FooterSection::FooterSection(SynthGuiData *data) : SynthSection("footer_section"),
                                                           body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)),
                                                            gallery(data->tree),
                                                            gainProcessor (*data->synth, data->tree),
                                                            busEqProcessor (*data->synth, data->tree),
                                                            busCompressorProcessor (*data->synth, data->tree)
{

    jassert(gallery.hasType(IDs::GALLERY));
    addOpenGlComponent(body_);

    data->synth->setGainProcessor(&gainProcessor);

    keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(keymap_);
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

    // setAlwaysOnTop(true);
    setSkinOverride(Skin::kHeader);
}


void FooterSection::paintBackground(juce::Graphics &g) {
    paintContainer(g);
    paintChildrenBackgrounds(g);
    g.setColour(findColour(Skin::kBody, true));
    int logo_section_width = 32.0 + getPadding();
    g.fillRect(0, 0, logo_section_width, getHeight());

    g.setColour(juce::Colours::white);
    g.fillRect(100, 50, 100, 100);
    paintKnobShadows(g);
}

void FooterSection::resized() {
    // body_->setBounds(getLocalBounds());
    // body_->setRounding(findValue(Skin::kBodyRounding));
    // body_->setColor(findColour(Skin::kBody, true));

    // the level meter and output gain slider
    // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
    auto& gainParams = gainProcessor.getState().params;
    auto& listeners  = gainProcessor.getState().getParameterListeners();
    levelMeter = std::make_unique<PeakMeterSection>(
        "BusGain",
        gainParams.outputGain,
        listeners,
        &gainParams.outputLevels,
        true
    );
    addSubSection(levelMeter.get());

    auto bounds = getLocalBounds();
    const int keySelectorWidth = 1000;
    const int buttonWidth      = 120;
    const int padding          = findValue(Skin::kLargePadding);

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
    // if (clicked_button == eqButton.get()) {
    //     showPrepPopup(std::move(EQPrep.getPrepPopup()),gallery,bitklavier::BKPreparationType::PreparationTypeEQ);
    // } else if (clicked_button == compressorButton.get()) {
    //     showPrepPopup(std::move(CompressorPrep.getPrepPopup()),gallery,bitklavier::BKPreparationType::PreparationTypeCompressor);
    // } else
    SynthSection::buttonClicked(clicked_button);
}

void FooterSection::sliderValueChanged(juce::Slider *slider) {
    SynthSection::sliderValueChanged(slider);
}