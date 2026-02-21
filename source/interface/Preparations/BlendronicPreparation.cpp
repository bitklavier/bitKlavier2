//
// Created by Dan Trueman on 7/26/25.
//

#include "BlendronicPreparation.h"
#include "BKitems/BKItem.h"
#include "BlendronicParametersView.h"
#include "synth_slider.h"


BlendronicPreparation::BlendronicPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("blendronic"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<BlendronicItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    if (_open_gl.context.isAttached() && _open_gl.context.isActive())
    {
        _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
            item->getImageComponent()->init (_open_gl);
        },
            false);
    }

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kBlendronic);
    juce::MemoryBlock data;

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);
    // width = 245;
    // height = 125;
}

std::unique_ptr<SynthSection> BlendronicPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<BlendronicProcessor*> (getProcessor()))
            return std::make_unique<BlendronicParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void BlendronicPreparation::resized()
{
    PreparationSection::resized();
}

BlendronicPreparation::~BlendronicPreparation()
{
}

void BlendronicPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}