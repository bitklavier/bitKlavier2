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
    PluginPreparation(std::unique_ptr<juce::AudioPluginInstance> proc, juce::ValueTree v, OpenGlWrapper& um);

    // Destructor method
    ~PluginPreparation();

    // Static function that returns a pointer to a PluginPreparation object
    static PreparationSection* createPluginSection(juce::ValueTree v, SynthGuiInterface* interface) {

            juce::PluginDescription desc;
            desc.loadFromXml(*v.getChildWithName(IDs::PLUGIN).createXml());
           juce::String err;
           auto instance = interface->userPreferences->userPreferences->formatManager.createPluginInstance (desc,
                                              interface->getSynth()->getSampleRate(),
                                              interface->getSynth()->getBufferSize(),err
                                              );
        if (instance == nullptr)
        {
            auto options = juce::MessageBoxOptions::makeOptionsOk (juce::MessageBoxIconType::WarningIcon,
                                                             TRANS ("Couldn't create plugin"),
                                                             err);
            interface->messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
              return nullptr;
        }
        return new PluginPreparation(std::move(instance), v, interface->getGui()->open_gl_);
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
    juce::AudioProcessor* getProcessor() override;
    std::unique_ptr<juce::AudioProcessor> getProcessorPtr() override;

    void mouseDoubleClick(const juce::MouseEvent &event) override {
      for (auto list: listeners_) {
          list->createWindow(node,PluginWindow::Type::normal);
      }
  }



private:
    // Private member variable for the PluginPreparation class: proc is a pointer to a
    // PluginProcessor Object
    juce::AudioPluginInstance & proc;
    std::unique_ptr<juce::AudioPluginInstance> _proc_ptr;


};



#endif // BITKLAVIER2_PluginPREPARATION_H