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
     addListener(m);

    // Initialize toggle button and processor state from ValueTree
    const bool vtToggle = (bool) parent.getProperty(IDs::modulationToggleMode, false);
    if (setToggleMode != nullptr)
        setToggleMode->setToggleState(vtToggle, juce::NotificationType::dontSendNotification);
}

void ModulationModuleSection::modulatorAdded( ModulatorBase* obj)
{
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

/*
 * todo: don't we also have to remove from the modulators_ vector in ModulationProcessor?
 *          - got a crash with a NULL modulator_ in ModulationProcessor when triggering after deleting a mod
 */
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
   if (modulation_list_ != nullptr)
       modulation_list_->removeListener(this);
}

void ModulationModuleSection::listAboutToBeDeleted(ModulationList* list)
{
    if (modulation_list_ == list && modulation_list_ != nullptr)
    {
        modulation_list_->removeListener(this);
        modulation_list_ = nullptr;
    }
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

    }
    else if (result == 2)
    {
        juce::ValueTree t(IDs::modulationproc);
        t.setProperty(IDs::type, "lfo", nullptr);
        t.setProperty(IDs::isState, false, nullptr);
        undo.beginNewTransaction();
        parent.appendChild(t,&undo);
    }
    else if (result == 3)
    {
        juce::ValueTree t(IDs::modulationproc);
        t.setProperty(IDs::type, "state", nullptr);
        t.setProperty(IDs::isState, true, nullptr);
        undo.beginNewTransaction();
        parent.appendChild(t,&undo);
    }
}

void ModulationModuleSection::buttonClicked(juce::Button* clicked_button)
{
    // Intercept toggle-mode button to propagate state to the underlying processor
    if (clicked_button == setToggleMode.get())
    {
        if (modulation_list_ != nullptr && modulation_list_->proc_ != nullptr)
        {
            const bool toggled = setToggleMode->getToggleState();
            // Persist to ValueTree so it is saved/loaded with galleries
            parent.setProperty(IDs::modulationToggleMode, toggled, &undo);
            modulation_list_->proc_->isToggle = toggled;
            DBG("ModulationModuleSection: set processor isToggle = " << (int)toggled);
        }
        return; // don't pass to base; we've handled it
    }

    // Defer other buttons to base implementation
    ModulesInterface::buttonClicked(clicked_button);
}

void ModulationModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    int padding = getPadding();
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int start_x = 0;
    int effect_width = getWidth() - start_x - large_padding * 1.5 - 2;
    int knob_section_height = getKnobSectionHeight();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int effect_height = 2 * knob_section_height - widget_margin;
    //int y = large_padding * 4; // make space for add modulation button and toggle mode button
    int y = (findValue(Skin::kComboMenuHeight) + 2 * padding) * 2;

    for(auto& section : modulation_sections_)
    {
        int effect_height_temp = effect_height;
        if (section->state.getProperty(IDs::isState))
        {
            // state mods don't need as much space since they don't have a knob
            effect_height_temp *= 0.5;
        }
        section->setBounds(shadow_width, y, effect_width, effect_height_temp);
        y += effect_height_temp + padding;
    }

    container_->setBounds(0, 0, viewport_.getWidth(), y - padding);

    //update scroll position so we can see the most recently added mod
    juce::Point<int> position(0, y);
    viewport_.setViewPosition(position);

    for (Listener* listener : listeners_)
        listener->effectsMoved();

    setScrollBarRange();

    repaintBackground();
}

PopupItems ModulationModuleSection::createPopupMenu()
{
    PopupItems options;
    options.addItem(1, "value (smoothed)" );
    options.addItem(2, "oscillator (lfo)");
    options.addItem(3, "state");
    return options;
}

std::map<std::string, SynthSlider*> ModulationModuleSection::getAllSliders()
{
    return container_->getAllSliders();
}

std::map<std::string, ModulationButton *> ModulationModuleSection::getAllModulationButtons() {
    return container_->getAllModulationButtons();
}


