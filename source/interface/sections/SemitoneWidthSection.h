//
// Created by Dan Trueman on 6/28/25.
//

#ifndef BITKLAVIER0_SEMITONEWIDTHSECTION_H
#define BITKLAVIER0_SEMITONEWIDTHSECTION_H

#include "synth_section.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "SemiToneWidthParams.h"
#include "open_gl_combo_box.h"
#include "synth_slider.h"

class SemitoneWidthSection : public SynthSection
{
public:
    SemitoneWidthSection (
        juce::String name,
        SemitoneWidthParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent);

    virtual ~SemitoneWidthSection();

    void setAlpha(float newAlpha);
    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<SynthSlider> widthSlider_;
    std::unique_ptr<chowdsp::SliderAttachment> widthSliderAttachment;
    std::unique_ptr<OpenGLComboBox> fundamentalComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> fundamentalComboBoxAttachment;
    std::unique_ptr<OpenGLComboBox> octaveComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> octaveComboBoxAttachment;

    juce::GroupComponent sectionBorder;
};

#endif //BITKLAVIER0_SEMITONEWIDTHSECTION_H
