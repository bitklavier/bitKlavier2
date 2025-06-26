//
// Created by Dan Trueman on 6/26/25.
//

#ifndef BITKLAVIER0_KNOBSSECTION_H
#define BITKLAVIER0_KNOBSSECTION_H
#include "synth_section.h"
#include "synth_slider.h"
#include <PreparationStateImpl.h>
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include <bitset>

class KnobsSection : public SynthSection
{
public:
    KnobsSection(chowdsp::ParamHolder *params, chowdsp::ParameterListeners& listeners, std::string parent_uuid)
    // try changing this to a KnobsParamHolder, which we need to create, along the lines TransposeParams, etc...
    {
        setComponentID(parent_uuid);

        // go through and get all the main float params (gain, hammer, etc...), make sliders for them
        // all the params for this prep are defined in struct DirectParams, in DirectProcessor.h
        for ( auto &param_ : *params->getFloatParams())
        {
            auto slider = std::make_unique<SynthSlider>(param_->paramID);
            auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
            addSlider(slider.get());
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            floatAttachments.emplace_back(std::move(attachment));
            _sliders.emplace_back(std::move(slider));
        }

        // border for the collection of output knobs
        knobsBorder.setName("knobsBorder");
        knobsBorder.setText("Output Gain Controls");
        knobsBorder.setTextLabelPosition(juce::Justification::centred);
        addAndMakeVisible(knobsBorder);
    }
    ~KnobsSection(){}

    // generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

    juce::GroupComponent knobsBorder;

    void resized() override;
};

#endif //BITKLAVIER0_KNOBSSECTION_H
