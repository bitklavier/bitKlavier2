/*********************************************************************************************/
/*                           Created by Davis Polito and Joshua Warner                       */
/*********************************************************************************************/

#include "DirectPreparation.h"
#include "BKitems/BKItem.h"
#include "synth_slider.h"

// Definition for the DirectPreparation constructor.  It takes three parameters: a pointer to
// a Direct Processor p, a juce::ValueTree v, and a reference to an OpenGlWrapper object.  Initializes
// the base class members and private DirectPreparation member proc with an initialization list.
DirectPreparation::DirectPreparation (std::unique_ptr<DirectProcessor> p,
    juce::ValueTree v, OpenGlWrapper& um) :
                         PreparationSection(juce::String("direct"), v, um,p.get()),
                         proc(*p.get()),
                         _proc_ptr(std::move(p))
{

    item = std::make_unique<DirectItem> (); // Initializes member variable `item` of PreparationSection class
    addOpenGlComponent (item->getImageComponent()); // Calls member function of SynthSection (parent class to PreparationSection)
    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
        {item->getImageComponent()->init(_open_gl);
        },false);

    addAndMakeVisible (item.get());
    setSkinOverride (Skin::kDirect);
    juce::MemoryBlock data;

}

std::unique_ptr<SynthSection> DirectPreparation::getPrepPopup()
{
//    if(popup_view) {
//        popup_view->destroyOpenGlComponents(_open_gl);
//        popup_view->reset();
//        return popup_view;
//    }
//    popup_view = std::make_shared<DirectParametersView>(proc.getState(), proc.getState().params,proc.v.getProperty(IDs::uuid).toString(), open_gl);
//
//    popup_view->initOpenGlComponents(_open_gl);
    return std::make_unique<DirectParametersView>(proc.getState(), proc.getState().params,proc.v.getProperty(IDs::uuid).toString(), open_gl);
}


void DirectPreparation::resized()
{
    PreparationSection::resized();
}

DirectPreparation::~DirectPreparation()
{
}

juce::AudioProcessor* DirectPreparation::getProcessor()
{
    return &proc;
}

std::unique_ptr<juce::AudioProcessor> DirectPreparation::getProcessorPtr()
{
    return std::move(_proc_ptr);
}


void DirectPreparation::paintBackground(juce::Graphics &g)  {
    for (auto * port: objects)
        port->redoImage();
    PreparationSection::paintBackground(g);
}


