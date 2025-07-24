//
// Created by Davis Polito on 8/12/24.
//

#include "SampleLoadManager.h"
#include "Synthesiser/Sample.h"
#include "synth_base.h"

/**
 * ******* SampleLoadManager stuff *******
 */

SampleLoadManager::SampleLoadManager (std::shared_ptr<UserPreferencesWrapper> preferences, SynthGuiInterface* synth_,SynthBase* synth) : preferences (preferences),
                                                                                         synth (synth),
                                                                                        synthGui(synth_),
                                                                                         audioFormatManager (new juce::AudioFormatManager())

{
    audioFormatManager->registerBasicFormats();

    // compile list of all string names on the piano A0 through C8
    for (int i = 0; i <= 8; i++)
    {
        for (auto note : allPitchClasses)
        {
            juce::String noteOctave = note + juce::String (i);
            allPitches.add (noteOctave);
        }
    }
    t = synth->getValueTree();

}

SampleLoadManager::~SampleLoadManager()
{
}

void SampleLoadManager::clearAllSamples() {
//    globalHammersSoundset = nullptr;
//    globalPedalsSoundset = nullptr;
//    globalReleaseResonanceSoundset = nullptr;
//    globalSoundset = nullptr;
    samplerSoundset.clear();

}
void SampleLoadManager::handleAsyncUpdate()
{
    if(sampleLoader.getNumJobs() > 0)
        return;
    DBG ("samples loaded, tell the audio thread its okay to change the move soundsets");
    DBG ("samplerSoundset size = " + juce::String (samplerSoundset[globalSoundset_name].size()));
    // synthGui->tryEnqueueProcessorInitQueue([this] {
        t.setProperty(IDs::mainSampleSet,globalSoundset_name ,nullptr);
        t.setProperty(IDs::hammerSampleSet,globalHammersSoundset_name ,nullptr);
        t.setProperty(IDs::releaseResonanceSampleSet,globalReleaseResonanceSoundset_name ,nullptr);
        t.setProperty(IDs::pedalSampleSet,globalPedalsSoundset_name ,nullptr);
    // });
}

// sort array of sample files into arrays of velocities by pitch
// specific to filename format for bK sample libraries
juce::Array<juce::File> SampleLoadManager::samplesByPitch (juce::String whichPitch, juce::Array<juce::File> inFiles)
{
    VelocityComparator vsorter;

    juce::Array<juce::File> outFiles;
    bool sharp = false;

    if (whichPitch.contains ("#"))
        sharp = true;

    for (auto file : inFiles)
    {
        juce::String fileToPlace = file.getFileNameWithoutExtension();
        if (fileToPlace.contains ("harm"))
        {
            fileToPlace = fileToPlace.replace ("harm", "");
            //fileToPlace.removeCharacters("harm");
            //DBG("removing 'harm' from filename, new name = " + fileToPlace);
        }

        if (sharp)
        {
            if (fileToPlace.startsWith (whichPitch))
            {
                outFiles.add (file);
            }
        }
        else
        {
            if (fileToPlace.startsWith (whichPitch) && !fileToPlace.contains ("#"))
            {
                outFiles.add (file);
            }
        }
    }

    // need to sort correctly by velocity....
    outFiles.sort (vsorter);
    return outFiles;
}

/**
SampleLoadManager::getMidiRange
 this looks at the allKeysWithSamples array and determines the start and end points
 needed for each sample to fully cover all the keys.
 the default bK samples are a minor 3rd apart, so it handles assigning to adjacent keys
 but it should work with any randomly spaced set of samples, named appropriately
 need to fully test it though!
 */

juce::BigInteger SampleLoadManager::getMidiRange (juce::String pitchName)
{
    int pitchNum = noteNameToRoot (pitchName);
    int pitchIndex = allKeysWithSamples.indexOf (pitchNum);

    int lowerNeighbor, upperNeighbor;
    int gapDown, gapUp;

    //this is not correct yet for even gaps!

    if (pitchIndex <= 0)
        gapDown = pitchNum;
    else
    {
        lowerNeighbor = allKeysWithSamples.getUnchecked (pitchIndex - 1);
        gapDown = std::trunc ((pitchNum - lowerNeighbor) / 2.);
    }

    if (pitchIndex >= allKeysWithSamples.size() - 1)
        gapUp = 128 - pitchNum;
    else
    {
        upperNeighbor = allKeysWithSamples.getUnchecked (pitchIndex + 1);
        gapUp = std::trunc (((upperNeighbor - pitchNum) / 2.) - 0.1); // subtract something small, so it truncates down when even, but still works when odd...
    }

    juce::BigInteger midirange;
    midirange.setRange (pitchNum - gapDown, gapDown + gapUp + 1, true); // 2nd argument needs to be range, not top val
    //DBG("getMidiRange for " + juce::String(pitchNum) + " start at " + juce::String(pitchNum - gapDown) + " with range " + juce::String(gapDown + gapUp + 1));

    return midirange;
}

void SampleLoadManager::loadSamples_sub(bitklavier::utils::BKPianoSampleType thisSampleType)
{
    using namespace bitklavier::utils;

    // maybe better to do this with templates, but for now...
    juce::String soundsetName;
    if (thisSampleType == BKPianoMain)
        soundsetName = globalSoundset_name;
    else if (thisSampleType == BKPianoHammer)
        soundsetName = globalHammersSoundset_name;
    else if (thisSampleType == BKPianoReleaseResonance)
        soundsetName = globalReleaseResonanceSoundset_name;
    else if (thisSampleType == BKPianoPedal)
        soundsetName = globalPedalsSoundset_name;

    juce::String samplePath = preferences->userPreferences->tree.getProperty("default_sample_path");
    samplePath.append("/Default", 20);
    samplePath.append(BKPianoSampleType_string[thisSampleType], 20); // subfolders need to be named accordingly
    juce::File directory(samplePath);
    juce::Array<juce::File> allSamples = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.wav");

    // sort alphabetically
    MyComparator sorter;
    allSamples.sort(sorter);

    //Build allKeysWithSamples: array that keeps track of which keys have samples, for building start/end ranges in keymap
    for (auto thisFile : allSamples)
    {
        if (thisSampleType == BKPianoMain || thisSampleType == BKPianoReleaseResonance)
        {
            juce::StringArray stringArray;
            stringArray.addTokens( thisFile.getFileName(), "v", "");
            juce::String noteName = stringArray[0].removeCharacters("harm");
            allKeysWithSamples.addIfNotAlreadyThere(noteNameToRoot(noteName));
        }
    }
    allKeysWithSamples.sort();

    // loop through all 12 notes and octaves
    for (auto pitchName : allPitches)
    {
//        DBG ("*****>>>>> LOADING " + juce::String (BKPianoSampleType_string[thisSampleType]) + " " + pitchName + " <<<<<*****");

        juce::Array<juce::File> onePitchSamples (allSamples);
        if (thisSampleType == BKPianoMain || thisSampleType == BKPianoReleaseResonance)
            onePitchSamples = samplesByPitch (pitchName, allSamples);
        else if (thisSampleType == BKPianoHammer)
        {
            onePitchSamples.clear();
            for (auto thisFile : allSamples)
            {
                // isolate MIDI
                juce::StringArray stringArray;
                stringArray.addTokens(thisFile.getFileName(), "l", "");
                int midiNote = stringArray[1].getIntValue() + 20;
                if (midiNote == noteNameToRoot(pitchName))
                {
                    onePitchSamples.add(thisFile);
                    break;
                }
            }
        }
        else if (thisSampleType == BKPianoPedal)
        {
            onePitchSamples.clear();
            for (auto thisFile : allSamples)
            {
                if(thisFile.getFileName().contains("D") && noteNameToRoot(pitchName) == 65)
                {
                    //pedalDown sample, set to 65
                    onePitchSamples.add(thisFile);
                }
                else if(thisFile.getFileName().contains("U") && noteNameToRoot(pitchName) == 66)
                {
                    //pedalDown sample, set to 66
                    onePitchSamples.add(thisFile);
                }
            }
        }

        if (onePitchSamples.size() <= 0) continue; // skip if no samples at this pitch

        juce::BigInteger midirange = getMidiRange(pitchName);


        sampleLoader.addJob(new SampleLoadJob(
                                    thisSampleType,
                                    onePitchSamples.size(),
                                    midirange,
                                    std::make_unique<FileArrayAudioFormatReaderFactory>(onePitchSamples),
                                    audioFormatManager.get(),
                                    &samplerSoundset[soundsetName],
                                    this),
                            true );

    }
}
    bool SampleLoadManager::loadSamples (int selection, bool isGlobal)
    {
        MyComparator sorter;
        juce::String samplePath = preferences->userPreferences->tree.getProperty ("default_sample_path");

        samplePath.append (bitklavier::utils::samplepaths[selection], 10); // change to "orig" to test that way
        //samplePath.append("/orig", 10);
        DBG ("sample path = " + samplePath);
        juce::File directory (samplePath);
        DBG (samplePath);
        juce::Array<juce::File> allSamples = directory.findChildFiles (juce::File::TypesOfFileToFind::findFiles, false, "*.wav");
        allSamples.sort (sorter);
        // look for how many of each type there are: 3 A0 samples, 7 C3 samples, 0 D# samples, 1 C# sample, etc...
        // divide up the velocity range depending on how many there are, and/or using dBFS
        // load accordingly
        //auto r = new FileArrayAudioFormatReaderFactory (allSamples);
        //readerFactory =
        int i = 0;
        for (auto file : allSamples)
        {
            DBG (file.getFullPathName() + " " + juce::String (++i));
        }

        // save this set as the global set
        if (isGlobal)
        {
            globalSoundset_name = bitklavier::utils::samplepaths[selection];
            globalHammersSoundset_name = bitklavier::utils::samplepaths[selection] + "Hammers";
            globalReleaseResonanceSoundset_name = bitklavier::utils::samplepaths[selection] + "ReleaseResonance";
            globalPedalsSoundset_name = bitklavier::utils::samplepaths[selection] + "Pedals";
        }

        loadSamples_sub (bitklavier::utils::BKPianoMain);
        loadSamples_sub (bitklavier::utils::BKPianoHammer);
        loadSamples_sub (bitklavier::utils::BKPianoReleaseResonance);
        loadSamples_sub (bitklavier::utils::BKPianoPedal);
    }

    /**
 * ******* SampleLoadJob Stuff *******
 */

    juce::ThreadPoolJob::JobStatus SampleLoadJob::runJob()
    {

        if(!loadSamples())
        {
            return jobNeedsRunningAgain;
        }
        return jobHasFinished;
    }

    // divide up into velocity layers; create tuples with min/max velocities, based on howmany
    juce::Array<std::tuple<int, int>> SampleLoadJob::getVelLayers (int howmany)
    {
        juce::Array<std::tuple<int, int>> layersToReturn;

        // load from custom velocity layers
        if (howmany == 1)
        {
            layersToReturn.add (bitklavier::utils::VelocityRange<1>().values[0]);
        }
        else if (howmany == 2)
        {
            for (auto vtuple : bitklavier::utils::VelocityRange<2>().values)
            {
                layersToReturn.add (vtuple);
            }
        }
        else if (howmany == 4)
        {
            for (auto vtuple : bitklavier::utils::VelocityRange<4>().values)
            {
                layersToReturn.add (vtuple);
            }
        }
        else if (howmany == 8)
        {
            for (auto vtuple : bitklavier::utils::VelocityRange<8>().values)
            {
                layersToReturn.add (vtuple);
            }
        }
        else //otherwise, divide up the velocity range evenly
        {
            int velThreshold = 0;
            int velIncrement = std::ceil (128. / howmany);

            while (velThreshold <= 128)
            {
                auto vtuple = std::make_tuple (velThreshold, velThreshold + velIncrement);
                velThreshold += velIncrement;
                layersToReturn.add (vtuple);
            }
        }

        /*
    for ( auto vtuple : layersToReturn)
    {
        auto [begin, end] = vtuple;
        DBG("velocity layers min/max = " + juce::String(begin) + "/" + juce::String(end));
    }
     */

        return layersToReturn;
    }

    bool SampleLoadJob::loadSamples()
    {
        using namespace bitklavier::utils;
        DBG ("loadSamples called for " + juce::String (BKPianoSampleType_string[thisSampleType]));

        if (thisSampleType == BKPianoMain)
            return loadMainSamplesByPitch();
        else if (thisSampleType == BKPianoHammer)
            return loadHammerSamples();
        else if (thisSampleType == BKPianoReleaseResonance)
            return loadReleaseResonanceSamples();
        else if (thisSampleType == BKPianoPedal)
            return loadPedalSamples();
    }

    bool SampleLoadJob::loadHammerSamples()
    {
        //for v1 samples, to define bottom threshold.
        //  for hammers, just to define a baseline, since we don't have velocity layers here
        float dBFSBelow = -50.f;

        while (true)
        {
            auto [reader, filename] = sampleReader->make (*manager);
            if (!reader)
                break; // Break the loop if the reader is null

            auto sample = new Sample (*(reader.get()), 90);

            // isolate MIDI
            juce::StringArray stringArray;
            stringArray.addTokens (filename, "l", "");
            int midiNote = stringArray[1].getIntValue() + 20;

//            DBG ("**** loading hammer sample: " + filename + " " + juce::String (midiNote));

            // hammers are only assigned to their one key (rel1 => A0 only, etc...)
            juce::BigInteger midiNoteRange;
            midiNoteRange.setRange (midiNote, 1, true);

            juce::BigInteger velRange;
            velRange.setRange (0, 128, true);

            auto sound = soundset->add (new BKSamplerSound (filename, std::shared_ptr<Sample<juce::AudioFormatReader>> (sample), midiNoteRange, midiNote, 0, velRange, 1, dBFSBelow));
            if (shouldExit())
                return false;
        }
        return true;
        DBG ("done loading hammer samples");
    }

    bool SampleLoadJob::loadReleaseResonanceSamples()
    {
        DBG ("loadReleaseResonanceSamples called");
        auto ranges = getVelLayers (velocityLayers);
        int layers = ranges.size();
        int currentVelLayer = 0;

        //for v1 samples, to define bottom threshold.
        //  quietest Yamaha sample is about -41dbFS, so -50 seems safe and reasonable
        float dBFSBelow = -50.f;

        while (true)
        {
            auto [reader, filename] = sampleReader->make (*manager);
            if (!reader)
                break; // Break the loop if the reader is null

//            DBG ("**** loading resonance sample: " + filename);
            auto sample = new Sample (*(reader.get()), 90);

            juce::StringArray stringArray;
            stringArray.addTokens (filename, "v", "");
            juce::String noteName = stringArray[0].removeCharacters ("harm");
            juce::String velLayer = stringArray[1];
            int midiNote = noteNameToRoot (noteName);

            auto [begin, end] = ranges.getUnchecked (currentVelLayer++);
            juce::BigInteger velRange;
            velRange.setRange (begin, end - begin, true);

            auto sound = soundset->add (new BKSamplerSound (filename, std::shared_ptr<Sample<juce::AudioFormatReader>> (sample), thisMidiRange, midiNote, 0, velRange, layers, dBFSBelow));

            dBFSBelow = sound->dBFSLevel; // to pass on to next sample, which should be the next velocity layer above

//            DBG ("**** loading resonance sample: " + filename);
//            DBG ("");
            if (shouldExit())
                return false;
        }
        return true;
        DBG ("done loading resonance samples, with this many velocity layers " + juce::String (currentVelLayer));
    }

    bool SampleLoadJob::loadPedalSamples()
    {
        //for v1 samples, to define bottom threshold.
        float dBFSBelow = -50.f;

        auto ranges = getVelLayers (velocityLayers);
        int layers = ranges.size();
        int currentVelLayer = 0;

        while (true)
        {
            auto [reader, filename] = sampleReader->make (*manager);
            if (!reader)
                break; // Break the loop if the reader is null

            auto sample = new Sample (*(reader.get()), 90);

            int midiNote;
            if (filename.contains ("D"))
                midiNote = 65;
            else if (filename.contains ("U"))
                midiNote = 66;

//            DBG ("**** loading pedal sample: " + filename + " " + juce::String (midiNote));

            // pedals set to single keys: 65 for pedalDown, 66 for pedalUp
            juce::BigInteger midiNoteRange;
            midiNoteRange.setRange (midiNote, 1, true);

            auto [begin, end] = ranges.getUnchecked (currentVelLayer++);
            juce::BigInteger velRange;
            velRange.setRange (begin, end - begin, true);
//            DBG ("pedal vel range = " + juce::String (begin) + " to " + juce::String (end) + " for " + filename);

            auto sound = soundset->add (new BKSamplerSound (filename, std::shared_ptr<Sample<juce::AudioFormatReader>> (sample), midiNoteRange, midiNote, 0, velRange, 1, dBFSBelow));
            if (shouldExit())
                return false;
        }
        return true;
        DBG ("done loading pedal samples");
    }

    // load all the velocity layers samples for a particular pitch/key
    bool SampleLoadJob::loadMainSamplesByPitch()
    {
        auto ranges = getVelLayers (velocityLayers);
        int layers = ranges.size();
        int currentVelLayer = 0;

        //for v1 samples, to define bottom threshold.
        //  quietest Yamaha sample is about -41dbFS, so -50 seems safe and reasonable
        float dBFSBelow = -50.f;

        while (true)
        {
            auto [reader, filename] = sampleReader->make (*manager);
            if (!reader)
                break; // Break the loop if the reader is null

//            DBG ("**** loading sample: " + filename);
            auto sample = new Sample (*(reader.get()), 90);

            juce::StringArray stringArray;
            stringArray.addTokens (filename, "v", "");
            juce::String noteName = stringArray[0];
            juce::String velLayer = stringArray[1];
            int midiNote = noteNameToRoot (noteName);

            auto [begin, end] = ranges.getUnchecked (currentVelLayer++);
            juce::BigInteger velRange;
            velRange.setRange (begin, end - begin, true);

            auto sound = soundset->add (new BKSamplerSound (filename, std::shared_ptr<Sample<juce::AudioFormatReader>> (sample), thisMidiRange, midiNote, 0, velRange, layers, dBFSBelow));

            dBFSBelow = sound->dBFSLevel; // to pass on to next sample, which should be the next velocity layer above

//            DBG ("**** loading sample: " + filename);
//            DBG ("");
            if (shouldExit())
                return false;
        }
        return true;
        DBG ("done loading string samples");
    }