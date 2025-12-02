//
// Created by Myra Norton on 8/20/25.
//

#ifndef TEMPOPARAMETERSVIEW_H
#define TEMPOPARAMETERSVIEW_H
#include "TempoProcessor.h"
#include "OpenGL_VelocityMinMaxSlider.h"
#include "TranspositionSliderSection.h"
#include "VelocityMinMaxParams.h"
#include "envelope_section.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"


class TempoParametersView : public SynthSection
{
public:
    TempoParametersView (chowdsp::PluginState& pluginState, TempoParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection ("")
    {
        // the name that will appear in the UI as the name of the section
        setName ("Tempo");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setTextSize (24.0f);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // go through and get all the main float params (gain, hammer, etc...), make sliders for them
        // all the params for this prep are defined in struct TempoParams, in TempoProcessor.h
        // we're only including the ones that we want to group together and call "placeKnobsInArea" on
        // we're leaving out "outputGain" since that has its own VolumeSlider
        for (auto& param_ : *params.getFloatParams())
        {
            if ( // make group of params to display together
                param_->paramID == "tempo" ||
                param_->paramID == "subdivisions")
            {
                auto slider = std::make_unique<SynthSlider> (param_->paramID,param_->getModParam());
                auto attachment = std::make_unique<chowdsp::SliderAttachment> (*param_.get(), listeners, *slider.get(), nullptr);
                slider->addAttachment(attachment.get()); // necessary for mods to be able to display properly
                addSlider (slider.get()); // adds the slider to the synthSection
//                slider->setSliderStyle (juce::Slider::LinearHorizontal);
                slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setShowPopupOnHover(true);
                auto slider_label = std::make_shared<PlainTextComponent>(slider->getName(), param_->getName(20));
                addOpenGlComponent(slider_label);
                slider_label->setTextSize (12.0f);
                slider_label->setJustification(juce::Justification::centred);
                slider_labels.emplace_back(slider_label);
                floatAttachments.emplace_back (std::move (attachment));
                _sliders.emplace_back (std::move (slider));
            }
        }

        // setSkinOverride(Skin::kTempo);
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        //paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);

        // for (auto& slider : _sliders)
        // {
        //     drawLabelForComponent (g, slider->getName(), slider.get());
        // }

        paintChildrenBackgrounds (g);
    }

    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

    // place to store generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::vector<std::shared_ptr<PlainTextComponent> > slider_labels;

    void resized() override;
};

#endif //TEMPOPARAMETERSVIEW_H
