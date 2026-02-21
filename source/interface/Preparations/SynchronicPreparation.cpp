//
// Created by Dan Trueman on 8/5/25.
//

#include "SynchronicPreparation.h"
#include "BKitems/BKItem.h"
#include "SynchronicParametersView.h"
#include "synth_slider.h"

SynchronicPreparation::SynchronicPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("synchronic"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<SynchronicItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    if (_open_gl.context.isAttached() && _open_gl.context.isActive())
    {
        _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
            item->getImageComponent()->init (_open_gl);
        },
            false);
    }

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kSynchronic);
    juce::MemoryBlock data;

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);
    // width = 245;
    // height = 125;
}

std::unique_ptr<SynthSection> SynchronicPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<SynchronicProcessor*> (getProcessor()))
            return std::make_unique<SynchronicParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void SynchronicPreparation::resized()
{
    PreparationSection::resized();
}

SynchronicPreparation::~SynchronicPreparation()
{
}

void SynchronicPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}