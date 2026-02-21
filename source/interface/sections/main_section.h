//
// Created by Davis Polito on 2/1/24.
//

#ifndef BITKLAVIER2_MAIN_SECTION_H
#define BITKLAVIER2_MAIN_SECTION_H
#include "synth_section.h"
class ConstructionSite;
class MainSection : public SynthSection
{
public:


    MainSection(const juce::ValueTree &v, juce::UndoManager &um, OpenGlWrapper &open_gl, SynthGuiData * data, juce::ApplicationCommandManager& _manager);
    ~MainSection();
    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void reset() override;
//    void reset() override;
//
//    void buttonClicked(juce::Button* clicked_button) override;
//
//    void notifyChange();
//    void notifyFresh();
//    juce::ValueTree v;
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    std::unique_ptr<ConstructionSite> constructionSite_;
    void removeAllGuiListeners();
    juce::UndoManager &um; // ok to be public?

private:
    juce::ApplicationCommandManager& commandManager;
    juce::Viewport constructionPort;

    std::vector<Listener*> listeners_;

};

#endif //BITKLAVIER2_MAIN_SECTION_H
