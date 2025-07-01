//
// Created by Davis Polito on 6/24/24.
//

#include "KeymapPreparation.h"
#include "FullInterface.h"
#include "KeymapParameterView.h"
KeymapPreparation::KeymapPreparation (const juce::ValueTree& v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID node,  SynthGuiInterface*) :
        PreparationSection(juce::String("keymap"), v, open_gl,node)

{

    item = std::make_unique<KeymapItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(),true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
                                        {item->getImageComponent()->init(_open_gl); },false);
    addAndMakeVisible (item.get());

    setSkinOverride (Skin::kKeymap);
    state.setProperty(IDs::width, 185, nullptr);
    state.setProperty(IDs::height, 105, nullptr);


}

KeymapPreparation::~KeymapPreparation()
{

}

std::unique_ptr<SynthSection> KeymapPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto *proc = dynamic_cast<KeymapProcessor*>(getProcessor()))
            return std::make_unique<KeymapParameterView>(*proc, *parent->getOpenGlWrapper());

    return nullptr;
}


