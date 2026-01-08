//
// Created by Myra Norton on 11/14/25.
//

#include "EQPreparation.h"
#include "BKitems/BKItem.h"
#include "EQParameterView.h"

// Definition for the EQPreparation constructor.  It takes three parameters: a pointer to
// an EQProcessor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private EQPreparation member proc with an initialization list.
EQPreparation::EQPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("EQ"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<EQItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    },
        false);

    addAndMakeVisible (item.get());

    // todo: make UI for EQ preps
    // setSkinOverride (Skin::kEQ);
    juce::MemoryBlock data;

    // width = 245;
    // height = 125;
}

std::unique_ptr<SynthSection> EQPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<EQProcessor*> (getProcessor()))
            return std::make_unique<EQParameterView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void EQPreparation::resized()
{
    PreparationSection::resized();
}

EQPreparation::~EQPreparation()
{
}

void EQPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}