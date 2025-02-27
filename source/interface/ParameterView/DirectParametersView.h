//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#define BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#include "ParametersView.h"
#include "envelope_section.h"
#include "TransposeParams.h"
#include "OpenGlTranspositionSlider.h"
#include "synth_section.h"
#include "synth_slider.h"
using SliderAttachmentTuple = std::tuple<std::shared_ptr<SynthSlider>, std::unique_ptr<chowdsp::SliderAttachment>>;
using BooleanAttachmentTuple = std::tuple<std::shared_ptr<SynthButton>, std::unique_ptr<chowdsp::ButtonAttachment>>;
class TranspositionSliderSection : public SynthSection
{
public:
    TranspositionSliderSection(TransposeParams *params, chowdsp::ParameterListeners& listeners, std::string parent_uuid)
            : slider(std::make_unique<OpenGlTranspositionSlider>(params,listeners)), SynthSection("")
    {
        setComponentID(parent_uuid);
        on = std::make_unique<SynthButton>(params->transpositionUsesTuning->paramID);
        on_attachment = std::make_unique<chowdsp::ButtonAttachment>(params->transpositionUsesTuning,listeners,*on,nullptr);
        on->setComponentID(params->transpositionUsesTuning->paramID);
        addSynthButton(on.get());
        addAndMakeVisible(on.get());
        setActivator(on.get());
        addStateModulatedComponent(slider.get());
//        addAndMakeVisible(*slider);
//        addOpenGlComponent(slider->getImageComponent());
    }
    ~TranspositionSliderSection() {}

    void paintBackground(juce::Graphics& g) {
        paintContainer(g);
        paintHeadingText(g);

        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
        paintBorder(g);
    }
    void resized() override
    {
        int title_width = getTitleWidth();
        slider->setBounds(title_width, 0, getWidth() - title_width, getHeight());
        slider->redoImage();
        SynthSection::resized();
    }
    std::unique_ptr<OpenGlTranspositionSlider> slider;
    std::unique_ptr<SynthButton> on;
    std::unique_ptr<chowdsp::ButtonAttachment> on_attachment;
};
class DirectParametersView : public SynthSection
{
public:
    DirectParametersView(chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params,juce::String name, OpenGlWrapper *open_gl) : SynthSection("")
                                                                                                                     //bitklavier::ParametersView(pluginState,params,open_gl,false)
    {
        //envelope = std::make_unique<EnvelopeSection>("ENV", "err");
        setName("");
        setLookAndFeel(DefaultLookAndFeel::instance());

        //addSubSection(envelope.get());
        setComponentID(name);
        auto& listeners = pluginState.getParameterListeners();
        for ( auto &param_ : *params.getFloatParams())
        {
            auto slider = std::make_unique<SynthSlider>(param_->paramID);
            auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
            addSlider(slider.get());
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            floatAttachments.emplace_back(std::move(attachment));
            _sliders.emplace_back(std::move(slider));

        }
//        for ( auto &param_ : *params.getBoolParams())
//        {
//            auto button = std::make_unique<SynthButton>(param_->paramID);
//            auto attachment = std::make_unique<chowdsp::ButtonAttachment>(*param_.get(), listeners,  *button.get(), nullptr);
//            _buttons.emplace_back(std::move(button));
//            buttonAttachments.emplace_back(std::move(attachment));
//        }

        for (auto paramHolder : *params.getParamHolders())
        {
            if (auto *envParams = dynamic_cast<EnvParams*>(paramHolder))
                envSection = std::make_unique<EnvelopeSection>("ENV", "ENV",*envParams,listeners, *this);//std::make_unique<BooleanParameterComponent>(*boolParam, listeners);

            if (auto *sliderParams = dynamic_cast<TransposeParams*>(paramHolder))
                transpositionSlider = std::make_unique<TranspositionSliderSection>(sliderParams,listeners,name.toStdString());
        }

              addSubSection(envSection.get());
        addSubSection(transpositionSlider.get());




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
    }
//    std::unique_ptr<EnvelopeSection> envelope;
    //std::unique_ptr<juce::Component> transpose_uses_tuning;
    std::unique_ptr<TranspositionSliderSection> transpositionSlider;
    std::unique_ptr<EnvelopeSection> envSection;
    void resized() override;
    chowdsp::ScopedCallbackList transposeCallbacks;
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
//    std::vector<std::unique_ptr<SynthButton>> _buttons;
//    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> buttonAttachments;
};

#endif //BITKLAVIER2_DIRECTPARAMETERSVIEW_H
