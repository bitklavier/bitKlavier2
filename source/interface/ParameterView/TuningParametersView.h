//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_TUNINGPARAMETERSVIEW_H
#define BITKLAVIER2_TUNINGPARAMETERSVIEW_H
//#include "envelope_section.h"
//#include "TransposeParams.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_combo_box.h"
#include "OpenGL_AbsoluteKeyboardSlider.h"
#include "TuningProcessor.h"
//#include "tuning_systems.h"
#include "SemitoneWidthSection.h"
#include "AdaptiveTuningSection.h"
#include "SpringTuningSection.h"

class AdaptiveTuningSection;
class TuningParametersView : public SynthSection, BKTuningKeyboardSlider::Listener
{
public:
    TuningParametersView(chowdsp::PluginState& pluginState, TuningParams& param, juce::String name, OpenGlWrapper *open_gl);

    void paintBackground(juce::Graphics& g) override
    {
        SynthSection::paintContainer(g);
        paintHeadingText(g);
        paintBorder(g);
        paintKnobShadows(g);
        for (auto& slider : _sliders) {
            if(slider->isVisible())
            {
                drawLabelForComponent(g, slider->getName(), slider.get());
            }
        }
        paintChildrenBackgrounds(g);
    }

    void keyboardSliderChanged(juce::String name) override;

    void showStaticTuning(bool show);
    void showAdaptiveTuning(bool show);
    void showSpringTuning(bool show);
    void resized() override;

    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::unique_ptr<OpenGLComboBox> tuning_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tuning_attachment;
    std::unique_ptr<OpenGLComboBox> fundamental_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> fundamental_attachment;
    std::unique_ptr<OpenGLComboBox> tuningtype_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tuningtype_attachment;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> keyboard;
    std::unique_ptr<OpenGLCircularKeyboardSlider> circular_keyboard;
    chowdsp::ScopedCallbackList tuningCallbacks;

    std::unique_ptr<SemitoneWidthSection> semitoneSection;
    std::unique_ptr<AdaptiveTuningSection> adaptiveSection;
    std::unique_ptr<SpringTuningSection> springTuningSection;

    std::shared_ptr<PlainTextComponent> lastNoteDisplay;
    std::shared_ptr<PlainTextComponent> lastIntervalDisplay;
    std::shared_ptr<PlainTextComponent> lastFrequencyDisplay;

    TuningParams& params;

};


#endif //BITKLAVIER2_TUNINGPARAMETERSVIEW_H
