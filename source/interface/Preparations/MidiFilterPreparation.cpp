//
// Created by Dan Trueman on 7/11/25.
//

#include "MidiFilterPreparation.h"
#include "BKitems/BKItem.h"
#include "MidiFilterParametersView.h"
#include "synth_button.h"
// #include "ParametersView.h"

MidiFilterPreparation::MidiFilterPreparation(
    juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* interface):
                                    PreparationSection(juce::String("midifilter"),  v, open_gl, node,*interface->getUndoManager()
                                        )

{
    item = std::make_unique<MidiFilterItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(),true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
        {item->getImageComponent()->init(_open_gl);
        },false);
    addAndMakeVisible (item.get());

//    setSkinOverride (Skin::kTuning);
}

std::unique_ptr<SynthSection> MidiFilterPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<MidiFilterProcessor*> (getProcessor()))
            return std::make_unique<MidiFilterParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}


void MidiFilterPreparation::resized()
{
    PreparationSection::resized();
}

MidiFilterPreparation::~MidiFilterPreparation()
{
}