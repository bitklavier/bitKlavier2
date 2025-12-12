//
// Created by Davis Polito on 11/19/24.
//

#ifndef BITKLAVIER_MODULATIONSECTION_H
#define BITKLAVIER_MODULATIONSECTION_H


#include "synth_section.h"
#include "ParameterView/ParametersView.h"
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
    void paintBackground(juce::Graphics& g) override
    {
        paintContainer(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
        paintBorder(g);
    };

private:
    std::unique_ptr<OpenGlShapeButton> exit_button_;
    std::unique_ptr<SynthSection> _view;
    std::shared_ptr<ModulationButton> mod_button;
    juce::UndoManager& undo;

    std::shared_ptr<OpenGL_LabeledBorder> modBorder;
};

#endif //BITKLAVIER_MODULATIONSECTION_H
