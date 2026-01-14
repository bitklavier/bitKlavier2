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
    explicit ModulationModuleSection(ModulationList *,juce::ValueTree &, ModulationManager* m, juce::UndoManager &um);
    virtual ~ModulationModuleSection();

    // ModulationList::Listener override: notified before the list is destroyed
    void listAboutToBeDeleted(ModulationList* list) override;

    void setEffectPositions() override;

    void modulatorAdded( ModulatorBase* ) override;
    void removeModulator( ModulatorBase* ) override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    juce::ValueTree parent;
    std::map<std::string, SynthSlider*> getAllSliders() override;
    ModulationList* modulation_list_;
    std::map<std::string, ModulationButton *> getAllModulationButtons() override;
    juce::UndoManager& undo;
};

#endif //BITKLAVIER_SOUNDMODULESECTION_H
