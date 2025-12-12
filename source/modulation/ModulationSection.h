//
// Created by Davis Polito on 11/19/24.
//

#ifndef BITKLAVIER_MODULATIONSECTION_H
#define BITKLAVIER_MODULATIONSECTION_H

#include "OpenGL_LabeledBorder.h"
#include "modulation_button.h"
#include "synth_section.h"
#include "synth_slider.h"

#include <juce_gui_basics/juce_gui_basics.h>
class ModulationButton;
class ModulationManager;
class ModulationSection : public SynthSection
{
public:
    ModulationSection( const juce::ValueTree &, SynthSection* editor, juce::UndoManager &um);
     ~ModulationSection() override;
    void resized() override;
    juce::ValueTree state;
    void addModButtonListener(ModulationManager*);
    void buttonClicked(juce::Button* button) override;
    void setVisible(bool visible) override
    {
        modBorder->setVisible (visible);
        if (visible) SynthSection::setVisible(visible);

        //SynthSection::reset();
        // for (auto& og : open_gl_components_)
        // {
        //     og->setVisible (visible);
        //     og->repaint();
        //     og->repaintBackground();
        // }
        // for (auto& cp : all_sliders_)
        // {
        //     DBG("redoing slider image");
        //     cp.second->setVisible (visible);
        //     cp.second->resized();
        // }
        // for (auto& mb : modulation_buttons_)
        // {
        //     mb.second->setVisible (visible);
        // }
        // for (auto& ss : sub_sections_)
        // {
        //     ss->setVisible (visible);
        //     ss->repaint();
        //     ss->repaintBackground();
        // }
        //
        // repaint();
        // repaintBackground();
    }
    void paintBackground(juce::Graphics& g) override
    {
        //if (isVisible())
        {
            paintContainer(g);
            paintKnobShadows(g);
            paintChildrenBackgrounds(g);
            paintBorder(g);
        }
    };

private:
    std::unique_ptr<OpenGlShapeButton> exit_button_;
    std::unique_ptr<SynthSection> _view;
    std::shared_ptr<ModulationButton> mod_button;
    juce::UndoManager& undo;

    std::shared_ptr<OpenGL_LabeledBorder> modBorder;
};

#endif //BITKLAVIER_MODULATIONSECTION_H
