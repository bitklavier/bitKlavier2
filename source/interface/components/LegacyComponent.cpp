//
// Created by Davis Polito on 2/25/25.
//

#include "LegacyComponent.h"
#include "synth_section.h"
//LegacyComponentWrapper::LegacyComponentWrapper(std::unique_ptr<SynthSection> &&legacy_component) :
//legacy_component_(std::move(legacy_component)){
//
//}
void LegacyComponentWrapper::mouseEnter(const juce::MouseEvent &e) {
//    OpenGlToggleButton::mouseEnter(e);
//legacy_component_->mouseEnter(e);
    for (auto* listener : listeners_)
        listener->hoverStarted(this);
    hovering_ = true;
}
void LegacyComponentWrapper::mouseExit(const juce::MouseEvent &e) {
//    OpenGlToggl
//    OeButton::mouseExit(e);
//legacy_component_->mouseExit(e);
    for (auto* listener : listeners_)
        listener->hoverEnded(this);
    hovering_ = false;
}