//
// Created by Dan Trueman on 10/8/25.
//

#ifndef BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
#define BITKLAVIER0_RESONANCEPARAMETERSVIEW_H

#include "ResonanceProcessor.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "synth_button.h"

class ResonanceParametersView : public SynthSection
{
public:
    ResonanceParametersView (chowdsp::PluginState& pluginState, ResonanceParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), sparams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("resonance");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();
    }

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;

    ResonanceParams& sparams_;
}

#endif //BITKLAVIER0_RESONANCEPARAMETERSVIEW_H
