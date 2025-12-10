//
// Created by Myra Norton on 11/27/25.
//

#pragma once

#include "EQProcessor.h"
#include "BKLookAndFeel.h"
#include "FullInterface.h"
#include "PreparationSection.h"

class BKEqualizerGraph  : public juce::Component
{
public:
    BKEqualizerGraph(EQProcessor &eqProcessor) : eq(eqProcessor){}
    ~BKEqualizerGraph() override {}

    class Listener
    {
    public:
        virtual ~Listener() {};
        // virtual void EQGraphValueChanged() = 0;
    };

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);   // clear the background

        // Divide up the space for the graph and labels
        juce::Rectangle<int> bounds(getLocalBounds());
        juce::Rectangle<int> graphArea(bounds);
        int b = 14; // space for text (b)elow the graph
        graphArea.removeFromBottom(b);
        int r = 15; // horizontal (r)eduction from the component width
        graphArea.reduce(r, 0);

        // position graph labels
        g.setColour(juce::Colours::white);
        double graphLeft = graphArea.getX();
        double graphRight = graphArea.getRight();
        auto labelMap = [graphLeft, graphRight](double freq) {
            return juce::jmap(juce::mapFromLog10(freq, 20.0, 20000.0), 0.0, 1.0, graphLeft, graphRight);
        };
        g.drawText("20", labelMap(20.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);
        g.drawText("20K", labelMap(20000.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);
        g.drawText("100", labelMap(100.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);
        g.drawText("500", labelMap(500.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);
        g.drawText("1K", labelMap(1000.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);
        g.drawText("5k", labelMap(5000.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);
        g.drawText("10K", labelMap(10000.0) - r, graphArea.getBottom(), 2 * r, b, juce::Justification::centred);

        // calculate magnitudes
        auto w = graphArea.getWidth();
        std::vector<double> mags;
        mags.resize(w);
        for (int i = 0; i < w; i++) {
            auto freq = juce::mapToLog10(double(i) / double(w), 20.0, 20000.0);
            double mag = eq.magForFreq(freq);

            mags[i] = juce::Decibels::gainToDecibels(mag);
        }

        // Calculate position of magnitudes on grpah
        juce::Path responseCurve;
        double outputMin = graphArea.getBottom() - 1;
        double outputMax = graphArea.getY();
        auto map = [outputMin, outputMax](double input) {
            double inputMin = -24.0;
            double inputMax = 24.0;

            if (input < inputMin) return outputMin;
            else if (input > inputMax) return outputMax;
            else return juce::jmap(input, inputMin, inputMax, outputMin, outputMax);
        };

        // Draw response curve
        responseCurve.startNewSubPath(graphArea.getX(), map(mags.front()));
        for (size_t i = 1; i < mags.size(); i++) {
            responseCurve.lineTo(graphArea.getX() + i, map(mags[i]));
        }
        g.setColour(juce::Colours::white);
        g.strokePath(responseCurve, juce::PathStrokeType(2.f));

        // draw an outline around the component
        g.setColour (juce::Colours::grey);
        g.drawRect (graphArea, 2);
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..
        repaint();
    }

    // hmmmm is there a thread safety issue here?
    // void setSampleRate(double sr) { eq.setSampleRate(sr); }
    // void updateEQ(EQProcessor& newEQ) { eq = newEQ; }

private:
    EQProcessor& eq;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKEqualizerGraph)
};
