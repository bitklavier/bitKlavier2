//
// Created by Joshua Warner on 6/27/24.
//


/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "TuningPreparation.h"
#include "BKitems/BKItem.h"

#include "synth_slider.h"
#include "ParametersView.h"
// Definition for the TuningPreparation constructor.  It takes three parameters: a pointer to
// a Tuning Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private TuningPreparation member proc with an initialization list.
TuningPreparation::TuningPreparation (std::unique_ptr<TuningProcessor> p,
                                    juce::ValueTree v, OpenGlWrapper& um) :
        PreparationSection(juce::String("tuning"), v, um,p.get()),
        proc(*p.get()),
        _proc_ptr(std::move(p))
{

    item = std::make_unique<TuningItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent()); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
         {item->getImageComponent()->init(_open_gl);
         },false);
    addAndMakeVisible (item.get());

    setSkinOverride (Skin::kTuning);

}

std::unique_ptr<SynthSection> TuningPreparation::getPrepPopup()
{

    return std::make_unique<bitklavier::ParametersView>(proc.getState(), proc.getState().params, proc.v.getProperty(IDs::uuid).toString(), open_gl);
}


void TuningPreparation::resized()
{
    PreparationSection::resized();
}

TuningPreparation::~TuningPreparation()
{
}

juce::AudioProcessor* TuningPreparation::getProcessor()
{
    return &proc;
}

std::unique_ptr<juce::AudioProcessor> TuningPreparation::getProcessorPtr()
{
    return std::move(_proc_ptr);
}


