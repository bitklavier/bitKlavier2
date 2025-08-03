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

        beatLengthsSlider = std::make_unique<OpenGL_MultiSlider>("beat_lengths", &params.beatLengths, listeners);
        beatLengthsSlider->setComponentID ("beat_lengths");
        beatLengthsSlider->setMinMaxDefaultInc({0., 8, 4., 0.01});
        beatLengthsSlider->setName("Beat Lengths (x/Tempo)");
        addStateModulatedComponent (beatLengthsSlider.get());

        delayLengthsSlider = std::make_unique<OpenGL_MultiSlider>("delay_lengths", &params.delayLengths, listeners);
        delayLengthsSlider->setComponentID ("delay_lengths");
        delayLengthsSlider->setMinMaxDefaultInc({0., 8, 4., 0.01});
        delayLengthsSlider->setName("Delay Lengths (x/Tempo)");
        addStateModulatedComponent (delayLengthsSlider.get());

        smoothingTimesSlider = std::make_unique<OpenGL_MultiSlider>("smoothing_times", &params.smoothingTimes, listeners);
        smoothingTimesSlider->setComponentID ("smoothing_times");
        smoothingTimesSlider->setMinMaxDefaultInc({0., 500, 50., 1.});
        smoothingTimesSlider->setSkewFromMidpoint(0.1);
        smoothingTimesSlider->setName("Smooth Times (ms)");
        addStateModulatedComponent (smoothingTimesSlider.get());

        feedbackCoeffsSlider = std::make_unique<OpenGL_MultiSlider>("feedback_coefficients", &params.feedbackCoeffs, listeners);
        feedbackCoeffsSlider->setComponentID ("feedback_coefficients");
        feedbackCoeffsSlider->setMinMaxDefaultInc({0., 1., 0.95, 0.001});
        feedbackCoeffsSlider->setSkewFromMidpoint(0.1);
        feedbackCoeffsSlider->setName("Feedback Coefficients (0-1)");
        addStateModulatedComponent (feedbackCoeffsSlider.get());

        /**
         * todo: these level meters/sliders need titles displayed in the UI
         */
        // the level meter and output gain slider (right side of preparation popup)
        // need to pass it the param.outputGain and the listeners so it can attach to the slider and update accordingly
        levelMeter = std::make_unique<PeakMeterSection>(name, params.outputGain, listeners, &params.outputLevels);
        addSubSection(levelMeter.get());

        // similar for send level meter/slider
        sendLevelMeter = std::make_unique<PeakMeterSection>(name, params.outputSend, listeners, &params.sendLevels);
        addSubSection(sendLevelMeter.get());

        // and for input level meter/slider
        inLevelMeter = std::make_unique<PeakMeterSection>(name, params.inputGain, listeners, &params.inputLevels);
        addSubSection(inLevelMeter.get());

        /*
         * listen for changes from mods/resets, redraw as needed
         */
        sliderChangedCallback += {
            listeners.addParameterListener
            (
            params.updateUIState, // this value will be changed whenever a mod or reset is called
            chowdsp::ParameterListenerThread::MessageThread,
                [this]
                {
                    beatLengthsSlider->updateFromParams();
                    delayLengthsSlider->updateFromParams();
                    smoothingTimesSlider->updateFromParams();
                    feedbackCoeffsSlider->updateFromParams();
                }
            )
        };

        /*
         * not sure why we need to redo this here, but they don't draw without these calls
         */
        beatLengthsSlider->drawSliders(juce::dontSendNotification);
        delayLengthsSlider->drawSliders(juce::dontSendNotification);
        smoothingTimesSlider->drawSliders(juce::dontSendNotification);
        feedbackCoeffsSlider->drawSliders(juce::dontSendNotification);

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

    chowdsp::ScopedCallbackList sliderChangedCallback;

    // place to store generic sliders/knobs for this prep, with their attachments for tracking/updating values
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;

    std::unique_ptr<OpenGL_MultiSlider> beatLengthsSlider;
    std::unique_ptr<OpenGL_MultiSlider> delayLengthsSlider;
    std::unique_ptr<OpenGL_MultiSlider> smoothingTimesSlider;
    std::unique_ptr<OpenGL_MultiSlider> feedbackCoeffsSlider;

    // level meters with gain sliders
    std::shared_ptr<PeakMeterSection> levelMeter;
    std::shared_ptr<PeakMeterSection> sendLevelMeter;
    std::shared_ptr<PeakMeterSection> inLevelMeter;

    void resized() override;
};

#endif //BITKLAVIER0_BLENDRONICPARAMETERSVIEW_H
