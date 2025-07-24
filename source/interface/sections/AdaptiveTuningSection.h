//
// Created by Dan Trueman on 7/4/25.
//

#ifndef BITKLAVIER0_ADAPTIVETUNINGSECTION_H
#define BITKLAVIER0_ADAPTIVETUNINGSECTION_H

#include "synth_section.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "AdaptiveTuningParams.h"
#include "open_gl_combo_box.h"
#include "synth_slider.h"

class AdaptiveTuningSection : public SynthSection
{
public:
    AdaptiveTuningSection (
        juce::String name,
        AdaptiveTuningParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent);

    virtual ~AdaptiveTuningSection();

    std::shared_ptr<PlainTextComponent> currentFundamental;

    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    std::unique_ptr<SynthSlider> clusterThreshold_Slider;
    std::unique_ptr<chowdsp::SliderAttachment> clusterThreshold_SliderAttachment;
    std::unique_ptr<SynthSlider> history_Slider;
    std::unique_ptr<chowdsp::SliderAttachment> history_SliderAttachment;

    std::unique_ptr<OpenGLComboBox> adaptiveIntervalScale_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> adaptiveIntervalScale_ComboBoxAttachment;
    std::unique_ptr<OpenGLComboBox> adaptiveAnchorScale_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> adaptiveAnchorScale_ComboBoxAttachment;
    std::unique_ptr<OpenGLComboBox> adaptiveAnchorFundamental_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> adaptiveAnchorFundamental_ComboBoxAttachment;

    std::unique_ptr<SynthButton> useInversionOfIntervalScale_Toggle;
    std::unique_ptr<chowdsp::ButtonAttachment> useInversionOfIntervalScale_ToggleAttachment;
    std::unique_ptr<SynthButton> resetButton;
    std::unique_ptr<chowdsp::ButtonAttachment> resetButton_attachment;

    std::shared_ptr<PlainTextComponent> intervalsLabel;
    std::shared_ptr<PlainTextComponent> anchorsLabel;

    juce::GroupComponent sectionBorder;
};

#endif //BITKLAVIER0_ADAPTIVETUNINGSECTION_H
