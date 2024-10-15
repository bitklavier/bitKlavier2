//
// Created by Davis Polito on 2/1/24.
//

#ifndef BITKLAVIER2_MAIN_SECTION_H
#define BITKLAVIER2_MAIN_SECTION_H
#include "synth_section.h"
#include "ConstructionSite.h"
class MainSection : public SynthSection
{
public:
    class Listener {
    public:
        virtual ~Listener() { }

        //virtual void showAboutSection() = 0;
    };

    MainSection(juce::ValueTree v, juce::UndoManager &um, OpenGlWrapper &open_gl, SynthGuiData * data);

    void paintBackground(Graphics& g) override;
    void resized() override;
//    void reset() override;
//
//    void buttonClicked(Button* clicked_button) override;
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
