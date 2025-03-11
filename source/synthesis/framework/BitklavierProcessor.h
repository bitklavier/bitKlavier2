//
// Created by Davis Polito on 3/5/25.
//

#ifndef BITKLAVIER2_BITKLAVIERPROCESSOR_H
#define BITKLAVIER2_BITKLAVIERPROCESSOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
struct ParameterChangeBuffer {




    bool valuetree_changed = false;
    std::vector<juce::ValueTree> changeState = {};
};


#endif //BITKLAVIER2_BITKLAVIERPROCESSOR_H
