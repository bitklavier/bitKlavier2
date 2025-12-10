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

        // pluginState is really more like preparationState; the state holder for this preparation (not the whole app/plugin)
        // we need to grab the listeners for this preparation here, so we can pass them to components below
        auto& listeners = pluginState.getParameterListeners();

        // bypass EQ button
        activeEq_toggle = std::make_unique<SynthButton>(eqparams_.activeEq->paramID);
        activeEq_attachment = std::make_unique<chowdsp::ButtonAttachment>(eqparams_.activeEq, listeners, *activeEq_toggle, nullptr);
        activeEq_toggle->setComponentID(eqparams_.activeEq->paramID);
        addSynthButton(activeEq_toggle.get(), true);
        activeEq_toggle->setPowerButton();

        // reset EQ button
        reset_button = std::make_unique<SynthButton>("reset");
        reset_button_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.resetEq,listeners,*reset_button,nullptr);
        reset_button->setComponentID("reset");
        addSynthButton(reset_button.get(), true);
        reset_button->setText("reset EQ");

        // EQ Graph
        // equalizerGraph = std::static_pointer_cast<OpenGlComponent>(
        //     std::make_shared<OpenGL_EqualizerGraph>(&this, listeners)
        // );
        // addOpenGlComponent (equalizerGraph);
        // equalizerGraph = std::make_unique<OpenGL_EqualizerGraph>(&this, listeners);
        // addOpenGlComponent (equalizerGraph);


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

        // to catch presses of the reset button
        eqCallbacks += {listeners.addParameterListener(
            eqparams_.resetEq,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                if (eqparams_.resetEq.get()->get()) {
                    // todo make this a function or just set all parameters here?
                    // resetToDefault() doesn't do anything about activeFilters
                    eqparams_.loCutFilterParams.resetToDefault();
                    eqparams_.peak1FilterParams.resetToDefault();
                    eqparams_.peak2FilterParams.resetToDefault();
                    eqparams_.peak3FilterParams.resetToDefault();
                    eqparams_.hiCutFilterParams.resetToDefault();
                    eqparams_.resetEq->setParameterValue(false);
                }
            })
        };

        // to catch presses of the eq power button
        eqCallbacks += {listeners.addParameterListener(
            eqparams_.activeEq,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                // if EQ is off, filters should be off but save which filters were on
                if (!eqparams_.activeEq.get()->get()) {
                    // todo: is this threadsafe?
                    eqparams_.activeFilters = {
                        eqparams_.loCutFilterParams.filterActive->get(),
                        eqparams_.peak1FilterParams.filterActive->get(),
                        eqparams_.peak2FilterParams.filterActive->get(),
                        eqparams_.peak3FilterParams.filterActive->get(),
                        eqparams_.hiCutFilterParams.filterActive->get()
                    };

                    eqparams_.loCutFilterParams.filterActive->setParameterValue(false);
                    eqparams_.peak1FilterParams.filterActive->setParameterValue(false);
                    eqparams_.peak2FilterParams.filterActive->setParameterValue(false);
                    eqparams_.peak3FilterParams.filterActive->setParameterValue(false);
                    eqparams_.hiCutFilterParams.filterActive->setParameterValue(false);
                }
                // if EQ is on, turn on the filters that were on previously
                if (eqparams_.activeEq.get()->get()) {
                    // todo: is this threadsafe? i shouldn't hardcode the numbers
                    if (eqparams_.activeFilters.getUnchecked (0)) eqparams_.loCutFilterParams.filterActive->setParameterValue(true);
                    if (eqparams_.activeFilters.getUnchecked (1)) eqparams_.peak1FilterParams.filterActive->setParameterValue(true);
                    if (eqparams_.activeFilters.getUnchecked (2)) eqparams_.peak2FilterParams.filterActive->setParameterValue(true);
                    if (eqparams_.activeFilters.getUnchecked (3)) eqparams_.peak3FilterParams.filterActive->setParameterValue(true);
                    if (eqparams_.activeFilters.getUnchecked (4)) eqparams_.hiCutFilterParams.filterActive->setParameterValue(true);
                }
            })
        };
    }

    void paintBackground (juce::Graphics& g) override
    {
        setLabelFont(g);
        SynthSection::paintContainer (g);
        paintHeadingText (g);
        paintBorder (g);
        paintKnobShadows (g);
        paintChildrenBackgrounds (g);
    }

    chowdsp::ScopedCallbackList eqCallbacks;

    // active eq & reset buttons
    std::unique_ptr<SynthButton> activeEq_toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> activeEq_attachment;
    std::unique_ptr<SynthButton> reset_button;
    std::unique_ptr<chowdsp::ButtonAttachment> reset_button_attachment;

    // equalizer graph
    std::unique_ptr<OpenGL_EqualizerGraph> equalizerGraph;

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