//
// Created by Dan Trueman on 11/12/24.
//

#ifndef BITKLAVIER2_BKGRAPHICSCONSTANTS_H
#define BITKLAVIER2_BKGRAPHICSCONSTANTS_H

static const float gMainComponentFontSize = 12.0f;

#define DEFAULT_WIDTH 933
#define DEFAULT_HEIGHT 700

#define DEFAULT_MIN_WIDTH 466
#define DEFAULT_MIN_HEIGHT 290


static const int gComponentLeftOffset = 5;
static const int gComponentTopOffset = 5;
static const int gComponentYSpacing = 5;
static const int gComponentXSpacing = 5;

static const int gComponentTextFieldWidth = 80;

static const int gComponentLabelWidth = 160;

static const int fontHeight = 15;

static const int gComponentComboBoxHeight = 24;
static const int gComponentLabelHeight = 17;
static const int gComponentTextFieldHeight = 24;

static const int gComponentRangeSliderHeight = 40;
static const int gComponentSingleSliderHeight = 40;
static const int gComponentStackedSliderHeight = 40;

//static const int gComponentToggleBoxWidth  = gComponentComboBoxHeight;
static const int gComponentToggleBoxHeight  = 24;

static const int gComponentToggleBoxWidth  = gComponentComboBoxHeight;

static const int gComponentSingleSliderXOffset  = 8;

static const int gVCWidth = 375;

static const int gXSpacing = 4;
static const int gYSpacing = 4;

static const std::vector<std::string> intervalNames = {"U", "m2", "M2", "m3", "M3", "P4", "d5", "P5", "m6", "M6", "m7", "M7", "O" };

static const int gPaddingConst = 10;
static const float gModAlpha = 0.4;

static const float gDim = 0.25;
static const float gBright = 1.;
static const float gMedium = 0.65;

#if JUCE_MAC
static const std::string gKeymapShortcut = " (K)";
static const std::string gDirectShortcut = " (D)";
static const std::string gNostalgicShortcut = " (N)";
static const std::string gSynchronicShortcut = " (S)";
static const std::string gBlendronicShortcut = " (B)";
static const std::string gResonanceShortcut = " (R)";
static const std::string gTuningShortcut = " (T)";
static const std::string gTempoShortcut = " (M)";
static const std::string gModificationShortcut = " (C)";
static const std::string gPianoMapShortcut = " (P)";
static const std::string gResetShortcut = " (\\)";
static const std::string gCommentShortcut = " (/)";
static const std::string gSaveShortcut = " (CMD+S)";
static const std::string gSaveAsShortcut = " (CMD+SHIFT+S)";
static const std::string gCopyShortcut = " (CMD+C)";
static const std::string gCutShortcut = " (CMD+X)";
static const std::string gPasteShortcut = " (CMD+V)";
static const std::string gEditShortcut = " (Double Click)";
static const std::string gConnectionShortcut = " (CMD+Click)";
static const std::string gConnectAllShortcut = " (U)";
static const std::string gDisconnectFromShortcut = " (O)";
static const std::string gDisconnectBetweenShortcut = " (I)";
static const std::string gAlignShortcut = " (CMD+Arrow)";
static const std::string gMidiEditShortcut = " (CMD+E)";
static const std::string gUndoShortcut = " (CMD+Z)";
static const std::string gRedoShortcut = " (CMD+SHIFT+Z)";
static const std::string gAllOffShortcut = " (Space)";
#elif JUCE_WINDOWS || JUCE_LINUX
static const std::string gKeymapShortcut = " (K)";
static const std::string gDirectShortcut = " (D)";
static const std::string gNostalgicShortcut = " (N)";
static const std::string gSynchronicShortcut = " (S)";
static const std::string gBlendronicShortcut = " (B)";
static const std::string gResonanceShortcut = " (R)";
static const std::string gTuningShortcut = " (T)";
static const std::string gTempoShortcut = " (M)";
static const std::string gModificationShortcut = " (C)";
static const std::string gPianoMapShortcut = " (P)";
static const std::string gResetShortcut = " (\\)";
static const std::string gCommentShortcut = " (/)";
static const std::string gSaveShortcut = " (CTRL+S)";
static const std::string gSaveAsShortcut = " (CTRL+SHIFT+S)";
static const std::string gCopyShortcut = " (CTRL+C)";
static const std::string gCutShortcut = " (CTRL+X)";
static const std::string gPasteShortcut = " (CTRL+V)";
static const std::string gEditShortcut = " (Double Click)";
static const std::string gConnectionShortcut = " (CTRL+Click)";
static const std::string gConnectAllShortcut = " (U)";
static const std::string gDisconnectFromShortcut = " (O)";
static const std::string gDisconnectBetweenShortcut = " (I)";
static const std::string gAlignShortcut = " (CTRL+Arrow)";
static const std::string gMidiEditShortcut = " (CTRL+E)";
static const std::string gUndoShortcut = " (CTRL+Z)";
static const std::string gRedoShortcut = " (CTRL+SHIFT+Z)";
static const std::string gAllOffShortcut = " (Space)";
#elif JUCE_IOS
static const std::string gKeymapShortcut = "";
static const std::string gDirectShortcut = "";
static const std::string gNostalgicShortcut = "";
static const std::string gSynchronicShortcut = "";
static const std::string gBlendronicShortcut = "";
static const std::string gResonanceShortcut = "";
static const std::string gTuningShortcut = "";
static const std::string gTempoShortcut = "";
static const std::string gModificationShortcut = "";
static const std::string gPianoMapShortcut = "";
static const std::string gResetShortcut = "";
static const std::string gCommentShortcut = "";
static const std::string gSaveShortcut = "";
static const std::string gSaveAsShortcut = "";
static const std::string gCopyShortcut = "";
static const std::string gCutShortcut = "";
static const std::string gPasteShortcut = "";
static const std::string gEditShortcut = " (Double Tap)";
static const std::string gConnectionShortcut = " (Hold)";
static const std::string gConnectAllShortcut = "";
static const std::string gDisconnectFromShortcut = "";
static const std::string gDisconnectBetweenShortcut = "";
static const std::string gAlignShortcut = "";
static const std::string gMidiEditShortcut = "";
static const std::string gUndoShortcut = "";
static const std::string gRedoShortcut = "";
static const std::string gAllOffShortcut = "";
#endif

#endif //BITKLAVIER2_BKGRAPHICSCONSTANTS_H
