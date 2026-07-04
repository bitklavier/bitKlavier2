// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "open_gl_code_editor.h"
#include "ChucKCodeTokeniser.h"

#include <vector>

// ─── static member definitions ───────────────────────────────────────────────

std::atomic<float> OpenGlCodeEditor::s_globalFontSize_ { 0.0f };
juce::Array<OpenGlCodeEditor*, juce::CriticalSection> OpenGlCodeEditor::s_liveEditors_;

void OpenGlCodeEditor::setFontSize (float newSize)
{
    newSize = juce::jlimit (8.0f, 72.0f, newSize);
    setFont (getFont().withHeight (newSize));
    s_globalFontSize_.store (newSize);
    broadcastFontSize (newSize);
}

void OpenGlCodeEditor::broadcastFontSize (float newSize)
{
    // Must be called on the message thread. Iterate a snapshot to avoid
    // re-entrancy if setFont somehow triggers a structural change.
    auto snapshot = s_liveEditors_;
    for (auto* ed : snapshot)
        if (ed->getFontSize() != newSize)
            ed->setFont (ed->getFont().withHeight (newSize));
}

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

// ─── keyPressed (zoom + find shortcuts + smart backspace) ────────────────────

bool OpenGlCodeEditor::keyPressed (const juce::KeyPress& key)
{
    // Zoom shortcuts — Cmd+= zoom in, Cmd+- zoom out, Cmd+0 reset.
    // These work in both editable and read-only modes (e.g. the console).
    if (key.getModifiers().isCommandDown() && ! key.getModifiers().isAltDown()
        && ! key.getModifiers().isShiftDown())
    {
        const int ch = key.getKeyCode();
        if (ch == '=' || ch == '+')  { setFontSize (getFontSize() + 1.0f); return true; }
        if (ch == '-')               { setFontSize (getFontSize() - 1.0f); return true; }
        if (ch == '0')               { setFontSize (defaultFontSize_);     return true; }
    }

    // Find shortcuts — Cmd+F open bar, Cmd+G next, Shift+Cmd+G previous.
    // Check both lower and upper since JUCE/macOS may report either for letter+modifier combos.
    if (key.getModifiers().isCommandDown() && ! key.getModifiers().isAltDown())
    {
        const int ch = key.getKeyCode();
        const bool shift = key.getModifiers().isShiftDown();
        if (! shift && (ch == 'f' || ch == 'F')) { if (onFindShortcut)     onFindShortcut();     return true; }
        if (! shift && (ch == 'g' || ch == 'G')) { if (onFindNextShortcut) onFindNextShortcut(); return true; }
        if (  shift && (ch == 'g' || ch == 'G')) { if (onFindPrevShortcut) onFindPrevShortcut(); return true; }
    }

    if (! isReadOnly() && key == juce::KeyPress::backspaceKey && trySmartBackspace())
        return true;

    return juce::CodeEditorComponent::keyPressed (key);
}

// ─── findNext / findPrev ─────────────────────────────────────────────────────

bool OpenGlCodeEditor::findNext (const juce::String& needle, bool caseSensitive)
{
    if (needle.isEmpty())
        return false;

    auto& doc    = getDocument();
    auto content = doc.getAllContent();
    int docLen   = content.length();
    int needleLen = needle.length();

    // Start from the end of the current selection (or caret) to advance past current match.
    int startPos = getHighlightedRegion().isEmpty() ? getCaretPos().getPosition()
                                                    : getHighlightedRegion().getEnd();

    int found = caseSensitive ? content.indexOf (startPos, needle)
                              : content.indexOfIgnoreCase (startPos, needle);

    if (found < 0 && startPos > 0)  // wrap around
        found = caseSensitive ? content.indexOf (0, needle)
                              : content.indexOfIgnoreCase (0, needle);

    if (found < 0 || found + needleLen > docLen)
        return false;

    setHighlightedRegion (juce::Range<int> (found, found + needleLen));
    int line = juce::CodeDocument::Position (doc, found).getLineNumber();
    scrollToLine (line);
    juce::MessageManager::callAsync ([this] { redoImage(); });
    return true;
}

bool OpenGlCodeEditor::findPrev (const juce::String& needle, bool caseSensitive)
{
    if (needle.isEmpty())
        return false;

    auto& doc     = getDocument();
    auto content  = doc.getAllContent();
    int docLen    = content.length();
    int needleLen = needle.length();

    // Start from the beginning of the current selection so we move past it backwards.
    int startPos = getHighlightedRegion().isEmpty() ? getCaretPos().getPosition()
                                                    : getHighlightedRegion().getStart();

    auto searchIn = content.substring (0, juce::jmax (0, startPos));
    int found = caseSensitive ? searchIn.lastIndexOf (needle)
                              : searchIn.lastIndexOfIgnoreCase (needle);

    if (found < 0)  // wrap around — search entire document
        found = caseSensitive ? content.lastIndexOf (needle)
                              : content.lastIndexOfIgnoreCase (needle);

    if (found < 0 || found + needleLen > docLen)
        return false;

    setHighlightedRegion (juce::Range<int> (found, found + needleLen));
    int line = juce::CodeDocument::Position (doc, found).getLineNumber();
    scrollToLine (line);
    juce::MessageManager::callAsync ([this] { redoImage(); });
    return true;
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
