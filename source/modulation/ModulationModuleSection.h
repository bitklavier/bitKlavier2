//
// Created by Davis Polito on 11/19/24.
//

#ifndef BITKLAVIER_SOUNDMODULESECTION_H
#define BITKLAVIER_SOUNDMODULESECTION_H
#include "ModuleListInterface.h"
class ModulationSection;
class ModulatorBase;
class ModulationManager;
namespace bitklavier {
    class ModulationProcessor;
}
class ModulationModuleSection : public ModulesInterface
{

public:
    explicit ModulationModuleSection(ModulationList *,juce::ValueTree &, ModulationManager* m);
    virtual ~ModulationModuleSection();

    void setEffectPositions() override;

    void modulatorAdded( ModulatorBase* ) override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;
//    juce::Array<ModulationSection*> objects;
    juce::ValueTree parent;
    std::map<std::string, SynthSlider*> getAllSliders() override;
    ModulationList* modulation_list_;
    std::map<std::string, ModulationButton *> getAllModulationButtons() override;
};

#endif //BITKLAVIER_SOUNDMODULESECTION_H
