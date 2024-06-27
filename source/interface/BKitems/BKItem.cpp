//
// Created by Davis Polito on 4/22/24.
//

#include "BKItem.h"
#include "paths.h"
namespace
{
    // Returns the path for each preparation type
    Array<Path> getPathForPreparation (bitklavier::BKPreparationType type)
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
    }
}

BKItem::BKItem (bitklavier::BKPreparationType type) : Button("bkitem")
{


    image_component_.setComponent(this);


    Array<Path> paths;
    paths = getPathForPreparation(type);
    layer_1_ = paths.getUnchecked(0);
    layer_2_ = paths.getUnchecked(1);
    layer_3_ = paths.getUnchecked(2);
    layer_4_ = paths.getUnchecked(3);

}


void BKItem::mouseDown(const MouseEvent& e)
{
    getParentComponent()->mouseDown(e);
}

void BKItem::mouseDrag(const MouseEvent& e)
{
    getParentComponent()->mouseDrag(e);
}

void BKItem::mouseDoubleClick(const MouseEvent& e)
{

}