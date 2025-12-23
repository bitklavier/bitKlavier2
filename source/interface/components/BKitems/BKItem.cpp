//
// Created by Davis Polito on 4/22/24.
//

#include "BKItem.h"
#include "paths.h"
namespace
{
    // Returns the path for each preparation type
    juce::Array<juce::Path> getPathForPreparation (bitklavier::BKPreparationType type)
    {
        // Returns the paths for a keymap preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeKeymap)
            return Paths::keymapPaths();

        // Returns the paths for a direct preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeDirect)
            return Paths::directPaths();

        // Returns the paths for a nostalgic preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeNostalgic)
            return Paths::nostalgicPaths();

        // Returns the paths for a synchronic preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeSynchronic)
            return Paths::synchronicPaths();

        // Returns the paths for a blendronic preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeBlendronic)
            return Paths::blendronicPaths();

        // Returns the paths for a resonance preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeResonance)
            return Paths::resonancePaths();

        // Returns the paths for a tuning preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeTuning)
            return Paths::tuningPaths();

        // Returns the paths for a tempo preparation window
        if (type == bitklavier::BKPreparationType::PreparationTypeTempo)
            return Paths::tempoPaths();

        if (type == bitklavier::BKPreparationType::PreparationTypeModulation)
            return Paths::modulationPaths();

        if (type == bitklavier::BKPreparationType::PreparationTypeVST)
            return Paths::vstPaths();

//        if (type == bitklavier::BKPreparationType::PreparationTypeMidiFilter)
//            return Paths::midiFilterPaths();

        return Paths::tuningPaths();
    }
}

BKItem::BKItem (bitklavier::BKPreparationType type) : juce::Button("bkitem")
{
    image_component_ = std::make_shared<OpenGlImageComponent>();
    image_component_->setComponent(this);
    image_component_->setAlwaysOnTop(false);

    juce::Array<juce::Path> paths;
    paths = getPathForPreparation(type);
    layer_1_ = paths.getUnchecked(0);
    layer_2_ = paths.getUnchecked(1);
    if (paths.size() > 2)
        layer_3_ = paths.getUnchecked(2);
    // setInterceptsMouseClicks (false, false);
}


void BKItem::mouseDown(const juce::MouseEvent& e)
{
    getParentComponent()->mouseDown(e);
}

void BKItem::mouseDrag(const juce::MouseEvent& e)
{
    getParentComponent()->mouseDrag(e);
}

void BKItem::mouseDoubleClick(const juce::MouseEvent& e)
{juce::Logger::writeToLog ("bkitemdoubelclick");
    getParentComponent()->mouseDoubleClick(e);
}
void BKItem::mouseUp(const juce::MouseEvent &e) {
    getParentComponent()->mouseUp(e);
}
