//
// Created by Davis Polito on 5/27/25.
//
#include "KeymapParameterView.h"
#include "FullInterface.h"

KeymapParameterView::KeymapParameterView (
    KeymapProcessor& _proc,
    KeymapParams& kparams,
    juce::String name,
    OpenGlWrapper *open_gl) : proc(_proc), params(kparams), SynthSection("")
 {
     setName("keymap");
     setLookAndFeel(DefaultLookAndFeel::instance());
     setComponentID(name);

     auto& listeners = proc.getState().getParameterListeners();

     keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(params);
     addStateModulatedComponent(keyboard_component_.get());
     addAndMakeVisible(keyboard_component_.get());

     // knobs
     asymmetricalWarp_knob = std::make_unique<SynthSlider>(params.velocityCurve_asymWarp->paramID);
     addSlider(asymmetricalWarp_knob.get());
     asymmetricalWarp_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     asymmetricalWarp_knob->setPopupPlacement(juce::BubbleComponent::below);
     asymmetricalWarp_knob->setShowPopupOnHover(true);
     asymmetricalWarp_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_asymWarp, listeners, *asymmetricalWarp_knob, nullptr);
     asymmetricalWarp_knob->addAttachment(asymmetricalWarp_knob_attach.get());

     symmetricalWarp_knob = std::make_unique<SynthSlider>(params.velocityCurve_symWarp->paramID);
     addSlider(symmetricalWarp_knob.get());
     symmetricalWarp_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     symmetricalWarp_knob->setPopupPlacement(juce::BubbleComponent::below);
     symmetricalWarp_knob->setShowPopupOnHover(true);
     symmetricalWarp_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_symWarp, listeners, *symmetricalWarp_knob, nullptr);
     symmetricalWarp_knob->addAttachment(symmetricalWarp_knob_attach.get());

     scale_knob = std::make_unique<SynthSlider>(params.velocityCurve_scale->paramID);
     addSlider(scale_knob.get());
     scale_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     scale_knob->setPopupPlacement(juce::BubbleComponent::below);
     scale_knob->setShowPopupOnHover(true);
     scale_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_scale, listeners, *scale_knob, nullptr);
     scale_knob->addAttachment(scale_knob_attach.get());

     offset_knob = std::make_unique<SynthSlider>(params.velocityCurve_offset->paramID);
     addSlider(offset_knob.get());
     offset_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     offset_knob->setPopupPlacement(juce::BubbleComponent::below);
     offset_knob->setShowPopupOnHover(true);
     offset_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_offset, listeners, *offset_knob, nullptr);
     offset_knob->addAttachment(offset_knob_attach.get());

     invert = std::make_unique<SynthButton>(params.velocityCurve_invert->paramID);
     invert_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.velocityCurve_invert, listeners, *invert, nullptr);
     invert->setComponentID(params.velocityCurve_invert->paramID);
     addSynthButton(invert.get(), true);
     invert->setText("invert?");

     //velocityCurveGraph.updateVelocityList(km->getVelocities());
//     velocityCurveGraph.setAsym_k(1.);
//     velocityCurveGraph.setSym_k(1.);
//     velocityCurveGraph.setScale(1.);
//     velocityCurveGraph.setOffset(0.);
//     velocityCurveGraph.setVelocityInvert(false);
//     addAndMakeVisible(velocityCurveGraph);
//
}

KeymapParameterView::~KeymapParameterView(){}

void KeymapParameterView::resized()
{
    const auto kTitleWidth = findValue(Skin::kTitleWidth);
    juce::Rectangle<int> area (getLocalBounds());

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int labelsectionheight = findValue(Skin::kLabelHeight);

    area.removeFromLeft(kTitleWidth);
    area.removeFromRight(kTitleWidth);

    juce::Rectangle keySelectorRect = area.removeFromBottom(150);
    keyboard_component_->setBounds(keySelectorRect);

    area.removeFromBottom(smallpadding);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent && midi_selector_ == nullptr) {
        juce::AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
        if (device_manager) {
            midi_selector_ = std::make_unique<OpenGlMidiSelector>
                    (proc.v);
            addAndMakeVisible(midi_selector_.get());
            addOpenGlComponent(midi_selector_->getImageComponent());
            parent->getGui()->open_gl_.context.executeOnGLThread([this](juce::OpenGLContext& context)
                                               {
                SynthGuiInterface* parent = this->findParentComponentOfClass<SynthGuiInterface>();
                midi_selector_->getImageComponent()->init(parent->getGui()->open_gl_); },false);
        }
    }

    if (midi_selector_) {
        juce::Rectangle midiSelectorRect = area.removeFromLeft(200);
        midiSelectorRect.removeFromTop(20);
        midi_selector_->setBounds(midiSelectorRect);
        //midi_selector_->setBounds(kTitleWidth, 10, 200, 200);
        midi_selector_->redoImage();
        midi_selector_->setRowHeight(22);

        juce::Colour background = findColour(Skin::kPopupBackground, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::backgroundColourId, background);

        juce::Colour text = findColour(Skin::kBodyText, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ComboBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    }

    // velocity knobs and invert button
    juce::Rectangle velocityKnobsRect = area.removeFromLeft(area.getWidth() * 0.5);
    velocityKnobsRect.reduce(velocityKnobsRect.getWidth() * 0.25, velocityKnobsRect.getHeight() * 0.25);
    juce::Rectangle invertButtonRect = velocityKnobsRect.removeFromBottom(comboboxheight);
    invertButtonRect.reduce(40, 0);
    invert->setBounds(invertButtonRect);

    velocityKnobsRect.removeFromBottom(20);
    juce::Rectangle velocityKnobsRect_top = velocityKnobsRect.removeFromTop(velocityKnobsRect.getHeight() * 0.5);
    asymmetricalWarp_knob->setBounds(velocityKnobsRect_top.removeFromLeft(velocityKnobsRect_top.getWidth() * 0.5));
    symmetricalWarp_knob->setBounds(velocityKnobsRect_top);
    scale_knob->setBounds(velocityKnobsRect.removeFromLeft(velocityKnobsRect.getWidth() * 0.5));
    offset_knob->setBounds(velocityKnobsRect);



}

void KeymapParameterView::redoImage()
{
    int mult = getPixelMultiple();
    int image_width = getWidth() * mult;
    int image_height = getHeight() * mult;
    //juce::Image background_image(juce::Image::ARGB,image_width, image_height, true);
    //juce::Graphics g (background_image);

    //paintKnobShadows(g);
    //sliderShadows.setOwnImage(background_image);

}