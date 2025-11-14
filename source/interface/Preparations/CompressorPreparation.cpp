//
// Created by Myra Norton on 11/14/25.
//

#include "CompressorPreparation.h"
#include "BKitems/BKItem.h"
#include "CompressorParameterView.h"

// Definition for the CompressorPreparation constructor.  It takes three parameters: a pointer to
// an CompressorProcessor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private CompressorPreparation member proc with an initialization list.
CompressorPreparation::CompressorPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("Compressor"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<CompressorItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
        item->getImageComponent()->init (_open_gl);
    },
        false);

    addAndMakeVisible (item.get());

    // todo: make UI for Compressor preps
    // setSkinOverride (Skin::kCompressor);
    juce::MemoryBlock data;

    // width = 245;
    // height = 125;
}

// Static function that returns a pointer to a CompressorPreparation object
std::unique_ptr<PreparationSection> CompressorPreparation::create (const juce::ValueTree& v, SynthGuiInterface* interface)
{
    return std::make_unique<CompressorPreparation> (v, interface->getGui()->open_gl_, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (v.getProperty (IDs::nodeID)), interface);
}

std::unique_ptr<SynthSection> CompressorPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<CompressorProcessor*> (getProcessor()))
            return std::make_unique<CompressorParameterView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void CompressorPreparation::resized()
{
    PreparationSection::resized();
}

CompressorPreparation::~CompressorPreparation()
{
}

void CompressorPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}