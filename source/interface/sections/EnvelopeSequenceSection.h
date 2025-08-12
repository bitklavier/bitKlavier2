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

    virtual ~EnvelopeSequenceSection() {}

    void buttonClicked (juce::Button* clicked_button);
    void setCurrentlyPlayingEnvelope(int which);

    void paintBackground(juce::Graphics& g) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<SynthButton>> _envActiveButtons;
    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> _envActiveButtons_toggleAttachments;

    std::vector<std::unique_ptr<SynthButton>> _envEditButtons;
    std::vector<std::unique_ptr<SynthButton>> _envPlayingButtons; // to display which one is actually playing right now

    int getEnvelopeIndex(const juce::String& s);
    EnvelopeSequenceParams& _params;
};



#endif //BITKLAVIER0_ENVELOPESEQUENCESECTION_H
