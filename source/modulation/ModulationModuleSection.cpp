//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationModuleSection.h"
#include "synth_gui_interface.h"
#include "ModulationSection.h"
#include "ModulatorBase.h"
#include "modulation_manager.h"
#include "synth_base.h"
#include "ModulationProcessor.h"
#include "tracktion_ValueTreeUtilities.h"
#include "ModulationList.h"
ModulationModuleSection::ModulationModuleSection(ModulationList* modulationProcessor,juce::ValueTree &v, ModulationManager *m, juce::UndoManager &um) :
ModulesInterface(v), modulation_list_(modulationProcessor), undo (um)
{
    container_->setComponentID(v.getProperty(IDs::uuid));
    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    scroll_bar_->setShrinkLeft(true);
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    scroll_bar_->addListener(this);
    modulation_sections_.reserve(modulation_list_->objects.size());


     for (auto& mod : modulation_list_->objects)
     {
         auto *module_section = new ModulationSection(mod->state, (mod->createEditor()), undo);
        container_->addSubSection(module_section);
        module_section->setInterceptsMouseClicks(false,true);
        modulation_sections_.emplace_back(std::move(module_section));
     }
     parent = modulation_list_->getValueTree();
     modulation_list_->addListener(this);
//    factory.registerType<OscillatorModuleProcessor, juce::ValueTree, LEAF*>("osc");
//    factory.registerType<FilterModuleProcessor, juce::ValueTree, LEAF*>("filt");
    addListener(m);
}
void ModulationModuleSection::modulatorAdded( ModulatorBase* obj)
{
//    auto obj = tracktion::engine::getObjectFor(*modulation_list_, v);
    auto *module_section = new ModulationSection(obj->state,obj->createEditor(), undo);
    {
        //TODO : make sure all the addSubsections are getting locked
        juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section);
    }
    module_section->setInterceptsMouseClicks(false,true);
    //watch out for this invalidating ptrs to the object in places such as
    //container->sub_sections
    // a vector resize could invalidate ptrs
    //could solve by rebuilidng the container sub section ptrs on add
    modulation_sections_.emplace_back(std::move(module_section));
    //parentHierarchyChanged();
    resized();
    for (auto listener: listeners_)
        listener->added();

}

void ModulationModuleSection::removeModulator (ModulatorBase* base)
{
    // find preparation section with the same id as the one we're removing
    int index = -1;
    if (modulation_sections_.empty())
        return;
    for (int i=0; i<modulation_sections_.size(); i++){
        if (modulation_sections_[i]->state == base->state){
            index = i;
            break;
        }
    }
    if (index == -1) jassertfalse;

    //

    //cleanup opengl
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        container_->removeSubSection (modulation_sections_[index].get());
    }

    //delete opengl
    modulation_sections_[index]->destroyOpenGlComponents (*findParentComponentOfClass<SynthGuiInterface>()->getOpenGlWrapper());
    //delete heap memory
    modulation_sections_.erase(modulation_sections_.begin()+index);
    for (auto listener: listeners_)
        listener->removed();
    DBG("moduleRemoved");
    resized();

}

ModulationModuleSection::~ModulationModuleSection()
{
   modulation_list_->removeListener(this);
}

void ModulationModuleSection::handlePopupResult(int result) {

    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1)
    {
        juce::ValueTree t(IDs::modulationproc);
        t.setProperty(IDs::type, "ramp", nullptr);
        t.setProperty(IDs::isState, false, nullptr);
        undo.beginNewTransaction();
        parent.appendChild(t,&undo);

    } else if (result == 2)
    {
        juce::ValueTree t(IDs::modulationproc);
        t.setProperty(IDs::type, "state", nullptr);
        t.setProperty(IDs::isState, true, nullptr);
        undo.beginNewTransaction();
        parent.appendChild(t,&undo);
    }
    else if (result == 3)
    {
        juce::ValueTree t(IDs::modulationproc);
        t.setProperty(IDs::type, "lfo", nullptr);
        t.setProperty(IDs::isState, true, nullptr);
        undo.beginNewTransaction();
        parent.appendChild(t,&undo);
    }
}


void ModulationModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    int padding = getPadding();
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int start_x = 0;
    int effect_width = getWidth() - start_x - large_padding;
    int knob_section_height = getKnobSectionHeight();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int effect_height = 2 * knob_section_height - widget_margin;
    int y = 0;

    juce::Point<int> position = viewport_.getViewPosition();
    for(auto& section : modulation_sections_)
    {
        section->setBounds(shadow_width, y, effect_width, effect_height);
        y += effect_height + padding;
    }

    container_->setBounds(0, 0, viewport_.getWidth(), y - padding);
    viewport_.setViewPosition(position);

    for (Listener* listener : listeners_)
        listener->effectsMoved();

    container_->setScrollWheelEnabled(container_->getHeight() <= viewport_.getHeight());
    setScrollBarRange();

    repaintBackground();
}
PopupItems ModulationModuleSection::createPopupMenu()
{
    PopupItems options;
    options.addItem(1, "add ramp" );
    options.addItem(2, "add state");
    options.addItem(3, "add lfo");
    return options;
}


std::map<std::string, SynthSlider*> ModulationModuleSection::getAllSliders()
{
    return container_->getAllSliders();
}

std::map<std::string, ModulationButton *> ModulationModuleSection::getAllModulationButtons() {
    return container_->getAllModulationButtons();
}


