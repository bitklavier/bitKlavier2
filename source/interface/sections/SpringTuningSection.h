//
// Created by Dan Trueman on 7/7/25.
//

#ifndef BITKLAVIER0_SPRINGTUNINGSECTION_H
#define BITKLAVIER0_SPRINGTUNINGSECTION_H

#include "synth_section.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "SpringTuningParams.h"
#include "open_gl_combo_box.h"
#include "synth_slider.h"
#include "TuningUtils.h"
#include "OpenGL_LabeledBorder.h"

class SpringTuningSection : public SynthSection
{
public:
    SpringTuningSection (
        juce::String name,
        SpringTuningParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent,
        chowdsp::PluginState& pluginState);

    virtual ~SpringTuningSection();

    std::shared_ptr<PlainTextComponent> currentFundamental;

    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    //holds the 6 float params
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::vector<std::shared_ptr<PlainTextComponent> > slider_labels;

    // interval spring length scale selection menus
    std::unique_ptr<OpenGLComboBox> scaleId_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> scaleId_ComboBox_ComboBoxAttachment;
    std::unique_ptr<OpenGLComboBox> intervalFundamental_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> intervalFundamental_ComboBoxAttachment;
    std::shared_ptr<PlainTextComponent> intervalsLabel;

    // tether/anchor scale location selection menus
    std::unique_ptr<OpenGLComboBox> scaleId_tether_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> scaleId_tether_ComboBox_ComboBoxAttachment;
    std::unique_ptr<OpenGLComboBox> tetherFundamental_ComboBox;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tetherFundamental_ComboBoxAttachment;
    std::shared_ptr<PlainTextComponent> anchorsLabel;

    // individual interval weight knobs
    std::vector<std::unique_ptr<SynthSlider>> intervalWeightSliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> intervalWeightsSliders_sliderAttachments;
    std::vector<std::shared_ptr<PlainTextComponent> > intervalWeights_labels;

    // toggles for setting springMode for each interval weight
    std::vector<std::unique_ptr<SynthButton>> useLocalOrFundamentalToggles;
    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> useLocalOrFundamentalToggles_sliderAttachments;

    std::shared_ptr<OpenGL_LabeledBorder> sectionBorder;
};

#endif //BITKLAVIER0_SPRINGTUNINGSECTION_H
