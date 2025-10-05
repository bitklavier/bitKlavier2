/*
==============================================================================

VelocityCurveGraph.h
      Created: 8 Jun 2021 5:28:51pm
  Author:  Jeffrey Gordon

  ==============================================================================
*/

#ifndef BITKLAVIER0_VELOCITYCURVEGRAPH_H
#define BITKLAVIER0_VELOCITYCURVEGRAPH_H

#pragma once

#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include "utils.h"

//==============================================================================
/*
 This class merely implements the graph component of the velocity curving UI.
 Whenever a parameter changes, KeymapViewController updates the parameters here and
 then draws the appropriate graph.
*/
//==============================================================================

class VelocityCurveGraph  : public juce::Component
{
public:
    VelocityCurveGraph(KeymapParams& kparams) : params(kparams)
    {
    }

    ~VelocityCurveGraph() override
    {
    }

    // note: geometry here could be greatly simplified if the graph label could be done in KeymapViewcontroller. The geometry is kind of ridiculous.
    void paint (juce::Graphics& g) override
    {
        asym_k            = params.velocityCurve_asymWarp->getCurrentValue();
        sym_k             = params.velocityCurve_symWarp->getCurrentValue();
        scale             = params.velocityCurve_scale->getCurrentValue();
        offset            = params.velocityCurve_offset->getCurrentValue();
        velocityInvert    = params.velocityCurve_invert.get();

        DBG("asym_k = " + juce::String(asym_k));

        int leftPadding = 80; // space for "velocity out" label
        int rightPadding = 4; // space for the graph's right edge to fully display
        int topPadding = 6; // space for the dot and graph's top edge to fully display
        int bottomPadding = 40; // space for "velocity in" label
        int leftAdditional = 30; // additional space for number labels on left

        // don't bother if there's not enough room
        if (getWidth() <= leftPadding + rightPadding + 2 || getHeight() <= bottomPadding + topPadding + 2) return;

        // wrapper rectangles for labels
        juce::Rectangle<int> graphArea(
            getX() + leftPadding,
            getY() + topPadding,
            getWidth() - leftPadding - rightPadding,
            getHeight() - bottomPadding - topPadding);

        // graph background setup
        g.setColour (juce::Colours::grey);
        g.drawRect(graphArea);
        int graphHeight = graphArea.getHeight();
        int graphWidth = graphArea.getWidth();
        int graphX = graphArea.getX();
        int graphY = graphArea.getY();

        // dashed midlines
        juce::Line<float> hMidLine(graphX, graphY + topPadding + graphHeight / 2,
            graphWidth + graphX, graphY + topPadding + graphHeight / 2);
        juce::Line<float> vMidLine(graphWidth / 2 + graphX, graphY,
            graphWidth / 2 + graphX, graphY + graphHeight);
        const float dashLengths[] = {5, 3};
        g.drawDashedLine(hMidLine, dashLengths, 2);
        g.drawDashedLine(vMidLine, dashLengths, 2);

        //        juce::Rectangle<int> bottomLabel = graphArea.removeFromBottom(20);
        //        juce::Rectangle<int> leftLabel = graphArea.removeFromLeft(100);


        juce::Rectangle<int> bottomLabel(graphX - rightPadding, graphY + getHeight() - bottomPadding,
            graphWidth - leftPadding + rightPadding, bottomPadding);
        //        juce::Rectangle<int> leftLabel(getX(), topPadding, leftPadding - leftAdditional,
        //            getHeight() - bottomPadding - topPadding);
        juce::Rectangle<int> leftLabel(graphX, graphY, leftPadding, graphHeight);
        juce::Rectangle<int> leftNumbers(graphX - leftAdditional, graphY + topPadding - 5, leftAdditional,
            getHeight() - bottomPadding - topPadding + 10);
        juce::Rectangle<int> zeroLabel(graphX - leftPadding, graphY + graphHeight - bottomPadding, leftPadding, bottomPadding);

        // graph label setup
        //g.drawText("0", bottomLabel, juce::Justification::topLeft);
        g.drawText("0", zeroLabel, juce::Justification::topRight);
        g.drawText("0.5", bottomLabel, juce::Justification::centredTop);
        g.drawText("1", bottomLabel, juce::Justification::topRight);
        //g.drawText("0", leftNumbers, juce::Justification::bottomRight);
        g.drawText("0.5", leftNumbers, juce::Justification::centredRight);
        g.drawText("1", leftNumbers, juce::Justification::topRight);
        g.setColour(juce::Colours::white);
        g.drawText("Velocity In", bottomLabel, juce::Justification::centredBottom);

        // === START ROTATED BLOCK ===
        // Save the current, un-rotated graphics context state.
        g.saveState();

        // Calculate the center point of the rectangle where the text will be drawn.
        auto center = leftLabel.getCentre();

        // Apply the rotation (-90 degrees, or -pi/2 radians) using the center
        // of the target rectangle as the rotation pivot point.
        g.addTransform(juce::AffineTransform::rotation(-bitklavier::kPi / 2.0f, center.getX(), center.getY()));

        // Draw the text. Because we rotated the drawing context, the text will appear
        // rotated around its center point.
        g.drawFittedText("Velocity Out", leftLabel, juce::Justification::centred, 2);

        // Restore the graphics context. This removes the rotation transform entirely,
        // ensuring that any code following this block draws horizontally again.
        g.restoreState();
        // === END ROTATED BLOCK ===

        // plotter
        g.setColour(juce::Colours::red);
        juce::Path plot;

        // go pixel by pixel, adding each point to plot
        for (int i = 0; i <= graphWidth; i++) {
            juce::Point<float> toAdd (graphX + i,
                graphHeight + topPadding -
                    graphHeight * bitklavier::utils::dt_warpscale((float) i / graphWidth, asym_k, sym_k, scale, offset));

            if (toAdd.getY() < topPadding) toAdd.setY(topPadding);
            if (toAdd.getY() > graphHeight + topPadding) toAdd.setY(graphHeight + topPadding);

            if (velocityInvert) {
                toAdd.setY(graphHeight + 2 * topPadding - toAdd.getY());
            }

            // the first point starts the subpath, whereas all subsequent points are merely
            // added to the subpath.
            if (i == 0) plot.startNewSubPath(toAdd);
            else plot.lineTo(toAdd);
        }

        g.strokePath(plot, juce::PathStrokeType(2.0));

        // add a dot to represent input velocity
        g.setColour(juce::Colours::goldenrod);
        int radius = 12;
        for (std::pair<int, float> element : velocities)
        {
            float velocity = element.second;
            float warpscale = bitklavier::utils::dt_warpscale(velocity, asym_k, sym_k, scale, offset);
            if (warpscale > 1) warpscale = 1;
            if (warpscale < 0) warpscale = 0;

            if (velocityInvert) {
                g.fillEllipse(graphArea.getX() + velocity * graphWidth - radius / 2,
                    topPadding + graphHeight * warpscale - radius / 2,
                    radius, radius);
            }
            else {
                g.fillEllipse(graphArea.getX() + velocity * graphWidth - radius / 2,
                    topPadding + graphHeight - graphHeight * warpscale - radius / 2,
                    radius, radius);
            }
        }
    }

    void resized() override
    {
        // the graph should be re-drawn when the window resizes
        repaint();
    }

    void setAsym_k (float newAsym_k) { asym_k = newAsym_k; }
    void setSym_k (float newSym_k) { sym_k = newSym_k; }
    void setScale (float newScale) { scale = newScale; }
    void setOffset (float newOffset) { offset = newOffset; }
    void setVelocityInvert (bool newVelocityInvert) { velocityInvert = newVelocityInvert; }

    // Gotta do a copy here for thread safety, sorry Jeff!
    void updateVelocityList(std::map<int, float>& velocityList) { velocities = velocityList; }

private:

    // Various Parameters
    KeymapParams& params;

    float asym_k;
    float sym_k;
    float scale;
    float offset;
    bool velocityInvert;

    std::map<int, float> velocities;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VelocityCurveGraph)
};




#endif //BITKLAVIER0_VELOCITYCURVEGRAPH_H
