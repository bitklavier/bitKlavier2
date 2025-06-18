//
// Created by Davis Polito on 6/24/24.
//

#include "KeymapPreparation.h"
#include "FullInterface.h"
#include "KeymapParameterView.h"
KeymapPreparation::KeymapPreparation (std::unique_ptr<KeymapProcessor> p,
                                      juce::ValueTree v, OpenGlWrapper& um) :
        PreparationSection(juce::String("keymap"), v, um,p.get()),
        proc(*p.get()),
        _proc_ptr(std::move(p))
{

    item = std::make_unique<KeymapItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent()); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
                                        {item->getImageComponent()->init(_open_gl); },false);
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
    return std::make_unique<KeymapParameterView>(proc, _open_gl);
}

juce::AudioProcessor* KeymapPreparation::getProcessor()
{
    return &proc;
}

std::unique_ptr<juce::AudioProcessor> KeymapPreparation::getProcessorPtr()
{
    return std::move(_proc_ptr);
}




