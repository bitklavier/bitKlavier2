//
// Created by Myra Norton on 8/20/25.
//

#ifndef TEMPOPARAMETERSVIEW_H
#define TEMPOPARAMETERSVIEW_H
#include "OpenGL_HoldTimeMinMaxSlider.h"
#include "OpenGL_VelocityMinMaxSlider.h"
#include "TempoAdaptiveSection.h"
#include "TempoProcessor.h"
#include "TranspositionSliderSection.h"
#include "VelocityMinMaxParams.h"
#include "envelope_section.h"
#include "peak_meter_section.h"
#include "synth_section.h"
#include "synth_slider.h"

class TempoParametersView : public SynthSection, public juce::Timer
{
public:
    TempoParametersView (chowdsp::PluginState& pluginState, TempoParams& params, juce::String name, OpenGlWrapper* open_gl, TempoProcessor* proc) : SynthSection (""), processor(proc), params_(params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("Tempo");

        // every section needs a LaF
        //  main settings for this LaF are in assets/default.bitklavierskin
        //  different from the bk LaF that we've taken from the old JUCE, to support the old UI elements
        //  we probably want to merge these in the future, but ok for now
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);

        setSkinOverride(Skin::kDirect);

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        FullInterface *parent = findParentComponentOfClass<FullInterface>();
        if (parent)
            parent->hideSoundsetSelector();

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
                auto slider = std::make_unique<SynthSlider> (param_->paramID, param_->getModParam());
                auto attachment = std::make_unique<chowdsp::SliderAttachment> (*param_.get(), listeners, *slider.get(), nullptr);
                slider->addAttachment(attachment.get()); // necessary for mods to be able to display properly
                addSlider (slider.get()); // adds the slider to the synthSection
                slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setShowPopupOnHover(true);
                auto slider_label = std::make_shared<PlainTextComponent>(slider->getName(), param_->getName(20));
                addOpenGlComponent(slider_label);
                slider_label->setJustification(juce::Justification::centred);
                slider_labels.emplace_back(slider_label);
                floatAttachments.emplace_back (std::move (attachment));
                _sliders.emplace_back (std::move (slider));
            }
        }

        if (auto* tempoParams = dynamic_cast<TempoParams*>(&params))
        {
            tempoMode = std::make_unique<OpenGLComboBox>(tempoParams->tempoModeOptions->paramID.toStdString());
            tempoMode_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tempoParams->tempoModeOptions.get(), listeners, *tempoMode, nullptr);
            addAndMakeVisible(tempoMode.get());
            addOpenGlComponent(tempoMode->getImageComponent());
        }

        primaryKnobsBorder = std::make_shared<OpenGL_LabeledBorder>("primary knobs border", "Primary Parameters");
        addBorder(primaryKnobsBorder.get());


        adaptiveSection = std::make_unique<TempoAdaptiveSection>(pluginState, params, name, proc);
        addSubSection(adaptiveSection.get());

        tempoCallbacks += {listeners.addParameterListener(
            params.tempoModeOptions,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                updateAdaptiveSectionVisibility();
            })
        };

        updateAdaptiveSectionVisibility();
        startTimer(50);
    }

    ~TempoParametersView() override
    {
        stopTimer();
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    // prep title, vertical, left side
    std::shared_ptr<PlainTextComponent> prepTitle;

    std::unique_ptr<TempoAdaptiveSection> adaptiveSection;

    // place to store generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::vector<std::shared_ptr<PlainTextComponent> > slider_labels;

    std::unique_ptr<OpenGLComboBox> tempoMode;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tempoMode_attachment;

    std::shared_ptr<OpenGL_LabeledBorder> primaryKnobsBorder;

    TempoProcessor* processor;
    TempoParams& params_;

    void updateAdaptiveSectionVisibility();
    void timerCallback() override;

    void resized() override;
private:
    chowdsp::ScopedCallbackList tempoCallbacks;
};

#endif //TEMPOPARAMETERSVIEW_H
