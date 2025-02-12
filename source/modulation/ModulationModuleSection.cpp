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
ModulationModuleSection::ModulationModuleSection(ModulationList* modulationProcessor,juce::ValueTree &v, ModulationManager *m) :
ModulesInterface(v), modulation_list_(modulationProcessor)
{
    container_->setComponentID(v.getProperty(IDs::uuid));
    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    scroll_bar_->setShrinkLeft(true);
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    scroll_bar_->addListener(this);
     for (auto& mod : modulation_list_->objects)
     {
         auto *module_section = new ModulationSection(v, (mod->createEditor()));
        container_->addSubSection(module_section);
        module_section->setInterceptsMouseClicks(false,true);
        objects.add(module_section);
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
    auto *module_section = new ModulationSection(obj->state,obj->createEditor());
    container_->addSubSection(module_section);
    module_section->setInterceptsMouseClicks(false,true);
    objects.add(module_section);
    parentHierarchyChanged();
    for (auto listener: listeners_)
        listener->added();
    resized();
}
ModulationModuleSection::~ModulationModuleSection()
{
//    freeObjects();
}

void ModulationModuleSection::handlePopupResult(int result) {

    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1 )
    {
        juce::ValueTree t(IDs::modulationproc);
        t.setProperty(IDs::type, "ramp", nullptr);
        parent.appendChild(t,nullptr);
    } else if (result == 2)
    {
//        juce::ValueTree t(IDs::modulationproc);
//        t.setProperty(IDs::type, "filt", nullptr);
//        parent.appendChild(t,nullptr);
    }
    //    if (result == kArmMidiLearn)
    //        synth->armMidiLearn(getName().toStdString());
    //    else if (result == kClearMidiLearn)
    //        synth->clearMidiLearn(getName().toStdString());
    //    else if (result == kDefaultValue)
    //        setValue(getDoubleClickReturnValue());
    //    else if (result == kManualEntry)
    //        showTextEntry();
    //    else if (result == kClearModulations) {
    //        for (vital::ModulationConnection* connection : connections) {
    //            std::string source = connection->source_name;
    //            synth_interface_->disconnectModulation(connection);
    //        }
    //        notifyModulationsChanged();
    //    }
    //    else if (result >= kModulationList) {
    //        int connection_index = result - kModulationList;
    //        std::string source = connections[connection_index]->source_name;
    //        synth_interface_->disconnectModulation(connections[connection_index]);
    //        notifyModulationsChanged();
    //    }
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
    for(auto& section : objects)
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
//    options.addItem(2, "add filt");
    return options;
}


std::map<std::string, SynthSlider*> ModulationModuleSection::getAllSliders()
{
    return container_->getAllSliders();
}

std::map<std::string, ModulationButton *> ModulationModuleSection::getAllModulationButtons() {
    return container_->getAllModulationButtons();
}