//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#define BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#include "DirectProcessor.h"
#include "OpenGL_VelocityMinMaxSlider.h"
#include "TranspositionSliderSection.h"
#include "VelocityMinMaxParams.h"
#include "envelope_section.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"

class DirectParametersView : public SynthSection
{
public:
    DirectParametersView (chowdsp::PluginState& pluginState, DirectParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection ("")
    {
        // the name that will appear in the UI as the name of the section
        setName ("direct");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // go through and get all the main float params (gain, hammer, etc...), make sliders for them
        // all the params for this prep are defined in struct DirectParams, in DirectProcessor.h
        // we're only including the ones that we want to group together and call "placeKnobsInArea" on
        // we're leaving out "outputGain" since that has its own VolumeSlider
        for (auto& param_ : *params.getFloatParams())
        {
            if ( // make group of params to display together
                param_->paramID == "Main" ||
                param_->paramID == "Hammers" ||
                param_->paramID == "Resonance" ||
                param_->paramID == "Pedal" ||
                param_->paramID == "Send")
            {
                auto slider = std::make_unique<SynthSlider> (param_->paramID);
                auto attachment = std::make_unique<chowdsp::SliderAttachment> (*param_.get(), listeners, *slider.get(), nullptr);
                addSlider (slider.get()); // adds the slider to the synthSection
                slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                floatAttachments.emplace_back (std::move (attachment));
                _sliders.emplace_back (std::move (slider));
            }
        }

        // create the more complex UI elements
        /**
         * question for Davis: what is the "value_prepend" parameter? it doesn't seem to be used
         * also, should the name be an arbitrary string, or "name", as in levelMeter below
         */
        envSection              = std::make_unique<EnvelopeSection>("ENV", "ENV", params.env ,listeners, *this);
        transpositionSlider     = std::make_unique<TranspositionSliderSection>(&params.transpose, listeners,name.toStdString());
        velocityMinMaxSlider    = std::make_unique<OpenGL_VelocityMinMaxSlider>(&params.velocityMinMax, listeners);

        // we add subsections for the elements that have been defined as sections
        addSubSection (envSection.get());
        addSubSection (transpositionSlider.get());

        // this slider does not need a section, since it's just one OpenGL component
        velocityMinMaxSlider->setComponentID ("velocity_min_max");
        addStateModulatedComponent (velocityMinMaxSlider.get());

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        addSubSection(levelMeter.get());

    }

    void paintBackground (juce::Graphics& g) override
    {
        SynthSection::paintContainer (g);
        paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);

        for (auto& slider : _sliders)
        {
            drawLabelForComponent (g, slider->getName(), slider.get());
        }

        paintChildrenBackgrounds (g);
    }

    // complex UI elements in this prep
    std::unique_ptr<TranspositionSliderSection> transpositionSlider;
    std::unique_ptr<EnvelopeSection> envSection;
    std::unique_ptr<OpenGL_VelocityMinMaxSlider> velocityMinMaxSlider;

    // place to store generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

    // level meter with output gain slider
    std::shared_ptr<PeakMeterSection> levelMeter;

    void resized() override;
};

#endif //BITKLAVIER2_DIRECTPARAMETERSVIEW_H
