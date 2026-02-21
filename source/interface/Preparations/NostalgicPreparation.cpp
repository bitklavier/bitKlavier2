/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "NostalgicPreparation.h"
#include "BKitems/BKItem.h"
#include "NostalgicParametersView.h"
#include "synth_slider.h"

// Definition for the NostalgicPreparation constructor.  It takes three parameters: a pointer to
// a Nostalgic Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private NostalgicPreparation member proc with an initialization list.
NostalgicPreparation::NostalgicPreparation (
    juce::ValueTree v,
    OpenGlWrapper& open_gl,
    juce::AudioProcessorGraph::NodeID node,
    SynthGuiInterface* _synth_gui_interface) :
                                               PreparationSection (juce::String ("Nostalgic"),
                                                   v,
                                                   open_gl,
                                                   node,
                                                   *_synth_gui_interface->getUndoManager()
                                                   )

{
    item = std::make_unique<NostalgicItem>(); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent(), true); // Calls member function of SynthSection (parent class to PreparationSection)
    if (_open_gl.context.isAttached() && _open_gl.context.isActive())
    {
        _open_gl.context.executeOnGLThread ([this] (juce::OpenGLContext& context) {
            item->getImageComponent()->init (_open_gl);
        },
            false);
    }

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kNostalgic);
    juce::MemoryBlock data;

    width = state.getProperty(IDs::width);
    height = state.getProperty(IDs::height);
    // width = 245;
    // height = 125;
}

std::unique_ptr<SynthSection> NostalgicPreparation::getPrepPopup()
{
    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
        if (auto* proc = dynamic_cast<NostalgicProcessor*> (getProcessor()))
            return std::make_unique<NostalgicParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

    return nullptr;
}

void NostalgicPreparation::resized()
{
    PreparationSection::resized();
}

NostalgicPreparation::~NostalgicPreparation()
{
}

void NostalgicPreparation::paintBackground (juce::Graphics& g)
{
    for (auto* port : objects)
        port->redoImage();
    PreparationSection::paintBackground (g);
}
