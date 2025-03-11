//
// Created by Davis Polito on 3/11/25.
//

#ifndef BITKLAVIER2_PARAMETER_CHANGE_BUFFER_H
#define BITKLAVIER2_PARAMETER_CHANGE_BUFFER_H
#include <vector>
#include <juce_data_structures/juce_data_structures.h>
namespace bitklavier {
    struct ParameterChangeBuffer {
        ParameterChangeBuffer()
        {
            changeState.reserve(10);
        }

        std::vector<std::pair<int,juce::ValueTree>> changeState = {};
    };
}
#endif //BITKLAVIER2_PARAMETER_CHANGE_BUFFER_H<