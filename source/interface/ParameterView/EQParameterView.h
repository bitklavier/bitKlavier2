//
// Created by Myra Norton on 11/14/25.
//

#ifndef BITKLAVIER2_EQPARAMETERVIEW_H
#define BITKLAVIER2_EQPARAMETERVIEW_H
#include "EQProcessor.h"
#include "EQFilterSection.h"
#include "synth_section.h"
#include "peak_meter_section.h"
#include "synth_slider.h"
#include "default_look_and_feel.h"
#include "OpenGL_EqualizerGraph.h"

class EQFilterSection;
class EQParameterView : public SynthSection
{
public:
    EQParameterView (chowdsp::PluginState& pluginState, EQParams& params, juce::String name, OpenGlWrapper* open_gl) : SynthSection (""), eqparams_ (params)
    {
        // the name that will appear in the UI as the name of the section
        setName ("eq");
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);
        setSkinOverride(Skin::kDirect);

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
        addOpenGlComponent(prepTitle);
        prepTitle->setJustification(juce::Justification::centredLeft);
        prepTitle->setFontType (PlainTextComponent::kTitle);
        prepTitle->setRotation (-90);

        // bypass EQ button
        activeEq_toggle = std::make_unique<SynthButton>(eqparams_.activeEq->paramID);
        activeEq_attachment = std::make_unique<chowdsp::ButtonAttachment>(eqparams_.activeEq, listeners, *activeEq_toggle, nullptr);
        activeEq_toggle->setComponentID(eqparams_.activeEq->paramID);
        addSynthButton(activeEq_toggle.get(), true);
        activeEq_toggle->setText("power");

        // reset EQ button
        reset_button = std::make_unique<SynthButton>("reset");
        reset_button_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.resetEq,listeners,*reset_button,nullptr);
        reset_button->setComponentID("reset");
        addSynthButton(reset_button.get(), true);
        reset_button->setText("reset");

        // EQ Graph
        equalizerGraph = std::make_unique<OpenGL_EqualizerGraph> (&params, listeners);
        addOpenGlComponent (equalizerGraph->getImageComponent());
        addAndMakeVisible(equalizerGraph.get());

        // eq filter sections
        loCutSection = std::make_unique<EQCutFilterSection>(name, eqparams_.loCutFilterParams, listeners, *this);
        addSubSection (loCutSection.get());
        peak1Section = std::make_unique<EQPeakFilterSection>(name, eqparams_.peak1FilterParams, listeners, *this);
        addSubSection(peak1Section.get());
        peak2Section = std::make_unique<EQPeakFilterSection>(name, eqparams_.peak2FilterParams, listeners, *this);
        addSubSection(peak2Section.get());
        peak3Section = std::make_unique<EQPeakFilterSection>(name, eqparams_.peak3FilterParams, listeners, *this);
        addSubSection(peak3Section.get());
        hiCutSection = std::make_unique<EQCutFilterSection>(name, eqparams_.hiCutFilterParams, listeners, *this);
        addSubSection(hiCutSection.get());

        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        levelMeter->setLabel("Main");
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        // sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSend, listeners, &params.sendLevels);
        // sendLevelMeter->setLabel("Send");
        // addSubSection(sendLevelMeter.get());

        // and for input level meter/slider
        inLevelMeter = std::make_unique<PeakMeterSection>(name, params.inputGain, listeners, &params.inputLevels);
        inLevelMeter->setLabel("Input");
        addSubSection(inLevelMeter.get());

        // redraw eq graph
        eqparams_.doForAllParameters ([this, &listeners] (auto& param, size_t) {
            eqRedoImageCallbacks += {listeners.addParameterListener(
                param,
                chowdsp::ParameterListenerThread::MessageThread,
                [this]() {
                    this->eqparams_.updateCoefficients();
                    this->equalizerGraph->redoImage();
                })
            };
        });


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

    chowdsp::ScopedCallbackList eqRedoImageCallbacks;

    // active eq & reset buttons
    std::unique_ptr<SynthButton> activeEq_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> activeEq_attachment;
    std::unique_ptr<SynthButton> reset_button;
    std::unique_ptr<chowdsp::ButtonAttachment> reset_button_attachment;

    // equalizer graph
    std::shared_ptr<OpenGL_EqualizerGraph> equalizerGraph;

    // EQ Filter Sections
    std::unique_ptr<EQCutFilterSection> loCutSection;
    std::unique_ptr<EQPeakFilterSection> peak1Section;
    std::unique_ptr<EQPeakFilterSection> peak2Section;
    std::unique_ptr<EQPeakFilterSection> peak3Section;
    std::unique_ptr<EQCutFilterSection> hiCutSection;

    // level meters
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;
    std::shared_ptr<PeakMeterSection> inLevelMeter;

    EQParams& eqparams_;
    void resized() override;
};

#endif //BITKLAVIER2_EQPARAMETERVIEW_H