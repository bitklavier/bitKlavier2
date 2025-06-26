//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#define BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#include "OpenGL_VelocityMinMaxSlider.h"
#include "TranspositionSliderSection.h"
#include "VelocityMinMaxParams.h"
#include "envelope_section.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "DirectProcessor.h"
#include "peak_meter_section.h"

class DirectParametersView : public SynthSection
{
public:
    DirectParametersView(chowdsp::PluginState& pluginState, DirectParams& params, juce::String name, OpenGlWrapper *open_gl) : SynthSection("")
    {
        // the name that will appear in the UI as the name of the section
        setName("direct");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel(DefaultLookAndFeel::instance());
        setComponentID(name);

        // pluginState is really preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // go through and get all the main float params (gain, hammer, etc...), make sliders for them
        // all the params for this prep are defined in struct DirectParams, in DirectProcessor.h
        for ( auto &param_ : *params.getFloatParams())
        {
            auto slider = std::make_unique<SynthSlider>(param_->paramID);
            auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
            addSlider(slider.get()); // adds the slider to the synthSection
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            floatAttachments.emplace_back(std::move(attachment));
            _sliders.emplace_back(std::move(slider));
        }

        // create larger UI sections
        envSection              = std::make_unique<EnvelopeSection>("ENV", "ENV", params.env ,listeners, *this);
        transpositionSlider     = std::make_unique<TranspositionSliderSection>(&params.transpose, listeners,name.toStdString());
        velocityMinMaxSlider    = std::make_unique<OpenGL_VelocityMinMaxSlider>(&params.velocityMinMax, listeners);

        // border for the collection of output knobs
        knobsBorder.setName("knobsBorder");
        knobsBorder.setText("Output Gain Controls");
        knobsBorder.setTextLabelPosition(juce::Justification::centred);
        addAndMakeVisible(knobsBorder);

        // we add subsections for the elements that have been defined as sections
        addSubSection(envSection.get());
        addSubSection(transpositionSlider.get());

        // this slider does not need a section, since it's just one OpenGL component
        velocityMinMaxSlider->setComponentID("velocity_min_max");
        addStateModulatedComponent(velocityMinMaxSlider.get());

        // to access and display the updating audio output levels
        levelMeter = std::make_unique<PeakMeterSection>("peakMeter", &params.outputLevels);
        addSubSection(levelMeter.get());

//        params.lastVelocityParam; // this should be passed to BKSynth and updated with the most recent noteOn velocity
//        velocityMinMaxSlider->setDisplayValue(56.5); // this is what we should callback to to set the display velocity
    }

    void paintBackground(juce::Graphics& g) override
    {
        SynthSection::paintContainer(g);
        paintHeadingText(g);
        paintBorder(g);
        paintKnobShadows(g);
        for (auto& slider : _sliders) {
            drawLabelForComponent(g, slider->getName(), slider.get());
        }
        paintChildrenBackgrounds(g);
        knobsBorder.paint(g);
    }

    // complex UI elements in this prep
    std::unique_ptr<TranspositionSliderSection> transpositionSlider;
    std::unique_ptr<EnvelopeSection> envSection;
    std::unique_ptr<OpenGL_VelocityMinMaxSlider> velocityMinMaxSlider;

    // generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

    juce::GroupComponent knobsBorder;
    std::shared_ptr<PeakMeterSection> levelMeter; // this should not have to be a shared pointer, nor should its components.

    void resized() override;

};

#endif //BITKLAVIER2_DIRECTPARAMETERSVIEW_H
