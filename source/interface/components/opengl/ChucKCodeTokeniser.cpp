// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChucKCodeTokeniser.h"

ChucKCodeTokeniser::ChucKCodeTokeniser()
{
    keywords_ = {
        "if", "else", "while", "for", "until", "break", "continue", "return",
        "fun", "function", "spork", "class", "extends", "public", "private",
        "static", "pure", "new", "null", "true", "false",
        "this", "me", "now", "global", "external"
    };

    builtinTypes_ = {
        "int", "float", "dur", "time", "void", "string",
        "Event", "Object", "array", "complex", "polar", "vec3", "vec4"
    };

    builtinUGens_ = {
        "SinOsc", "Osc", "SqrOsc", "TriOsc", "SawOsc", "PulseOsc",
        "Noise", "Impulse", "Step", "Gain",
        "HPF", "LPF", "BPF", "BRF", "ResonZ", "BiQuad",
        "ADSR", "Envelope",
        "Delay", "DelayA", "DelayL",
        "Reverb", "PRCRev", "JCRev", "NRev",
        "Pan2", "Mix2",
        "Std", "Math", "Machine",
        "dac", "adc", "blackhole"
    };

    bkGlobals_ = {
        "bkMidiIn", "bkMidiOut", "bkMidiInEvent", "bkMidiOutEvent",
        "bkTuningTable", "bkTempoBPM"
    };

    timeUnits_ = {
        "samp", "ms", "second", "minute", "hour", "day", "week"
    };
}

static bool isIdentStart (juce::juce_wchar c) noexcept
{
    return juce::CharacterFunctions::isLetter (c) || c == '_';
}

static bool isIdentBody (juce::juce_wchar c) noexcept
{
    return juce::CharacterFunctions::isLetterOrDigit (c) || c == '_';
}

static bool isDigit (juce::juce_wchar c) noexcept
{
    return c >= '0' && c <= '9';
}

// Reads digits and an optional fractional part (no exponent). Returns true if
// at least one digit was consumed, and sets isFloat if a '.' was encountered.
static bool readNumericPart (juce::CodeDocument::Iterator& source, bool& isFloat) noexcept
{
    int numDigits = 0;
    while (isDigit (source.peekNextChar())) { source.skip(); ++numDigits; }

    isFloat = false;
    if (source.peekNextChar() == '.')
    {
        source.skip();
        isFloat = true;
        while (isDigit (source.peekNextChar())) source.skip();
    }

    return numDigits > 0 || isFloat;
}

int ChucKCodeTokeniser::readNextToken (juce::CodeDocument::Iterator& source)
{
    // Whitespace
    if (juce::CharacterFunctions::isWhitespace (source.peekNextChar()))
    {
        source.skipWhitespace();
        return tokenType_whitespace;
    }

    auto firstChar = source.peekNextChar();
    if (firstChar == 0) return tokenType_error;

    // Comments: // or /* */
    if (firstChar == '/')
    {
        source.skip();
        auto next = source.peekNextChar();
        if (next == '/')
        {
            source.skipToEndOfLine();
            return tokenType_comment;
        }
        if (next == '*')
        {
            source.skip();
            bool lastWasStar = false;
            for (;;)
            {
                auto c = source.nextChar();
                if (c == 0 || (c == '/' && lastWasStar)) break;
                lastWasStar = (c == '*');
            }
            return tokenType_comment;
        }
        // /=> (divide-chuck)
        if (source.peekNextChar() == '=') source.skip();
        return tokenType_operator;
    }

    // Strings
    if (firstChar == '"')
    {
        source.skip(); // consume opening quote
        for (;;)
        {
            auto c = source.nextChar();
            if (c == '"' || c == 0) break;
            if (c == '\\') source.skip(); // skip escaped char
        }
        return tokenType_string;
    }

    // Numeric literals (possibly followed by :: timeUnit)
    if (isDigit (firstChar))
    {
        bool isFloat = false;
        readNumericPart (source, isFloat);

        // Peek for :: <time_unit>
        if (source.peekNextChar() == ':')
        {
            auto saved = source;
            source.skip(); // consume first ':'
            if (source.peekNextChar() == ':')
            {
                source.skip(); // consume second ':'
                juce::String ident;
                while (isIdentBody (source.peekNextChar()))
                    ident += source.nextChar();
                if (timeUnits_.count (ident.toStdString()) > 0)
                    return tokenType_time_literal;
                else
                    source = saved; // restore — not a time literal
            }
            else
            {
                source = saved;
            }
        }

        return isFloat ? tokenType_float : tokenType_integer;
    }

    // Identifiers and keywords
    if (isIdentStart (firstChar))
    {
        juce::String ident;
        while (isIdentBody (source.peekNextChar()))
            ident += source.nextChar();

        std::string s (ident.toStdString());

        if (bkGlobals_.count (s))    return tokenType_bk_global;
        if (keywords_.count (s))     return tokenType_keyword;
        if (builtinUGens_.count (s)) return tokenType_builtin_ugen;
        if (builtinTypes_.count (s)) return tokenType_builtin_type;
        return tokenType_identifier;
    }

    // @ prefix: handles @=> (at-chuck)
    if (firstChar == '@')
    {
        source.skip();
        if (source.peekNextChar() == '=')
        {
            source.skip();
            if (source.peekNextChar() == '>') source.skip();
        }
        return tokenType_operator;
    }

    // = can be ==, =>, =<, or plain =
    if (firstChar == '=')
    {
        source.skip();
        auto next = source.peekNextChar();
        if (next == '=' || next == '>' || next == '<') source.skip();
        return tokenType_operator;
    }

    // + can be +, ++, +=, +=>
    if (firstChar == '+')
    {
        source.skip();
        if (source.peekNextChar() == '+') { source.skip(); return tokenType_operator; }
        if (source.peekNextChar() == '=')
        {
            source.skip();
            if (source.peekNextChar() == '>') source.skip();
        }
        return tokenType_operator;
    }

    // - can be -, --, -=, -=>
    if (firstChar == '-')
    {
        source.skip();
        if (source.peekNextChar() == '-') { source.skip(); return tokenType_operator; }
        if (source.peekNextChar() == '=')
        {
            source.skip();
            if (source.peekNextChar() == '>') source.skip();
        }
        return tokenType_operator;
    }

    // * can be *, *=, *=>
    if (firstChar == '*')
    {
        source.skip();
        if (source.peekNextChar() == '=')
        {
            source.skip();
            if (source.peekNextChar() == '>') source.skip();
        }
        return tokenType_operator;
    }

    // % can be %, %=, %=>
    if (firstChar == '%')
    {
        source.skip();
        if (source.peekNextChar() == '=')
        {
            source.skip();
            if (source.peekNextChar() == '>') source.skip();
        }
        return tokenType_operator;
    }

    // ! can be ! or !=
    if (firstChar == '!')
    {
        source.skip();
        if (source.peekNextChar() == '=') source.skip();
        return tokenType_operator;
    }

    // < can be <, <=, <<, <<<
    if (firstChar == '<')
    {
        source.skip();
        auto next = source.peekNextChar();
        if (next == '=') { source.skip(); return tokenType_operator; }
        if (next == '<')
        {
            source.skip();
            if (source.peekNextChar() == '<') source.skip(); // <<<
        }
        return tokenType_operator;
    }

    // > can be >, >=, >>, >>>
    if (firstChar == '>')
    {
        source.skip();
        auto next = source.peekNextChar();
        if (next == '=') { source.skip(); return tokenType_operator; }
        if (next == '>')
        {
            source.skip();
            if (source.peekNextChar() == '>') source.skip(); // >>>
        }
        return tokenType_operator;
    }

    // & can be & or &&
    if (firstChar == '&')
    {
        source.skip();
        if (source.peekNextChar() == '&') source.skip();
        return tokenType_operator;
    }

    // | can be | or ||
    if (firstChar == '|')
    {
        source.skip();
        if (source.peekNextChar() == '|') source.skip();
        return tokenType_operator;
    }

    // : can be : or ::
    if (firstChar == ':')
    {
        source.skip();
        if (source.peekNextChar() == ':') source.skip();
        return tokenType_operator;
    }

    // All other single-char operators and punctuation
    source.skip();
    return tokenType_operator;
}

juce::CodeEditorComponent::ColourScheme ChucKCodeTokeniser::getDefaultColourScheme()
{
    struct TokenColour { const char* name; juce::uint32 argb; };

    static const TokenColour colours[] =
    {
        { "Error",        0xffcc3333 },  // tokenType_error        — red
        { "Comment",      0xff77a0a0 },  // tokenType_comment      — muted teal
        { "Keyword",      0xffe0a060 },  // tokenType_keyword      — amber
        { "BuiltinType",  0xff70b0e0 },  // tokenType_builtin_type — soft blue
        { "BuiltinUGen",  0xffb090d0 },  // tokenType_builtin_ugen — soft violet
        { "BkGlobal",     0xffe0c080 },  // tokenType_bk_global    — peach
        { "Identifier",   0xffe0e0e0 },  // tokenType_identifier   — near-white
        { "Integer",      0xff90d090 },  // tokenType_integer      — soft green
        { "Float",        0xff90d090 },  // tokenType_float        — soft green
        { "TimeLiteral",  0xff88d0d0 },  // tokenType_time_literal — cyan
        { "String",       0xffb0d090 },  // tokenType_string       — khaki
        { "Operator",     0xffb0b0b0 },  // tokenType_operator     — grey
        { "Whitespace",   0xff000000 },  // tokenType_whitespace   — (unused)
    };

    juce::CodeEditorComponent::ColourScheme cs;
    for (auto& t : colours)
        cs.set (t.name, juce::Colour (t.argb));
    return cs;
}
