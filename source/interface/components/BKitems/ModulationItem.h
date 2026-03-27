//
// Created by Davis Polito on 1/30/25.
//

#ifndef BITKLAVIER2_MODULATIONITEM_H
#define BITKLAVIER2_MODULATIONITEM_H
#include "BKItem.h"
class ModulationItem : public BKItem
{
public:
    ModulationItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeModulation)
    {
    }


    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        // Retrieves and sets the color of each layer
//

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary2, true);
        g.setColour(c);
        g.fillPath(layer_1_);
        g.setColour(selected_ ? juce::Colours::white : juce::Colours::black);
        g.strokePath(layer_1_, juce::PathStrokeType (1.0f));
    }
    bool hitTest(int x, int y) override
    {
        layer_1_.contains(x,y );
    }
};
#endif //BITKLAVIER2_MODULATIONITEM_H
