//
// Created by Dan Trueman on 7/26/25.
//

#ifndef BITKLAVIER0_BLENDRONICPARAMETERSVIEW_H
#define BITKLAVIER0_BLENDRONICPARAMETERSVIEW_H

#include "BlendronicProcessor.h"
#include "OpenGL_MultiSlider.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"

class BlendronicParametersView : public SynthSection
{
public:
    BlendronicParametersView (chowdsp::PluginState& pluginState, BlendronicParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection ("")
{
    // the name that will appear in the UI as the name of the section
    setName ("blendronic");

    // every section needs a LaF
    //  main settings for this LaF are in assets/default.bitklavierskin
    //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
    //  we probably want to merge these in the future, but ok for now
    setLookAndFeel (DefaultLookAndFeel::instance());
    setComponentID (name);

    // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
    // we need to grab the listeners for this preparation here, so we can pass them to components below
    auto& listeners = pluginState.getParameterListeners();

    // we're leaving out "outputGain" since that has its own VolumeSlider
    for (auto& param_ : *params.getFloatParams())
    {
        if ( // make group of params to display together
            param_->paramID == "InputGain" ||
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

    beatLengthsSlider = std::make_unique<OpenGL_MultiSlider>(&params.beatLengths, listeners);
    beatLengthsSlider->setComponentID ("beat_lengths");
    addStateModulatedComponent (beatLengthsSlider.get());

//    beatlengthSlider     = std::make_unique<TranspositionSliderSection>(&params.transpose, listeners,name.toStdString());
//
//    // create the more complex UI elements
//    envSection              = std::make_unique<EnvelopeSection>( params.env ,listeners, *this);
//    transpositionSlider     = std::make_unique<TranspositionSliderSection>(&params.transpose, listeners,name.toStdString());
//    velocityMinMaxSlider    = std::make_unique<OpenGL_VelocityMinMaxSlider>(&params.velocityMinMax, listeners);
//
//    // we add subsections for the elements that have been defined as sections
//    addSubSection (envSection.get());
//    addSubSection (transpositionSlider.get());
//
//    // this slider does not need a section, since it's just one OpenGL component
//    velocityMinMaxSlider->setComponentID ("velocity_min_max");
//    addStateModulatedComponent (velocityMinMaxSlider.get());

    // the level meter and output gain slider (right side of preparation popup)
    // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
    levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
    addSubSection(levelMeter.get());
//    setSkinOverride(Skin::kBlendronic);
}

void paintBackground (juce::Graphics& g) override
{
    setLabelFont(g);
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

//// complex UI elements in this prep
//std::unique_ptr<TranspositionSliderSection> transpositionSlider;
//std::unique_ptr<EnvelopeSection> envSection;
//std::unique_ptr<OpenGL_VelocityMinMaxSlider> velocityMinMaxSlider;

// place to store generic sliders/knobs for this prep, with their attachments for tracking/updating values
std::vector<std::unique_ptr<SynthSlider>> _sliders;
std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

std::unique_ptr<OpenGL_MultiSlider> beatLengthsSlider;

// level meter with output gain slider
std::shared_ptr<PeakMeterSection> levelMeter;

void resized() override;
};

#endif //BITKLAVIER0_BLENDRONICPARAMETERSVIEW_H
