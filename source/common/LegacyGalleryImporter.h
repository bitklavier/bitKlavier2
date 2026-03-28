//
// LegacyGalleryImporter.h
// bitKlavier2
//
// Converts legacy bitKlavier 1 gallery XML files (.xml) to the new
// bitKlavier 2 ValueTree format (.bk2).
//
// Design: best-effort / graceful-ignore. Preparation types and attributes
// that are not recognised are silently skipped. This class is intentionally
// self-contained so that new conversion logic can be added by editing a
// single file. Each supported preparation type has its own static method.
//

#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <juce_core/juce_core.h>

class LegacyGalleryImporter
{
public:
    // Main entry point.
    // Returns a valid GALLERY ValueTree on success, or an invalid ValueTree
    // on failure. errorMessage is populated on failure.
    static juce::ValueTree importFromFile (const juce::File& xmlFile,
                                           juce::String& errorMessage);

private:
    // -----------------------------------------------------------------------
    // Per-preparation converters
    // Each receives an XmlElement representing one old-format preparation
    // (e.g. <direct Id="1" name="...">) and returns a new-format ValueTree.
    // Returns an invalid ValueTree if the element should be skipped.
    // -----------------------------------------------------------------------
    static juce::ValueTree convertDirect       (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    static juce::ValueTree convertSynchronic   (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    static juce::ValueTree convertNostalgic    (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    static juce::ValueTree convertTuning       (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    static juce::ValueTree convertTempo        (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    static juce::ValueTree convertKeymap       (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    static juce::ValueTree convertBlendronic   (const juce::XmlElement& el,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex);

    // PianoMap has no gallery-level definition; all info comes from the piano item.
    static juce::ValueTree convertPianoMap     (uint32_t nodeID,
                                                const juce::String& uuid,
                                                int x, int y,
                                                const juce::String& name,
                                                int instanceIndex,
                                                int selectedPianoIndex,
                                                const juce::String& selectedPianoName);

    // Comment has no gallery-level definition; all info comes from the piano item.
    static juce::ValueTree convertComment      (const juce::XmlElement& selfItem,
                                                uint32_t nodeID,
                                                const juce::String& uuid,
                                                const juce::String& name);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Generate a pseudo-random nodeID (large uint32)
    static uint32_t newNodeID();

    // Generate a simple hex UUID string
    static juce::String newUUID();

    // Build a <MODULATABLE_PARAM> ValueTree child
    static juce::ValueTree makeModParam (const juce::String& paramName,
                                         float start, float end,
                                         float skew, float val);

    // Build the default <BUSEQ> ValueTree
    static juce::ValueTree makeDefaultBusEQ();

    // Build the default <BUSCOMPRESSOR> ValueTree
    static juce::ValueTree makeDefaultBusCompressor();

    // Convert an old linear gain (0..N) to dB, clamped to [-80, 6]
    // Old format used linear multipliers; new format uses dB.
    static float gainToDb (float linearGain);

    // Parse a space-separated float list from an XmlElement's indexed attrs
    // e.g. f0="0.1" f1="0.2" ... up to maxCount entries.
    // Returns a space-separated string suitable for the new format.
    static juce::String parseIndexedFloats (const juce::XmlElement& el,
                                             const juce::String& prefix,
                                             int maxCount);

    // Parse indexed booleans (b0, b1, ...) — returns "true"/"false" space list
    static juce::String parseIndexedBools (const juce::XmlElement& el,
                                            const juce::String& prefix,
                                            int maxCount);

    // Old item type codes in the piano layout (from old bitKlavier AudioConstants.h cPreparationIdToType)
    enum OldItemType {
        OldDirect        = 0,
        OldSynchronic    = 1,
        OldNostalgic     = 2,
        OldTuning        = 3,
        OldTempo         = 4,
        OldKeymap        = 5,
        OldDirectMod     = 6,
        OldSynchronicMod = 7,
        OldNostalgicMod  = 8,
        OldTuningMod     = 9,
        OldTempoMod      = 10,
        OldPianoMap      = 12,
        OldReset         = 13,
        OldComment       = 15,
        OldBlendronic    = 16,
        OldResonance     = 18,
    };

    // Maps old type code to new-format element tag name
    static juce::String tagForOldType (int oldType);

    // Maps old type code to new-format "type" attribute value
    static int newTypeForOldType (int oldType);

    // Map old TuningSystem enum value → new TuningSystemNames sequential index
    static int mapOldTuningSystem (int oldScale);

    // Returns true for modification item types (6–10)
    static bool isOldModType (int oldType);

    // Returns the gallery-level element tag for a modification type (e.g. OldTuningMod → "modtuning")
    static juce::String modDataTagForOldType (int oldType);
};
