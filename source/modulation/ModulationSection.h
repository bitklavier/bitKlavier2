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
    ModulationSection( const juce::ValueTree &, SynthSection* editor);

     ~ModulationSection() override;

    void resized() override;
    juce::ValueTree state;
    void addModButtonListener(ModulationManager*);
private:

    std::unique_ptr<SynthSection> _view;
    std::shared_ptr<ModulationButton> mod_button;

};

#endif //BITKLAVIER_MODULATIONSECTION_H
