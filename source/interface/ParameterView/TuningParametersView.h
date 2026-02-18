//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_TUNINGPARAMETERSVIEW_H
#define BITKLAVIER2_TUNINGPARAMETERSVIEW_H

#include "AdaptiveTuningSection.h"
#include "OffsetKnobSection.h"
#include "OpenGL_AbsoluteKeyboardSlider.h"
#include "SemitoneWidthSection.h"
#include "SpringTuningSection.h"
#include "TuningProcessor.h"
#include "open_gl_combo_box.h"
#include "synth_section.h"
#include "synth_slider.h"

class AdaptiveTuningSection;
class TuningParametersView : public SynthSection, BKTuningKeyboardSlider::Listener, juce::Timer
{
public:
    TuningParametersView(chowdsp::PluginState& pluginState, TuningParams& param, juce::String name, OpenGlWrapper *open_gl);
    ~TuningParametersView(){ stopTimer(); }

    void paintBackground(juce::Graphics& g) override
    {
        SynthSection::paintContainer(g);
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);

        drawSpiral(g);
    }

    void keyboardSliderChanged(juce::String name) override;

    void showStaticTuning(bool show);
    void showAdaptiveTuning(bool show);
    void showSpringTuning(bool show);
    void showCurrentTuningType();

    void timerCallback(void) override;

    /*
     * important to include this override in all parameter view classes that use timers
     */
    void stopAllTimers() override {
        stopTimer();
    }

    void drawSpiral(juce::Graphics& g);

    void resized() override;

    std::shared_ptr<PlainTextComponent> prepTitle;

    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::unique_ptr<OpenGLComboBox> tuning_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tuning_attachment;
    std::unique_ptr<OpenGLComboBox> fundamental_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> fundamental_attachment;
    std::unique_ptr<OpenGLComboBox> tuningtype_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tuningtype_attachment;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> absolutekeyboard;
    std::unique_ptr<OpenGLCircularKeyboardSlider> circular_keyboard;
    chowdsp::ScopedCallbackList tuningCallbacks;

    std::unique_ptr<SemitoneWidthSection> semitoneSection;
    std::unique_ptr<AdaptiveTuningSection> adaptiveSection;
    std::unique_ptr<SpringTuningSection> springTuningSection;
    std::unique_ptr<OffsetKnobSection> offsetKnobSection;

    std::shared_ptr<PlainTextComponent> lastNoteDisplay;
    std::shared_ptr<PlainTextComponent> lastIntervalDisplay;
    std::shared_ptr<PlainTextComponent> lastFrequencyDisplay;

    juce::Rectangle<int> spiralBox;

    TuningParams& params;

};


#endif //BITKLAVIER2_TUNINGPARAMETERSVIEW_H
