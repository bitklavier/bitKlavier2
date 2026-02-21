//
// Created by Dan Trueman on 10/8/25.
//

#include "ResonancePreparation.h"
#include "ResonanceParametersView.h"
#include "BKitems/BKItem.h"
#include "synth_slider.h"

ResonancePreparation::ResonancePreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("resonance"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                               )

{
    item = std::make_unique<ResonanceItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    if (_open_gl.context.isAttached() && _open_gl.context.isActive())
    {
        _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
            item->getImageComponent()->init (_open_gl);
        },
            false);
    }

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kResonance);
    juce::MemoryBlock data;

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);
    // width = 245;
    // height = 125;
}

std::unique_ptr<SynthSection> ResonancePreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<ResonanceProcessor*> (getProcessor()))
            return std::make_unique<ResonanceParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void ResonancePreparation::resized()
{
    PreparationSection::resized();
}

ResonancePreparation::~ResonancePreparation()
{
}

void ResonancePreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}