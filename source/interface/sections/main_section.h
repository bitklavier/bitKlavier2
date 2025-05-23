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


    MainSection(juce::ValueTree v, juce::UndoManager &um, OpenGlWrapper &open_gl, SynthGuiData * data);
    ~MainSection();
    void paintBackground(juce::Graphics& g) override;
    void resized() override;
//    void reset() override;
//
//    void buttonClicked(juce::Button* clicked_button) override;
//
//    void notifyChange();
//    void notifyFresh();
//    juce::ValueTree v;
    void addListener(Listener* listener) { listeners_.push_back(listener); }
private:

    juce::UndoManager &um;
    juce::Viewport constructionPort;
    std::unique_ptr<ConstructionSite> constructionSite_;
    std::vector<Listener*> listeners_;

};

#endif //BITKLAVIER2_MAIN_SECTION_H
