//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_CompressorPARAMETERVIEW_H
#define BITKLAVIER2_CompressorPARAMETERVIEW_H
#include "CompressorProcessor.h"
#include "OpenGL_LabeledBorder.h"
#include "default_look_and_feel.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "OpenGL_CompressorMeter.h"

class CompressorParameterView : public SynthSection, public juce::Timer
{
public:
    CompressorParameterView (chowdsp::PluginState& pluginState, CompressorParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), compressorParams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("compressor");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);
        setSkinOverride(Skin::kDirect);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // menus
        if (auto* compressorParams = dynamic_cast<CompressorParams*>(&params)) {
            presets_combo_box = std::make_unique<OpenGLComboBox>(compressorParams->presets->paramID.toStdString());
            presets_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*compressorParams->presets.get(), listeners, *presets_combo_box, nullptr);
            addAndMakeVisible(presets_combo_box.get());
            addOpenGlComponent(presets_combo_box->getImageComponent());
        }

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // Compressor Meter
        compressorMeter = std::make_unique<OpenGL_CompressorMeter> (&params, listeners);
        addOpenGlComponent (compressorMeter->getImageComponent());
        addAndMakeVisible(compressorMeter.get());

        // power EQ button
        activeCompressor_toggle = std::make_unique<SynthButton>(compressorParams_.activeCompressor->paramID);
        activeCompressor_attachment = std::make_unique<chowdsp::ButtonAttachment>(compressorParams_.activeCompressor, listeners, *activeCompressor_toggle, nullptr);
        activeCompressor_toggle->setComponentID(compressorParams_.activeCompressor->paramID);
        addSynthButton(activeCompressor_toggle.get(), true);
        activeCompressor_toggle->setText("power");

        // reset EQ button
        reset_button = std::make_unique<SynthButton>("reset");
        reset_button_attachment = std::make_unique<chowdsp::ButtonAttachment>(compressorParams_.resetCompressor,listeners,*reset_button,nullptr);
        reset_button->setComponentID("reset");
        addSynthButton(reset_button.get(), true);
        reset_button->setText("reset");

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Out");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        // sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSend, listeners, &params.sendLevels);
        // sendLevelMeter->setLabel("Send");
        // addSubSection(sendLevelMeter.get());

        // and for input level meter/slider
        inLevelMeter = std::make_unique<PeakMeterSection>(name, params.inputGain, listeners, &params.inputLevels);
        inLevelMeter->setLabel("In");
        addSubSection(inLevelMeter.get());

        // knobs
        attack_knob = std::make_unique<SynthSlider>(params.attack->paramID, params.attack->getModParam());
        addSlider(attack_knob.get());
        attack_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        attack_knob->setPopupPlacement(juce::BubbleComponent::below);
        attack_knob->setShowPopupOnHover(true);
        attack_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attack, listeners, *attack_knob, nullptr);
        attack_knob->addAttachment(attack_knob_attachment.get());

        release_knob = std::make_unique<SynthSlider>(params.release->paramID, params.release->getModParam());
        addSlider(release_knob.get());
        release_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        release_knob->setPopupPlacement(juce::BubbleComponent::below);
        release_knob->setShowPopupOnHover(true);
        release_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.release, listeners, *release_knob, nullptr);
        release_knob->addAttachment(release_knob_attachment.get());

        threshold_knob = std::make_unique<SynthSlider>(params.threshold->paramID, params.threshold->getModParam());
        addSlider(threshold_knob.get());
        threshold_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        threshold_knob->setPopupPlacement(juce::BubbleComponent::below);
        threshold_knob->setShowPopupOnHover(true);
        threshold_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.threshold, listeners, *threshold_knob, nullptr);
        threshold_knob->addAttachment(threshold_knob_attachment.get());

        makeup_knob = std::make_unique<SynthSlider>(params.makeup->paramID, params.makeup->getModParam());
        addSlider(makeup_knob.get());
        makeup_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        makeup_knob->setPopupPlacement(juce::BubbleComponent::below);
        makeup_knob->setShowPopupOnHover(true);
        makeup_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.makeup, listeners, *makeup_knob, nullptr);
        makeup_knob->addAttachment(makeup_knob_attachment.get());

        // mix_knob = std::make_unique<SynthSlider>(params.mix->paramID, params.mix->getModParam());
        // addSlider(mix_knob.get());
        // mix_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        // mix_knob->setPopupPlacement(juce::BubbleComponent::below);
        // mix_knob->setShowPopupOnHover(true);
        // mix_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.mix, listeners, *mix_knob, nullptr);
        // mix_knob->addAttachment(mix_knob_attachment.get());

        ratio_knob = std::make_unique<SynthSlider>(params.ratio->paramID, params.ratio->getModParam());
        addSlider(ratio_knob.get());
        ratio_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ratio_knob->setPopupPlacement(juce::BubbleComponent::below);
        ratio_knob->setShowPopupOnHover(true);
        ratio_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.ratio, listeners, *ratio_knob, nullptr);
        ratio_knob->addAttachment(ratio_knob_attachment.get());

        knee_knob = std::make_unique<SynthSlider>(params.knee->paramID, params.knee->getModParam());
        addSlider(knee_knob.get());
        knee_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knee_knob->setPopupPlacement(juce::BubbleComponent::below);
        knee_knob->setShowPopupOnHover(true);
        knee_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.knee, listeners, *knee_knob, nullptr);
        knee_knob->addAttachment(knee_knob_attachment.get());

        // knob labels
        attack_knob_label = std::make_shared<PlainTextComponent>(attack_knob->getName(), "ATTACK");
        addOpenGlComponent(attack_knob_label);
        attack_knob_label->setJustification(juce::Justification::centred);

        release_knob_label = std::make_shared<PlainTextComponent>(release_knob->getName(), "RELEASE");
        addOpenGlComponent(release_knob_label);
        release_knob_label->setJustification(juce::Justification::centred);

        threshold_knob_label = std::make_shared<PlainTextComponent>(threshold_knob->getName(), "THRESHOLD");
        addOpenGlComponent(threshold_knob_label);
        threshold_knob_label->setJustification(juce::Justification::centred);

        makeup_knob_label = std::make_shared<PlainTextComponent>(makeup_knob->getName(), "MAKEUP");
        addOpenGlComponent(makeup_knob_label);
        makeup_knob_label->setJustification(juce::Justification::centred);

        // mix_knob_label = std::make_shared<PlainTextComponent>(mix_knob->getName(), "MIX");
        // addOpenGlComponent(mix_knob_label);
        // mix_knob_label->setJustification(juce::Justification::centred);

        ratio_knob_label = std::make_shared<PlainTextComponent>(ratio_knob->getName(), "RATIO");
        addOpenGlComponent(ratio_knob_label);
        ratio_knob_label->setJustification(juce::Justification::centred);

        knee_knob_label = std::make_shared<PlainTextComponent>(knee_knob->getName(), "KNEE");
        addOpenGlComponent(knee_knob_label);
        knee_knob_label->setJustification(juce::Justification::centred);

        compressorControlsBorder = std::make_shared<OpenGL_LabeledBorder>("compressor controls border", "Compressor Parameters");
        addBorder(compressorControlsBorder.get());
        presetsBorder = std::make_shared<OpenGL_LabeledBorder>("presets border", "Power and Presets");
        addBorder(presetsBorder.get());

        // for updating the compressorMeter
        startTimer(50);
    }

    void timerCallback() override;

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);

        const auto bounds = compressorMeter->getBounds().toFloat();
        const float centreX = bounds.getX() + bounds.getWidth() * 0.5f;
        const float centreY = bounds.getY() + bounds.getHeight();
        const float needleLength = juce::jmin(bounds.getWidth() * 0.7f, bounds.getHeight() * 0.7f);
        g.setColour(compressorMeter->needle.needleColour);
        compressorMeter->needle.redrawNeedle (g, centreX, centreY, needleLength);
    }
    
    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

    // compressor meter
    std::shared_ptr<OpenGL_CompressorMeter> compressorMeter;

    // preset combo box menu
    std::unique_ptr<OpenGLComboBox> presets_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> presets_attachment;

    // active eq & reset buttons
    std::unique_ptr<SynthButton> activeCompressor_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> activeCompressor_attachment;
    std::unique_ptr<SynthButton> reset_button;
    std::unique_ptr<chowdsp::ButtonAttachment> reset_button_attachment;

    // level meters
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;
    std::shared_ptr<PeakMeterSection> inLevelMeter;

    // knobs
    std::unique_ptr<SynthSlider> attack_knob;
    std::unique_ptr<chowdsp::SliderAttachment> attack_knob_attachment;
    std::unique_ptr<SynthSlider> release_knob;
    std::unique_ptr<chowdsp::SliderAttachment> release_knob_attachment;
    std ::unique_ptr<SynthSlider> threshold_knob;
    std::unique_ptr<chowdsp::SliderAttachment> threshold_knob_attachment;
    std ::unique_ptr<SynthSlider> makeup_knob;
    std::unique_ptr<chowdsp::SliderAttachment> makeup_knob_attachment;
    // std::unique_ptr<SynthSlider> mix_knob;
    // std::unique_ptr<chowdsp::SliderAttachment> mix_knob_attachment;
    std ::unique_ptr<SynthSlider> ratio_knob;
    std::unique_ptr<chowdsp::SliderAttachment> ratio_knob_attachment;
    std ::unique_ptr<SynthSlider> knee_knob;
    std::unique_ptr<chowdsp::SliderAttachment> knee_knob_attachment;

    // knob labels
    std::shared_ptr<PlainTextComponent> attack_knob_label;
    std::shared_ptr<PlainTextComponent> release_knob_label;
    std::shared_ptr<PlainTextComponent> threshold_knob_label;
    std::shared_ptr<PlainTextComponent> makeup_knob_label;
    std::shared_ptr<PlainTextComponent> mix_knob_label;
    std::shared_ptr<PlainTextComponent> ratio_knob_label;
    std::shared_ptr<PlainTextComponent> knee_knob_label;

    // borders
    std::shared_ptr<OpenGL_LabeledBorder> compressorControlsBorder;
    std::shared_ptr<OpenGL_LabeledBorder> presetsBorder;

    CompressorParams& compressorParams_;
    void resized() override;
};

#endif //BITKLAVIER2_CompressorPARAMETERVIEW_H