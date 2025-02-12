//
// Created by Davis Polito on 6/24/24.
//

#include "KeymapPreparation.h"
#include "FullInterface.h"
KeymapPreparation::KeymapPreparation (std::unique_ptr<KeymapProcessor> p,
                                      juce::ValueTree v, OpenGlWrapper& um) :
        PreparationSection(juce::String("keymap"), v, um,p.get()),
        proc(*p.get()),
        _proc_ptr(std::move(p))
{

    item = std::make_unique<KeymapItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent()); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.initOpenGlComp.try_enqueue([this]
                                        {item->getImageComponent()->init(_open_gl); });
    addAndMakeVisible (item.get());

    setSkinOverride (Skin::kKeymap);

//    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
//    if (parent) {
//        juce::AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
//        if (device_manager)
//                    (*proc._midi.get(), proc._midi->enabledMidiInputs, *device_manager);
//            addAndMakeVisible(midi_selector_.get());
//            addOpenGlComponent(midi_selector_->getImageComponent());
//        }
//    }

}

KeymapPreparation::~KeymapPreparation()
{

}

std::unique_ptr<SynthSection> KeymapPreparation::getPrepPopup()
{
    return std::make_unique<KeymapPopup>(proc, _open_gl);
}

juce::AudioProcessor* KeymapPreparation::getProcessor()
{
    return &proc;
}

std::unique_ptr<juce::AudioProcessor> KeymapPreparation::getProcessorPtr()
{
    return std::move(_proc_ptr);
}



KeymapPopup::KeymapPopup(KeymapProcessor& _proc, OpenGlWrapper &open_gl):  proc(_proc), opengl(open_gl), SynthSection("keymap")
 {

//    auto &_params = proc.getState().params;

}
KeymapPopup::~KeymapPopup(){}
void KeymapPopup::resized()
{
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent && midi_selector_ == nullptr) {
        juce::AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
        if (device_manager) {
            midi_selector_ = std::make_unique<OpenGlMidiSelector>
                    (proc.v);
            addAndMakeVisible(midi_selector_.get());
            addOpenGlComponent(midi_selector_->getImageComponent());
            parent->getGui()->open_gl_.initOpenGlComp.try_enqueue([this]
                                               {SynthGuiInterface* parent = this->findParentComponentOfClass<SynthGuiInterface>();
                midi_selector_->getImageComponent()->init(parent->getGui()->open_gl_); });
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
}

void KeymapPopup::redoImage()
{
    int mult = getPixelMultiple();
    int image_width = getWidth() * mult;
    int image_height = getHeight() * mult;
    //juce::Image background_image(juce::Image::ARGB,image_width, image_height, true);
    //juce::Graphics g (background_image);

    //paintKnobShadows(g);
    //sliderShadows.setOwnImage(background_image);

}
