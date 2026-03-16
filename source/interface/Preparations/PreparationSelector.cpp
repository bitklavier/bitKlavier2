//
// Created by Davis Polito on 6/26/24.
//

#include "PreparationSelector.h"
#include "ConstructionSite.h"
PreparationSelector::PreparationSelector (const ConstructionSite& csite) : csite(csite)
{
}

void PreparationSelector::findLassoItemsInArea (juce::Array<PreparationSection*>& results, const juce::Rectangle<int>& area)
{
    const auto checkAndAddEditor = [&results, area] (PreparationSection* editor)
    {
        // Get the editor's bounds in the coordinate system of the ConstructionSite
        auto bounds = editor->getBoundsInParent().getSmallestIntegerContainer();
        if (area.intersects (bounds))
            results.add (editor);
    };

    for (auto &editor : csite.plugin_components)
        checkAndAddEditor (editor.get());
//    checkAndAddEditor (csite.getInputProcessorEditor());
//    checkAndAddEditor (csite.getOutputProcessorEditor());
}
