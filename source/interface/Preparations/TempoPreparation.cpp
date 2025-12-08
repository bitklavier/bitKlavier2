//
// Created by Myra Norton on 8/20/25.
//

#include "TempoPreparation.h"
#include "BKitems/BKItem.h"
#include "TempoParametersView.h"
#include "synth_slider.h"

// Definition for the TempoPreparation constructor.  It takes three parameters: a pointer to
// a Tempo Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private TempoPreparation member proc with an initialization list.
TempoPreparation::TempoPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("tempo"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<TempoItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    },
        false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);
    juce::MemoryBlock data;

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);
    // width = 100;
    // height = 180;
}

std::unique_ptr<SynthSection> TempoPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<TempoProcessor*> (getProcessor()))
            return std::make_unique<TempoParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void TempoPreparation::resized()
{
    PreparationSection::resized();
}

TempoPreparation::~TempoPreparation()
{
}

void TempoPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}
