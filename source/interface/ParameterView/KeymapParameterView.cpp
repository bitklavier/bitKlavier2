//
// Created by Davis Polito on 5/27/25.
//
#include "KeymapParameterView.h"
#include "FullInterface.h""
KeymapParameterView::KeymapParameterView(KeymapProcessor& _proc, OpenGlWrapper &open_gl):  proc(_proc), opengl(open_gl), SynthSection("keymap")
 {

//    auto &_params = proc.getState().params;
    keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(_proc.getState().params);
    addStateModulatedComponent(keyboard_component_.get());
    addAndMakeVisible(keyboard_component_.get());

}
KeymapParameterView::~KeymapParameterView(){}
void KeymapParameterView::resized()
{
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
        midi_selector_->setBounds(10, 10, 200, 200);
        midi_selector_->redoImage();
        midi_selector_->setRowHeight(22);
        juce::Colour background = findColour(Skin::kPopupBackground, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::backgroundColourId, background);
        //setColorRecursively(midi_selector_.get(), juce::ComboBox::backgroundColourId, background);
        //setColorRecursively(midi_selector_.get(), juce::PopupMenu::backgroundColourId, background);
        //setColorRecursively(midi_selector_.get(), juce::BubbleComponent::backgroundColourId, background);

        juce::Colour text = findColour(Skin::kBodyText, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ComboBox::textColourId, text);

        //setColorRecursively(midi_selector_.get(), juce::TextEditor::highlightColourId, juce::Colours::transparentBlack);
        setColorRecursively(midi_selector_.get(), juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        //setColorRecursively(midi_selector_.get(), juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    }
    keyboard_component_->setBounds(10, 220, 600, 100);

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