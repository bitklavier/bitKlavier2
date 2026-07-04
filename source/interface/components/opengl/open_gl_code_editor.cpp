// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "open_gl_code_editor.h"
#include "ChucKCodeTokeniser.h"

#include <vector>

// ─── helpers ─────────────────────────────────────────────────────────────────

static juce::String leadingWhitespace (const juce::String& line)
{
    int end = 0;
    while (end < line.length() && (line[end] == ' ' || line[end] == '\t'))
        ++end;
    return line.substring (0, end);
}

static bool isBraceChar (juce::juce_wchar ch)
{
    return ch == '{' || ch == '}' || ch == '(' || ch == ')' || ch == '[' || ch == ']';
}

// ─── handleReturnKey ─────────────────────────────────────────────────────────

void OpenGlCodeEditor::handleReturnKey()
{
    if (isReadOnly() || ! getHighlightedRegion().isEmpty())
    {
        juce::CodeEditorComponent::handleReturnKey();
        return;
    }

    auto pos = getCaretPos();
    auto lineText = pos.getLineText();
    auto beforeCaret = lineText.substring (0, pos.getIndexInLine());

    // Carry the current line's leading whitespace to the new line.
    juce::String indent = leadingWhitespace (beforeCaret);

    // If the last non-whitespace char before the caret is an opener, add one indent.
    auto trimmed = beforeCaret.trimEnd();
    if (trimmed.isNotEmpty())
    {
        auto lastCh = trimmed[trimmed.length() - 1];
        if (lastCh == '{' || lastCh == '(' || lastCh == '[')
        {
            indent += areSpacesInsertedForTabs() ? getTabString (getTabSize())
                                                 : juce::String ("\t");
        }
    }

    juce::CodeEditorComponent::insertTextAtCaret (getDocument().getNewLineCharacters() + indent);
}

// ─── insertTextAtCaret (auto-dedent on '}') ──────────────────────────────────

void OpenGlCodeEditor::insertTextAtCaret (const juce::String& textToInsert)
{
    // Auto-dedent: intercept lone '}' typed as the first non-whitespace on the line.
    if (! isReadOnly()
        && textToInsert == "}"
        && getHighlightedRegion().isEmpty())
    {
        auto caret = getCaretPos();
        auto lineUpToCaret = caret.getLineText().substring (0, caret.getIndexInLine());

        if (lineUpToCaret.trim().isEmpty())  // only whitespace (or nothing) before caret
        {
            int matchIdx = findMatchingBrace (caret.getPosition(), '}');
            if (matchIdx >= 0)
            {
                juce::CodeDocument::Position matchPos (getDocument(), matchIdx);
                juce::String matchIndent = leadingWhitespace (matchPos.getLineText());

                // Select the whitespace on the current line, then replace with
                // the matching opener's indent + '}'. Full qualification prevents recursion.
                auto lineStartPos = caret.getPosition() - caret.getIndexInLine();
                juce::CodeDocument::Position lineStart (getDocument(), lineStartPos);
                selectRegion (lineStart, caret);
                juce::CodeEditorComponent::insertTextAtCaret (matchIndent + "}");
                return;
            }
        }
    }

    juce::CodeEditorComponent::insertTextAtCaret (textToInsert);
}

// ─── keyPressed (smart backspace) ────────────────────────────────────────────

bool OpenGlCodeEditor::keyPressed (const juce::KeyPress& key)
{
    if (! isReadOnly() && key == juce::KeyPress::backspaceKey && trySmartBackspace())
        return true;

    return juce::CodeEditorComponent::keyPressed (key);
}

bool OpenGlCodeEditor::trySmartBackspace()
{
    if (! getHighlightedRegion().isEmpty())
        return false;

    auto caret = getCaretPos();
    if (caret.getIndexInLine() == 0)
        return false;

    // Only apply inside pure leading whitespace.
    auto lineUpToCaret = caret.getLineText().substring (0, caret.getIndexInLine());
    if (lineUpToCaret.trim().isNotEmpty())
        return false;

    if (areSpacesInsertedForTabs())
    {
        // Delete back to the previous tab-stop column.
        int col = caret.getIndexInLine();
        int tabSize = getTabSize();
        int deleteCount = (col % tabSize == 0) ? tabSize : (col % tabSize);
        if (deleteCount > col) deleteCount = col;
        for (int i = 0; i < deleteCount; ++i)
            deleteBackwards (false);
    }
    else
    {
        // Hard tab: delete the single '\t' immediately to the left.
        deleteBackwards (false);
    }
    return true;
}

// ─── caretPositionMoved / brace matching ─────────────────────────────────────

void OpenGlCodeEditor::caretPositionMoved()
{
    juce::CodeEditorComponent::caretPositionMoved();
    updateBraceMatch();
}

void OpenGlCodeEditor::updateBraceMatch()
{
    braceMatchValid_ = false;

    if (tokeniser_ == nullptr)
        return;

    auto caret = getCaretPos();
    auto& doc = getDocument();

    // Prefer the character immediately LEFT of the caret (more natural after typing).
    juce::juce_wchar anchor = 0;
    int anchorIdx = -1;

    if (caret.getPosition() > 0)
    {
        juce::CodeDocument::Position before (doc, caret.getPosition() - 1);
        auto ch = before.getCharacter();
        if (isBraceChar (ch)) { anchor = ch; anchorIdx = caret.getPosition() - 1; }
    }

    // Fall back to the character AT the caret.
    if (anchorIdx < 0)
    {
        auto ch = caret.getCharacter();
        if (isBraceChar (ch)) { anchor = ch; anchorIdx = caret.getPosition(); }
    }

    if (anchorIdx < 0)
        return;

    int matchIdx = findMatchingBrace (anchorIdx, anchor);
    if (matchIdx < 0)
        return;

    braceA_ = juce::CodeDocument::Position (doc, anchorIdx);
    braceB_ = juce::CodeDocument::Position (doc, matchIdx);
    braceMatchValid_ = true;

    // Dirty only the two character cells so the 30 Hz timer captures the overlay.
    repaint (getCharacterBounds (braceA_).getUnion (getCharacterBounds (braceB_)));
}

int OpenGlCodeEditor::findMatchingBrace (int anchorIdx, juce::juce_wchar anchorChar) const
{
    if (tokeniser_ == nullptr)
        return -1;

    const bool anchorIsOpener = (anchorChar == '{' || anchorChar == '(' || anchorChar == '[');
    juce::juce_wchar opener, closer;

    if (anchorIsOpener)
    {
        opener = anchorChar;
        closer = (anchorChar == '{') ? '}' : (anchorChar == '(') ? ')' : ']';
    }
    else
    {
        closer = anchorChar;
        opener = (anchorChar == '}') ? '{' : (anchorChar == ')') ? '(' : '[';
    }

    auto& doc = getDocument();

    // First pass: collect string/comment skip ranges via the tokeniser.
    // This reuses ChucKCodeTokeniser's existing state machine for strings
    // and both // and /* */ comments, avoiding duplication.
    struct SkipRange { int start, end; };
    std::vector<SkipRange> skips;
    {
        juce::CodeDocument::Iterator it (doc);
        while (! it.isEOF())
        {
            int ts = it.getPosition();
            int tt = tokeniser_->readNextToken (it);
            int te = it.getPosition();
            if (te <= ts) break;  // tokeniser didn't advance (e.g. null-terminator of last line); exit
            if (tt == ChucKCodeTokeniser::tokenType_string ||
                tt == ChucKCodeTokeniser::tokenType_comment)
                skips.push_back ({ ts, te });
        }
    }

    // Second pass: scan characters, skipping strings and comments.
    std::vector<int> openerStack;  // for closer-anchor: stack of unmatched openers
    bool foundAnchor = false;
    int depth = 0;
    size_t skipIdx = 0;

    juce::CodeDocument::Iterator it (doc);
    while (! it.isEOF())
    {
        int pos = it.getPosition();

        // Advance past skip ranges that have ended.
        while (skipIdx < skips.size() && skips[skipIdx].end <= pos)
            ++skipIdx;

        auto ch = it.nextChar();

        bool inSkip = (skipIdx < skips.size() &&
                       pos >= skips[skipIdx].start &&
                       pos < skips[skipIdx].end);
        if (inSkip)
            continue;

        if (anchorIsOpener)
        {
            if (ch == opener)
            {
                if (pos == anchorIdx) { foundAnchor = true; depth = 1; }
                else if (foundAnchor)  { ++depth; }
            }
            else if (ch == closer && foundAnchor)
            {
                if (--depth == 0)
                    return pos;
            }
        }
        else  // anchor is a closer — find the matching opener
        {
            if (ch == opener)
            {
                openerStack.push_back (pos);
            }
            else if (ch == closer)
            {
                if (pos == anchorIdx)
                    return openerStack.empty() ? -1 : openerStack.back();

                if (! openerStack.empty())
                    openerStack.pop_back();
            }
        }
    }

    return -1;
}

// ─── paint (brace-match overlay) ─────────────────────────────────────────────

void OpenGlCodeEditor::paint (juce::Graphics& g)
{
    juce::CodeEditorComponent::paint (g);
    if (braceMatchValid_)
        drawBraceMatchOverlay (g);
}

void OpenGlCodeEditor::drawBraceMatchOverlay (juce::Graphics& g)
{
    auto colour = findColour (juce::CodeEditorComponent::defaultTextColourId).withAlpha (0.4f);
    g.setColour (colour);
    g.drawRect (getCharacterBounds (braceA_), 1);
    g.drawRect (getCharacterBounds (braceB_), 1);
}
