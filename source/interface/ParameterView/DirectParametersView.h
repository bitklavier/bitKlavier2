//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#define BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#include "../components/opengl/OpenGL_TranspositionSlider.h"
#include "../components/opengl/OpenGL_VelocityMinMaxSlider.h"
#include "TransposeParams.h"
#include "VelocityMinMaxParams.h"
#include "envelope_section.h"
#include "synth_section.h"
#include "synth_slider.h"

// move this out to its own class, since it will be needed by Nostalgic and maybe others
/*
 * TranspositionSliderSection combines a StackedSlider (wrapped in OpenGL_TranspositionSlider) with a toggle
 *  the toggle "on" is to set whether the transposition slider uses the attached Tuning or not
 *  so, if it is toggled true, and the TranspositionSlider has values of 0 and 4, the 4 will be tuned according
 *  to the Tuning (say, 14c flat for basic 5/4 M3rd)
 *  Otherwise, the 4 will be an ET M3rd
 */
class TranspositionSliderSection : public SynthSection
{
public:
    TranspositionSliderSection(TransposeParams *params, chowdsp::ParameterListeners& listeners, std::string parent_uuid)
            : slider(std::make_unique<OpenGL_TranspositionSlider>(params,listeners)), SynthSection("")
    {
        setComponentID(parent_uuid);

        //button for setting whether the transposition slider uses tuning from an attached Tuning
        on = std::make_unique<SynthButton>(params->transpositionUsesTuning->paramID);

        // "on" has to have an attachment so we can track its value; the listeners for this preparation will now know about it
        on_attachment = std::make_unique<chowdsp::ButtonAttachment>(params->transpositionUsesTuning,listeners,*on,nullptr);

        // unique ID that will be combined with parentID in addSynthButton so we can track this
        on->setComponentID(params->transpositionUsesTuning->paramID);

        // add it and show it
        addSynthButton(on.get(), true);

        // what we want the button to show to the user
        on->setText("use Tuning?");

        //set component id to map to statechange params set in processor constructor
        slider->setComponentID("transpose");

        //addStateModulatedComponent is needed for this complex slider to get picked up by modulations
        // since we have 12 individual sliders here, these will be wrapped into one state change for all together
        // don't need to call this for simpler things like a basic slider or the button above
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

        // find and create the more complex parameters
        for (auto paramHolder : *params.getParamHolders())
        {
            if (auto *envParams = dynamic_cast<EnvParams*>(paramHolder))
                envSection = std::make_unique<EnvelopeSection>("ENV", "ENV",*envParams,listeners, *this);//std::make_unique<BooleanParameterComponent>(*boolParam, listeners);

            if (auto *sliderParams = dynamic_cast<TransposeParams*>(paramHolder))
                transpositionSlider = std::make_unique<TranspositionSliderSection>(sliderParams,listeners,name.toStdString());

            if (auto *sliderParam = dynamic_cast<VelocityMinMaxParams*>(paramHolder))
                velocityMinMaxSlider = std::make_unique<OpenGL_VelocityMinMaxSlider>(sliderParam,listeners);
        }

        // we add subsections for the elements that have been defined as sections
        addSubSection(envSection.get());
        addSubSection(transpositionSlider.get());

        // this slider does not need a section, since it's just one OpenGL component
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

    // complex UI elements in this prep
    std::unique_ptr<TranspositionSliderSection> transpositionSlider;
    std::unique_ptr<EnvelopeSection> envSection;
    std::unique_ptr<OpenGL_VelocityMinMaxSlider> velocityMinMaxSlider;

    // generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

    void resized() override;

};

#endif //BITKLAVIER2_DIRECTPARAMETERSVIEW_H
