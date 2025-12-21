//
// Created by Joshua Warner on 6/27/24.
//


/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "TuningPreparation.h"
#include "BKitems/BKItem.h"
#include "TuningParametersView.h"

// #include "synth_slider.h"
// #include "ParametersView.h"
// Definition for the TuningPreparation constructor.  It takes three parameters: a pointer to
// a Tuning Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private TuningPreparation member proc with an initialization list.
TuningPreparation::TuningPreparation( juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* interface):
        PreparationSection(juce::String("tuning"),  v, open_gl, node,*interface->getUndoManager())

{

    item = std::make_unique<TuningItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(),true); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
         {item->getImageComponent()->init(_open_gl);
         },false);
    addAndMakeVisible (item.get());

    setSkinOverride (Skin::kTuning);

}

std::unique_ptr<SynthSection> TuningPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<TuningProcessor*> (getProcessor()))
            return std::make_unique<TuningParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}


void TuningPreparation::resized()
{
    PreparationSection::resized();
}

TuningPreparation::~TuningPreparation()
{
}

