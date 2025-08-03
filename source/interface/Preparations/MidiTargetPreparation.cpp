//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetPreparation.h"
#include "MidiTargetParametersView.h"
#include "BKitems/BKItem.h"
#include "synth_button.h"
#include "ParametersView.h"

MidiTargetPreparation::MidiTargetPreparation(
    juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* interface):
                                    PreparationSection(juce::String("miditarget"),  v, open_gl, node,*interface->getUndoManager()
                                    )

{
    item = std::make_unique<MidiTargetItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(),true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
        {item->getImageComponent()->init(_open_gl);
        },false);

    addAndMakeVisible (item.get());
}

std::unique_ptr<SynthSection> MidiTargetPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<MidiTargetProcessor*> (getProcessor()))
            return std::make_unique<MidiTargetParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}


void MidiTargetPreparation::resized()
{
    PreparationSection::resized();
}

MidiTargetPreparation::~MidiTargetPreparation()
{
}