if(NOT DEFINED INPUT_LIST_FILE OR INPUT_LIST_FILE STREQUAL "")
    message(FATAL_ERROR "check_parameterid_underscores: INPUT_LIST_FILE not provided")
endif()

if(NOT EXISTS "${INPUT_LIST_FILE}")
    message(FATAL_ERROR "check_parameterid_underscores: list file not found: ${INPUT_LIST_FILE}")
endif()

# Single-line match (so we can print the exact offending line).
# Matches: juce::ParameterID { "something_with_underscore", ...
set(REGEX "juce::ParameterID[ \t]*\\{[ \t]*\"[^\"]*_[^\"]*\"[ \t]*,")

file(STRINGS "${INPUT_LIST_FILE}" files)

set(total 0)

foreach(file IN LISTS files)
    if(NOT EXISTS "${file}")
        continue()
    endif()

    file(STRINGS "${file}" lines)
    set(line_num 0)

    foreach(line IN LISTS lines)
        math(EXPR line_num "${line_num}+1")
        string(REGEX MATCH "${REGEX}" m "${line}")

        if(NOT m STREQUAL "")
            math(EXPR total "${total}+1")

            # Compiler-style output (clickable in IDEs)
            # Use SEND_ERROR so CMake marks the step as failed.
            message(SEND_ERROR "${file}:${line_num}: error: juce::ParameterID string contains '_' (use camelCase): ${line}")
        endif()
    endforeach()
endforeach()

if(total GREATER 0)
    message(FATAL_ERROR "Found ${total} invalid juce::ParameterID string(s) containing '_'")
endif()
