// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <unordered_set>
#include <string>

class ChucKCodeTokeniser : public juce::CodeTokeniser
{
public:
    enum TokenType
    {
        tokenType_error = 0,
        tokenType_comment,
        tokenType_keyword,
        tokenType_builtin_type,
        tokenType_builtin_ugen,
        tokenType_bk_global,
        tokenType_identifier,
        tokenType_integer,
        tokenType_float,
        tokenType_time_literal,
        tokenType_string,
        tokenType_operator,
        tokenType_whitespace,
        tokenType_numTypes
    };

    ChucKCodeTokeniser();
    ~ChucKCodeTokeniser() override = default;

    int readNextToken (juce::CodeDocument::Iterator& source) override;
    juce::CodeEditorComponent::ColourScheme getDefaultColourScheme() override;

private:
    std::unordered_set<std::string> keywords_;
    std::unordered_set<std::string> builtinTypes_;
    std::unordered_set<std::string> builtinUGens_;
    std::unordered_set<std::string> bkGlobals_;
    std::unordered_set<std::string> timeUnits_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChucKCodeTokeniser)
};
