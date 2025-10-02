//
// Created by Davis Polito on 5/27/25.
//
#include "KeymapParameterView.h"
#include "FullInterface.h"

KeymapParameterView::KeymapParameterView (
    KeymapProcessor& _proc,
    KeymapParams& kparams,
    juce::String name,
    OpenGlWrapper &open_gl) : proc(_proc), params(kparams), opengl(open_gl), SynthSection("")
 {
     setName("keymap");
     setLookAndFeel(DefaultLookAndFeel::instance());
     setComponentID(name);

     auto& listeners = proc.getState().getParameterListeners();;

     keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(params);
     addStateModulatedComponent(keyboard_component_.get());
     addAndMakeVisible(keyboard_component_.get());

     if (auto* kp = dynamic_cast<KeymapParams*>(&params))
     {
         selectDeselect_combobox = std::make_unique<OpenGLComboBox> (kp->selectChoice->paramID.toStdString());
         selectDeselect_attachment = std::make_unique<chowdsp::ComboBoxAttachment> (*kp->selectChoice.get(), listeners, *selectDeselect_combobox, nullptr);
         addComboBox (selectDeselect_combobox.get(), true, true);
     }
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
    int keyboardTop = keyboard_component_->getKeyboardTop();
    int editButtonRight = keyboard_component_->getEditAllTextButtonRight();
    int kcomboboxheight = keyboardTop - keyboard_component_->getY();

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

    selectDeselect_combobox->setBounds(area.getRight() - 150, keyboard_component_->getY(), 150, comboboxheight);
    //keyboard_component_->setBounds(kTitleWidth, 220, 600, 100);
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