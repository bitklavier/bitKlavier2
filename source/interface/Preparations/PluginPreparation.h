/************************************************************************************/
/*                 Created by Davis Polito and Joshua Warner                        */
/************************************************************************************/

#ifndef BITKLAVIER2_PluginPREPARATION_H
#define BITKLAVIER2_PluginPREPARATION_H


#include "PreparationSection.h"

#include "FullInterface.h"

#include "UserPreferences.h"
#include "synth_base.h"
/************************************************************************************/
/*             CLASS: PluginPreparation, inherits from PreparationSection           */
/************************************************************************************/

class PluginPreparation : public PreparationSection
                            {
public:

    // Constructor method that takes three arguments: a smart pointer to a PolygonalOscProcessor,
    // a value tree, and a reference to an OpenGlWrapper object
    PluginPreparation(const juce::ValueTree& v, OpenGlWrapper &open_gl, juce::AudioProcessorGraph::NodeID node,  SynthGuiInterface*);

    // Destructor method
    ~PluginPreparation();

    // Static function that returns a pointer to a PluginPreparation object
    static std::unique_ptr<PreparationSection> create(const juce::ValueTree& v, SynthGuiInterface* interface) {

        return std::make_unique<PluginPreparation> (v, interface->getGui()->open_gl_,juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::nodeID)),interface);
    }

    // Public function definitions for the PluginPreparation class, which override functions
    // in the PreparationSection base class
    void addSoundSet(std::map<juce::String, juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>>* s) override
    {
    }

    void addSoundSet(
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* s,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* h,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* r,
        juce::ReferenceCountedArray<BKSamplerSound<juce::AudioFormatReader>>* p) override
    {
    }

    std::unique_ptr<SynthSection> getPrepPopup() override;
    void resized() override;
    void paintBackground(juce::Graphics &g);


    void mouseDoubleClick(const juce::MouseEvent &event) override {
      for (auto list: listeners_) {
          auto interface = findParentComponentOfClass<SynthGuiInterface>();
          list->createWindow(interface->getSynth()->getNodeForId(pluginID),PluginWindow::Type::normal);
      }
  }




};



#endif // BITKLAVIER2_PluginPREPARATION_H