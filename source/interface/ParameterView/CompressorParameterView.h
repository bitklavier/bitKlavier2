//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_CompressorPARAMETERVIEW_H
#define BITKLAVIER2_CompressorPARAMETERVIEW_H
#include "CompressorProcessor.h"
#include "synth_section.h"

class CompressorParameterView : public SynthSection
{
public:
    CompressorParameterView (chowdsp::PluginState& pluginState, CompressorParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), eqparams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("eq");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintHeadingText (g);
        paintBorder (g);
        // paintChildrenBackgrounds (g);
    }

    CompressorParams& eqparams_;
    void resized() override;
};

#endif //BITKLAVIER2_CompressorPARAMETERVIEW_H