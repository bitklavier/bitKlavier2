//
// Created by Dan Trueman on 7/22/25.
//

#include "PianoSwitchPreparation.h"
#include "BKitems/BKItem.h"
#include "synth_button.h"
#include "ParametersView.h"

PianoSwitchPreparation::PianoSwitchPreparation(
    juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* interface):
                                    PreparationSection(
                                        juce::String("pianoswitch"),
                                        v,
                                        open_gl,
                                        node,
                                        *interface->getUndoManager()
                                    )
{
    // Initializes member variable `item` of PreparationSection class
    item = std::make_unique<PianoSwitchItem> ();

    // Calls member function of SynthSection (parent class to PreparationSection)
    addOpenGlComponent (item->getImageComponent(),true);

    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
        {item->getImageComponent()->init(_open_gl);
        },false);

    addAndMakeVisible (item.get());
}

std::unique_ptr<SynthSection> PianoSwitchPreparation::getPrepPopup()
{
//    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
//        if (auto* proc = dynamic_cast<PianoSwitchPreparation*> (getProcessor()))
//            return std::make_unique<MidiFilterParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

/**
 * no popup for piano switch!
 */
    return nullptr;
}

void PianoSwitchPreparation::resized()
{
    PreparationSection::resized();
}

