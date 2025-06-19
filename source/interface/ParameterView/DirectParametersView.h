//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#define BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#include "../components/opengl/OpenGL_TranspositionSlider.h"
#include "../components/opengl/OpenGL_VelocityMinMaxSlider.h"
//#include "ParametersView.h" // don't need this anymore! can probably delete this file completely
#include "TransposeParams.h"
#include "VelocityMinMaxParams.h"
#include "envelope_section.h"
#include "synth_section.h"
#include "synth_slider.h"

// move this out to its own class, since it will be needed by Nostalgic and maybe others
class TranspositionSliderSection : public SynthSection
{
public:
    TranspositionSliderSection(TransposeParams *params, chowdsp::ParameterListeners& listeners, std::string parent_uuid)
            : slider(std::make_unique<OpenGL_TranspositionSlider>(params,listeners)), SynthSection("")
    {
        setComponentID(parent_uuid);

        //button for setting whether the transposition slider uses tuning from an attached Tuning pre
        on = std::make_unique<SynthButton>(params->transpositionUsesTuning->paramID);
        on_attachment = std::make_unique<chowdsp::ButtonAttachment>(params->transpositionUsesTuning,listeners,*on,nullptr);
        on->setComponentID(params->transpositionUsesTuning->paramID);
        on->setText("use Tuning?");
        addSynthButton(on.get());
        addAndMakeVisible(on.get());

        //set component id to map to statechange params set in processor constructor
        slider->setComponentID("transpose");

        //needed to get picked up by modulations
        // since we have 12 individual sliders here, these will be wrapped into one state change for all together
        // don't need to call this for simpler things like a basic slider
        addStateModulatedComponent(slider.get());
    }
    ~TranspositionSliderSection() {}

    void paintBackground(juce::Graphics& g) {
        paintContainer(g);
        paintHeadingText(g);

        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
        paintBorder(g);
    }

    // move to cpp file?
    void resized() override
    {
        slider->setBounds(0, 0, getWidth(), getHeight());
        slider->redoImage();

        int onWidth = 100;
        int onHeight = 20;
        on->setBounds(getWidth() - onWidth, getHeight() - onHeight, onWidth, onHeight);
        on->toFront(false);

        SynthSection::resized();
    }

    std::unique_ptr<OpenGL_TranspositionSlider> slider;
    std::unique_ptr<SynthButton> on;
    std::unique_ptr<chowdsp::ButtonAttachment> on_attachment;
};


class DirectParametersView : public SynthSection
{
public:
    DirectParametersView(chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params, juce::String name, OpenGlWrapper *open_gl) : SynthSection("")
    {
        // the name that will appear in the UI as the name of the section
        setName("direct");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future
        setLookAndFeel(DefaultLookAndFeel::instance());
        setComponentID(name);

        // pluginState is really preparationState
        // all the params for this prep are defineed in DirectParams, in DirectProcessor.h
        auto& listeners = pluginState.getParameterListeners();

        // go through and get all the main float params (gain, hammer, etc...), make sliders for them
        for ( auto &param_ : *params.getFloatParams())
        {
            auto slider = std::make_unique<SynthSlider>(param_->paramID);
            auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
            addSlider(slider.get()); // adds the slider to the synthSection
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            floatAttachments.emplace_back(std::move(attachment));
            _sliders.emplace_back(std::move(slider));
        }

        //find complex parameters
        for (auto paramHolder : *params.getParamHolders())
        {
            if (auto *envParams = dynamic_cast<EnvParams*>(paramHolder))
                envSection = std::make_unique<EnvelopeSection>("ENV", "ENV",*envParams,listeners, *this);//std::make_unique<BooleanParameterComponent>(*boolParam, listeners);

            if (auto *sliderParams = dynamic_cast<TransposeParams*>(paramHolder))
                transpositionSlider = std::make_unique<TranspositionSliderSection>(sliderParams,listeners,name.toStdString());

            if (auto *sliderParam = dynamic_cast<VelocityMinMaxParams*>(paramHolder))
                velocityMinMaxSlider = std::make_unique<OpenGL_VelocityMinMaxSlider>(sliderParam,listeners);
        }

        addSubSection(envSection.get());
        addSubSection(transpositionSlider.get());

        velocityMinMaxSlider->setComponentID("velocity_min_max");
        addStateModulatedComponent(velocityMinMaxSlider.get());

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

    std::unique_ptr<TranspositionSliderSection> transpositionSlider;
    std::unique_ptr<EnvelopeSection> envSection;

    void resized() override;

    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::unique_ptr<OpenGL_VelocityMinMaxSlider> velocityMinMaxSlider;

};

#endif //BITKLAVIER2_DIRECTPARAMETERSVIEW_H
