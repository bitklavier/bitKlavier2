//
// Created by Dan Trueman on 8/8/25.
//

#ifndef BITKLAVIER0_ENVELOPESEQUENCESECTION_H
#define BITKLAVIER0_ENVELOPESEQUENCESECTION_H

#include "synth_section.h"
#include <chowdsp_plugin_utils/chowdsp_plugin_utils.h>
#include "EnvelopeSequenceParams.h"
#include "synth_button.h"

class EnvelopeSequenceSection : public SynthSection
{
public:
    EnvelopeSequenceSection (
        juce::String name,
        EnvelopeSequenceParams &params,
        chowdsp::ParameterListeners &listeners,
        SynthSection &parent);

    virtual ~EnvelopeSequenceSection();

    void buttonClicked (juce::Button* clicked_button);
    void setCurrentlyPlayingEnvelope(int which);
    int currentSelectedEnvelope = 1;

    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    //holds the 6 float params
    std::vector<std::unique_ptr<SynthButton>> _envActiveButtons;
    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> _envActiveButtons_toggleAttachments;

    std::vector<std::unique_ptr<SynthButton>> _envEditButtons;
    std::vector<std::unique_ptr<SynthButton>> _envPlayingButtons; // to display which one is actually playing right now

    int getEnvelopeIndex(const juce::String& s);
//
//    // interval spring length scale selection menus
//    std::unique_ptr<OpenGLComboBox> scaleId_ComboBox;
//    std::unique_ptr<chowdsp::ComboBoxAttachment> scaleId_ComboBox_ComboBoxAttachment;
//    std::unique_ptr<OpenGLComboBox> intervalFundamental_ComboBox;
//    std::unique_ptr<chowdsp::ComboBoxAttachment> intervalFundamental_ComboBoxAttachment;
//    std::shared_ptr<PlainTextComponent> intervalsLabel;
//
//    // tether/anchor scale location selection menus
//    std::unique_ptr<OpenGLComboBox> scaleId_tether_ComboBox;
//    std::unique_ptr<chowdsp::ComboBoxAttachment> scaleId_tether_ComboBox_ComboBoxAttachment;
//    std::unique_ptr<OpenGLComboBox> tetherFundamental_ComboBox;
//    std::unique_ptr<chowdsp::ComboBoxAttachment> tetherFundamental_ComboBoxAttachment;
//    std::shared_ptr<PlainTextComponent> anchorsLabel;
//
//    // individual interval weight knobs
//    std::vector<std::unique_ptr<SynthSlider>> intervalWeightSliders;
//    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> intervalWeightsSliders_sliderAttachments;
//
//    // toggles for setting springMode for each interval weight
//    std::vector<std::unique_ptr<SynthButton>> useLocalOrFundamentalToggles;
//    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> useLocalOrFundamentalToggles_sliderAttachments;
//
//    juce::GroupComponent sectionBorder;
};



#endif //BITKLAVIER0_ENVELOPESEQUENCESECTION_H
