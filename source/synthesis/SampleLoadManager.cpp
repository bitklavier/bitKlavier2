//
// Created by Davis Polito on 8/12/24.
//

#include "SampleLoadManager.h"
#include "Synthesiser/Sample.h"
#include "synth_base.h"

/**
 * ******* SampleLoadManager stuff *******
 */
#include "SFZSound.h"
#include "SF2Sound.h"
#include "SFZSample.h"

SampleLoadManager::SampleLoadManager (SynthBase* parent, std::shared_ptr<UserPreferencesWrapper> preferences) : preferences (preferences),
                                                                                                                audioFormatManager (new juce::AudioFormatManager()),parent(parent)

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
}

SampleLoadManager::~SampleLoadManager()
{
    for (auto [_, samples]: samplerSoundset) {
            samples.clear();
    }
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
    if (sampleLoader.getNumJobs() > 0)
        return;

    DBG ("All samples loaded. Audio thread can safely switch soundsets.");

    for (auto& [name, progress] : soundsetProgressMap)
    {
        if (progress->completedJobs == progress->totalJobs)
        {
            DBG ("Soundset " + name + " finished loading.");
            progress->currentProgress = 1.0f;
        }
    }
    parent->finishedSampleLoading();
}

// sort array of sample files into arrays of velocities by pitch
// specific to filename format for bK sample libraries
juce::Array<juce::File> SampleLoadManager::samplesByPitch (juce::String whichPitch, juce::Array<juce::File> inFiles)
{
    VelocityComparator vsorter;

    juce::Array<juce::File> outFiles;
    bool sharp = false;
    bool flat = false;

    if (whichPitch.contains ("#"))
        sharp = true;
    if (whichPitch.contains ("b"))
        flat = true;

    for (auto file : inFiles)
    {
        juce::String fileToPlace = file.getFileNameWithoutExtension();
        if (fileToPlace.contains ("harm"))
        {
            fileToPlace = fileToPlace.replace ("harm", "");
            //fileToPlace.removeCharacters("harm");
            //DBG("removing 'harm' from filename, new name = " + fileToPlace);
        }

        if (sharp || flat)
        {
            if (fileToPlace.startsWith (whichPitch))
            {
                outFiles.add (file);
            }
        }
        else
        {
            if (fileToPlace.startsWith (whichPitch) && !fileToPlace.contains ("#") && !fileToPlace.contains ("b"))
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
const std::vector<std::string> SampleLoadManager::getAllSampleSets()
{
    std::vector<std::string> sampleSets;

    juce::String samplePath = preferences->userPreferences->tree.getProperty("default_sample_path");
    juce::File baseDir(samplePath);

    if (baseDir.isDirectory())
    {
        // 1. Directories
        juce::Array<juce::File> dirs = baseDir.findChildFiles(
            juce::File::findDirectories,
            false
        );

        for (auto& d : dirs)
            sampleSets.push_back(d.getFileName().toStdString());

        // 2. .sf2 and .sfz files
        juce::Array<juce::File> sfFiles = baseDir.findChildFiles(
            juce::File::findFiles,
            false,
            "*.sf2;*.sfz" // JUCE supports ";"-separated patterns
        );

        for (auto& f : sfFiles)
            sampleSets.push_back(f.getFileName().toStdString());
    }
    return sampleSets;
}

bool SampleLoadManager::loadSamples(std::string sample_name) {
    // juce::String samplePath = preferences->userPreferences->tree.getProperty ("default_sample_path");
    DBG ("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);

    MyComparator sorter;
    // samplePath.append (sample_name, 1000); // change to "orig" to test that way
    juce::String samplePath = preferences->userPreferences->tree.getProperty ("default_sample_path");

    juce::File baseDir (
        preferences->userPreferences->tree.getProperty ("default_sample_path").toString()
    );

    juce::File directory = baseDir.getChildFile (sample_name);
    //samplePath.append("/orig", 10);
    DBG ("sample path = " + directory.getFullPathName());
    // juce::File directory (samplePath);
    // DBG (samplePath);
    juce::Array<juce::File> allSamples = directory.findChildFiles (juce::File::TypesOfFileToFind::findFiles, false, "*.wav");
    allSamples.sort (sorter);

    // look for how many of each type there are: 3 A0 samples, 7 C3 samples, 0 D# samples, 1 C# sample, etc...
    // divide up the velocity range depending on how many there are, and/or using dBFS
    // load accordingly
    int i = 0;
    for (auto file : allSamples)
    {
        DBG (file.getFullPathName() + " " + juce::String (++i));
    }
    std::shared_ptr<SampleSetProgress> progress;

    // Create or fetch the progress entry
    if (!soundsetProgressMap.contains (sample_name))
        soundsetProgressMap[sample_name] = std::make_shared<SampleSetProgress>();

    progress = soundsetProgressMap[sample_name];

    // Attach ValueTree + property info for later completion
    progress->soundsetName = sample_name;
    progress->targetTree = juce::ValueTree{};

    const juce::String ext = directory.getFileExtension().toLowerCase();
    if(ext == ".sfz" || ext == ".sf2") {
        loadSamples_sub(bitklavier::utils::BKPianoSoundFont,sample_name);
        return true;
    }

    loadSamples_sub (bitklavier::utils::BKPianoMain,sample_name);
    loadSamples_sub (bitklavier::utils::BKPianoHammer,sample_name);
    loadSamples_sub (bitklavier::utils::BKPianoReleaseResonance,sample_name);
    loadSamples_sub (bitklavier::utils::BKPianoPedal,sample_name);
    return true;
}
// Helper: decide if this tree represents the "global/gallery" target.
static bool isGalleryTree (const juce::ValueTree& vt)
{
    return vt.isValid() && vt.hasType (IDs::GALLERY); // change IDs::gallery if needed
}

bool SampleLoadManager::loadSamples (const juce::String& soundsetName,
                                     const juce::ValueTree& targetTree)
{
    DBG ("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);

    allKeysWithSamples.clear();

    // Decide where the soundset property should be written:
    // - Gallery/global -> write into `t` (your global ValueTree)
    // - otherwise -> write into the passed-in tree
    const bool targetIsGallery = isGalleryTree (targetTree);

    juce::ValueTree vtToWrite = targetIsGallery ? t : targetTree; // NOTE: both are handles

    // If already loaded, just set the property and return
    // (Adjust this condition if your samplerSoundset keys are not exactly the base name.)
    if (samplerSoundset.contains (soundsetName.toStdString()))
    {
        if (vtToWrite.isValid())
            vtToWrite.setProperty (IDs::soundset, soundsetName, nullptr);

        return true;
    }

    // Build directory for this soundset
    juce::String samplePath = preferences->userPreferences->tree.getProperty ("default_sample_path");

    juce::File baseDir (
        preferences->userPreferences->tree.getProperty ("default_sample_path").toString()
    );

    juce::File directory = baseDir.getChildFile (soundsetName);
    DBG ("sample path = " + directory.getFullPathName());


    if (! directory.exists())
    {
        DBG ("Soundset dir does not exist: " + directory.getFullPathName());
        return false;
    }

    // (Optional debug listing)
    {
        MyComparator sorter;
        auto allSamples = directory.findChildFiles (juce::File::findFiles, false, "*.wav");
        allSamples.sort (sorter);

        int i = 0;
        for (auto file : allSamples)
            DBG (file.getFullPathName() + " " + juce::String (++i));
    }

    // Progress entry
    auto& progressPtr = soundsetProgressMap[soundsetName.toStdString()];
    if (!progressPtr)
        progressPtr = std::make_shared<SampleSetProgress>();

    progressPtr->soundsetName = soundsetName.toStdString();
    progressPtr->targetTree   = vtToWrite; // store where to write soundset / completion callbacks

    parent->startSampleLoading();

    const juce::String ext = directory.getFileExtension().toLowerCase();
    if (ext == ".sfz" || ext == ".sf2")
    {
        loadSamples_sub (bitklavier::utils::BKPianoSoundFont, soundsetName.toStdString());
        return true;
    }

    loadSamples_sub (bitklavier::utils::BKPianoMain,             soundsetName.toStdString());
    loadSamples_sub (bitklavier::utils::BKPianoHammer,           soundsetName.toStdString());
    loadSamples_sub (bitklavier::utils::BKPianoReleaseResonance, soundsetName.toStdString());
    loadSamples_sub (bitklavier::utils::BKPianoPedal,            soundsetName.toStdString());
    return true;
}

bool SampleLoadManager::loadSamples (int selection, bool isGlobal, const juce::ValueTree& prep_tree)
{
    DBG ("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);

    allKeysWithSamples.clear();

    temp_prep_tree = prep_tree;
    MyComparator sorter;
    juce::String samplePath = preferences->userPreferences->tree.getProperty ("default_sample_path");
    auto soundsets = getAllSampleSets();
    // save this set as the global set
    if (isGlobal)
    {
        if(samplerSoundset.contains(soundsets[selection])) {
            t.setProperty(IDs::soundset,juce::String(soundsets[selection]),nullptr);
            return true;
        }
    }
    if(temp_prep_tree.isValid()) {
        if(samplerSoundset.contains(soundsets[selection]) ) {
            temp_prep_tree.setProperty(IDs::soundset,juce::String(soundsets[selection]) ,nullptr);
            return true;
        }
    }
 // juce::String samplePath = preferences->userPreferences->tree.getProperty ("default_sample_path");

    juce::File baseDir (
        preferences->userPreferences->tree.getProperty ("default_sample_path").toString()
    );

    juce::File directory = baseDir.getChildFile (soundsets[selection]);
    // samplePath.append (soundsets[selection], 1000); // change to "orig" to test that way
    //samplePath.append("/orig", 10);
    DBG ("sample path = " + directory.getFullPathName());
    // juce::File directory (samplePath);
    // DBG (samplePath);
    juce::Array<juce::File> allSamples = directory.findChildFiles (juce::File::TypesOfFileToFind::findFiles, false, "*.wav");
    allSamples.sort (sorter);

    // look for how many of each type there are: 3 A0 samples, 7 C3 samples, 0 D# samples, 1 C# sample, etc...
    // divide up the velocity range depending on how many there are, and/or using dBFS
    // load accordingly
    int i = 0;
    for (auto file : allSamples)
    {
        DBG (file.getFullPathName() + " " + juce::String (++i));
    }

    std::shared_ptr<SampleSetProgress> progress;

    // Create or fetch the progress entry
    if (!soundsetProgressMap.contains (soundsets[selection]))
        soundsetProgressMap[soundsets[selection]] = std::make_shared<SampleSetProgress>();

    progress = soundsetProgressMap[soundsets[selection]];

    // Attach ValueTree + property info for later completion
    progress->soundsetName =soundsets[selection];
    progress->targetTree = prep_tree;

    parent->startSampleLoading();
    const juce::String ext = directory.getFileExtension().toLowerCase();
    if(ext == ".sfz" || ext == ".sf2") {
        loadSamples_sub(bitklavier::utils::BKPianoSoundFont,soundsets[selection]);
        return true;
    }
    loadSamples_sub (bitklavier::utils::BKPianoMain,soundsets[selection]);
    loadSamples_sub (bitklavier::utils::BKPianoHammer,soundsets[selection]);
    loadSamples_sub (bitklavier::utils::BKPianoReleaseResonance, soundsets[selection]);
    loadSamples_sub (bitklavier::utils::BKPianoPedal,soundsets[selection]);
    return true;
}

void SampleLoadManager::loadSamples_sub(bitklavier::utils::BKPianoSampleType thisSampleType, std::string name)
{
    using namespace bitklavier::utils;
    // maybe better to do this with templates, but for now...
    juce::String soundsetName = name;
    if (thisSampleType == BKPianoMain)
        soundsetName = name;
    else if (thisSampleType == BKPianoHammer)
        soundsetName = name + "Hammers";
    else if (thisSampleType == BKPianoReleaseResonance)
        soundsetName = name+ "ReleaseResonance";
    else if (thisSampleType == BKPianoPedal)
        soundsetName = name + "Pedals";

    juce::String samplePath = preferences->userPreferences->tree.getProperty("default_sample_path");
    samplePath.append("/" + juce::String(name), 1000);
    samplePath.append(BKPianoSampleType_string[thisSampleType], 1000); // subfolders need to be named accordingly
    juce::File directory(samplePath);
    juce::Array<juce::File> allSamples = directory.findChildFiles(juce::File::TypesOfFileToFind::findFiles, false, "*.wav");
    if(thisSampleType == BKPianoSoundFont) {
        auto progressPtr = soundsetProgressMap[name];
        if (!progressPtr)
            progressPtr = soundsetProgressMap[name] = std::make_shared<SampleSetProgress>();
        progressPtr->totalJobs++; // one job per sample type
        sampleLoader.addJob(new SampleLoadJob(directory,
                                 &samplerSoundset[soundsetName],
                                 this,progressPtr,
                                *this), true);
        return;
    }

    // sort alphabetically
    MyComparator sorter;
    allSamples.sort(sorter);
    if(allSamples.isEmpty())
        return;
    auto progressPtr = soundsetProgressMap[name];
    if (!progressPtr)
        progressPtr = soundsetProgressMap[name] = std::make_shared<SampleSetProgress>();

    //Build allKeysWithSamples: array that keeps track of which keys have samples, for building start/end ranges in keymap
    allKeysWithSamples.clear();
    for (auto thisFile : allSamples)
    {
        if (thisSampleType == BKPianoMain || thisSampleType == BKPianoReleaseResonance)
        {
            juce::StringArray stringArray;
            stringArray.addTokens( thisFile.getFileName(), "v", "");
            juce::String noteName = stringArray[0].removeCharacters("harm");
            if (allKeysWithSamples.addIfNotAlreadyThere(noteNameToRoot(noteName))) {
                DBG(noteName + " " + juce::String(noteNameToRoot(noteName)));
            }
        }
    }
    allKeysWithSamples.sort();

    std::vector<PitchSamplesInfo> pitchesVector;

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

        /**
         * make a struct that holds the first four arguments below
         * make an array of that struct, fill it with each "pitch" below; don't addJob
         * then, outside of this for loop, add the job for the array of structs,
         * - so that the job does all the pitches, instead of doing them one by one
         *
         * SampleLoadJob will need a new constructor that takes the array of structs, and a new runJob
         */

//        sampleLoader.addJob(new SampleLoadJob(
//                                    thisSampleType,
//                                    onePitchSamples.size(),
//                                    midirange,
//                                    std::make_unique<FileArrayAudioFormatReaderFactory>(onePitchSamples),
//                                    audioFormatManager.get(),
//                                    &samplerSoundset[soundsetName],
//                                    this),
//                            true );

        PitchSamplesInfo newPitchSamples;
        newPitchSamples.sampleType = thisSampleType;
        newPitchSamples.numLayers = onePitchSamples.size();
        newPitchSamples.newMidiRange = midirange;
        newPitchSamples.sampleReaderArray = std::make_unique<FileArrayAudioFormatReaderFactory>(onePitchSamples);
        pitchesVector.push_back(std::move(newPitchSamples));
    }

    int totalUnits = 0;
    for (auto& ps : pitchesVector)
        totalUnits += std::max(1, ps.numLayers);

    progressPtr->totalJobs++; // one job per sample type
    sampleLoader.addJob(new SampleLoadJob(
                             audioFormatManager.get(),
                             std::move(pitchesVector),
                             &samplerSoundset[soundsetName],
                             this,progressPtr,
                            totalUnits,*this), true);
}



/**
* ******* SampleLoadJob Stuff *******
*/

juce::ThreadPoolJob::JobStatus SampleLoadJob::runJob()
{
    /**
     * here, have a for loop that runs through the array of structs
     * and calls loadSamples for each, setting
     * - thisSampleType = sampleType;
     * - velocityLayers = numLayers;
     * - thisMidiRange = newMidiRange;
     * beforehand
     */

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
    bool loadSuccess = false;
    if (sampleReaderVector.empty() && sfzFile.exists())
        return loadSoundFont(sfzFile);
    for (int i = continueValue; i < sampleReaderVector.size(); i ++)

    {
        auto &samplePitch = sampleReaderVector[i];
        thisSampleType = samplePitch.sampleType;
        velocityLayers = samplePitch.numLayers;
        thisMidiRange = samplePitch.newMidiRange;
        velLayerContinue = 0;
        dbfsBelowContinue = -50.f;
        // int start = thisMidiRange.findNextSetBit (0);   // first (lowest) bit that’s set
        // int end   = thisMidiRange.getHighestBit();      // last (highest) bit that’s set
        //
        // DBG ("BigInteger range: start = " + juce::String (start)
        //      + ", end = " + juce::String (end)
        //      + ", length = " + juce::String (end - start + 1));
        sampleReader = std::move(samplePitch.sampleReaderArray);

        //DBG ("loadSamples called for " + juce::String (BKPianoSampleType_string[thisSampleType]));

        if (thisSampleType == BKPianoMain)
            loadSuccess = loadMainSamplesByPitch();
        else if (thisSampleType == BKPianoHammer)
            loadSuccess = loadHammerSamples();
        else if (thisSampleType == BKPianoReleaseResonance)
            loadSuccess = loadReleaseResonanceSamples();
        else if (thisSampleType == BKPianoPedal)
            loadSuccess = loadPedalSamples();

        if (!loadSuccess)
        {
            continueValue = i;
            DBG("SampleLoadJob::loadSamples() did not load some samples successfully");
            return false;
        }
        tickProgress();
    }

    return loadSuccess;
}

bool SampleLoadJob::loadHammerSamples()
{
    //for v1 samples, to define bottom threshold.
    //  for hammers, just to define a baseline, since we don't have velocity layers here
    float dBFSBelow = dbfsBelowContinue;

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

        //DBG ("**** loading hammer sample: " + filename + " " + juce::String (midiNote));

        // hammers are only assigned to their one key (rel1 => A0 only, etc...)
        juce::BigInteger midiNoteRange;
        midiNoteRange.setRange (midiNote, 1, true);

        juce::BigInteger velRange;
        velRange.setRange (0, 128, true);

        auto sound = soundset->add (new BKSamplerSound (filename, std::shared_ptr<Sample<juce::AudioFormatReader>> (sample), midiNoteRange, midiNote, 0, velRange, 1, dBFSBelow));
        if (shouldExit())
            return false;
    }
    // DBG ("done loading hammer samples");
    return true;
}

bool SampleLoadJob::loadReleaseResonanceSamples()
{
    auto ranges = getVelLayers (velocityLayers);
    int layers = ranges.size();
    int currentVelLayer = velLayerContinue;

    //for v1 samples, to define bottom threshold.
    //  quietest Yamaha sample is about -41dbFS, so -50 seems safe and reasonable
    float dBFSBelow = dbfsBelowContinue;

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

        //DBG ("**** loading resonance sample: " + filename);
        //DBG ("");
        if (shouldExit())
            return false;
    }

    // DBG ("done loading resonance samples, with this many velocity layers " + juce::String (currentVelLayer));
    return true;

}

bool SampleLoadJob::loadPedalSamples()
{
    //for v1 samples, to define bottom threshold.
    float dBFSBelow = dbfsBelowContinue;

    auto ranges = getVelLayers (velocityLayers);
    int layers = ranges.size();
    int currentVelLayer = velLayerContinue;

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

        //DBG ("**** loading pedal sample: " + filename + " " + juce::String (midiNote));

        // pedals set to single keys: 65 for pedalDown, 66 for pedalUp
        juce::BigInteger midiNoteRange;
        midiNoteRange.setRange (midiNote, 1, true);

        auto [begin, end] = ranges.getUnchecked (currentVelLayer++);
        juce::BigInteger velRange;
        velRange.setRange (begin, end - begin, true);
        //DBG ("pedal vel range = " + juce::String (begin) + " to " + juce::String (end) + " for " + filename);

        auto sound = soundset->add (new BKSamplerSound (filename, std::shared_ptr<Sample<juce::AudioFormatReader>> (sample), midiNoteRange, midiNote, 0, velRange, 1, dBFSBelow));
        if (shouldExit())
            return false;
    }

    DBG ("done loading pedal samples");
    return true;

}

// load all the velocity layers samples for a particular pitch/key
bool SampleLoadJob::loadMainSamplesByPitch()
{
    auto ranges = getVelLayers (velocityLayers);
    int layers = ranges.size();
    int currentVelLayer = velLayerContinue;

    //for v1 samples, to define bottom threshold.
    //  quietest Yamaha sample is about -41dbFS, so -50 seems safe and reasonable
    float dBFSBelow = dbfsBelowContinue;
    // DBG ("**** loading sample: " );
    while (true)
    {
        auto [reader, filename] = sampleReader->make (*manager);
        if (!reader)
            break; // Break the loop if the reader is null
        auto sample = std::make_shared<Sample<juce::AudioFormatReader>>(*(reader.get()), 10);
        juce::StringArray stringArray;
        stringArray.addTokens (filename, "v", "");
        juce::String noteName = stringArray[0];
        juce::String velLayer = stringArray[1];
        int midiNote = noteNameToRoot (noteName);
        // DBG(velLayer + "= " +  juce::String(currentVelLayer));
        auto [begin, end] = ranges.getUnchecked (currentVelLayer++);
        juce::BigInteger velRange;
        velRange.setRange (begin, end - begin, true);

        auto sound = soundset->add (new BKSamplerSound <juce::AudioFormatReader>(filename, sample, thisMidiRange, midiNote, 0, velRange, layers, dBFSBelow));

        dBFSBelow = sound->dBFSLevel; // to pass on to next sample, which should be the next velocity layer above

        //DBG ("**** loading sample: " + filename);
        //DBG ("");
        if (shouldExit()) {
            velLayerContinue = currentVelLayer++;
            return false;
        }
        //DBG ("done loading main samples for " + filename);
    }

    return true;
}

bool SampleLoadJob::loadSoundFont(juce::File sfzFile)
{
    // --- Validation ---
    if (soundset == nullptr)
    {
        DBG("loadSoundFont: soundset pointer is null.");
        return false;
    }

    if (!sfzFile.existsAsFile())
    {
        DBG("loadSoundFont: File does not exist: " + sfzFile.getFullPathName());
        return false;
    }

    // formatManager.registerBasicFormats();

    const juce::String ext = sfzFile.getFileExtension().toLowerCase();
    std::unique_ptr<SFZSound> sound;

    if (ext == ".sfz")
        sound = std::make_unique<SFZSound>(sfzFile.getFullPathName().toStdString());
    else if (ext == ".sf2")
        sound = std::make_unique<SF2Sound>(sfzFile.getFullPathName().toStdString());
    else
    {
        DBG("loadSoundFont: Unsupported extension " + ext);
        return false;
    }

    // --- Load the soundfont ---
    sound->load_regions();
    sound->load_samples();

    // dBFS stuff not currently relevant for soundfonts
    float dBFSBelow = -100.0f;
    int regionIndex = 0;

    // --- Load each region into BKSamplerSound ---
    for (int i =0; i <sound->num_regions(); i++)
    {
        auto region = sound->region_at(i);
        if (region == nullptr || region->sample == nullptr)
            continue;

        const juce::File sampleFile = juce::File(region->sample->get_path());

        // std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));
        // if (reader == nullptr)
        // {
        //     DBG("loadSoundFont: Failed to open sample file: " + sampleFile.getFullPathName());
        //     continue;
        // }

        // --- Wrap the reader into your Sample class ---
        std::shared_ptr<Sample<SFZRegion>> sample =
            std::make_shared<Sample<SFZRegion>>(*region);

        // --- Define note and velocity ranges ---
        juce::BigInteger noteRange;
        noteRange.setRange(region->lokey, region->hikey - region->lokey + 1, true);

        juce::BigInteger velRange;
        velRange.setRange(region->lovel, region->hivel - region->lovel + 1, true);

        int rootMidiNote = region->pitch_keycenter >= 0 ? region->pitch_keycenter : region->lokey;
        int transpose = (region->lokey == region->hikey) ? (region->lokey - rootMidiNote) : 0;

        // --- Create BKSamplerSound ---
        auto* newSound = new BKSamplerSound<SFZRegion>(
            region->sample->short_name(),
            sample
        );

        // --- Apply loop information ---
        // if (region->loop_mode != SFZRegion::no_loop)
        // {
        //     newSound->setLoopMode(LoopMode::forward);
        //
        //     const double sampleRate = sample->getSampleRate();
        //     const double startSecs = (double)region->loop_start / sampleRate;
        //     const double endSecs   = (double)region->loop_end / sampleRate;
        //
        //     newSound->setLoopPointsInSeconds(juce::Range<double>(startSecs, endSecs));
        // }

        // --- Add to soundset ---
        soundset->add(newSound);

        // --- Debug output ---
        DBG("Loaded region " + juce::String(regionIndex++) +
            ": " + region->sample->short_name() +
            " | key " + juce::String(region->lokey) + "-" + juce::String(region->hikey) +
            " | vel " + juce::String(region->lovel) + "-" + juce::String(region->hivel));
    }

    samplerLoader.soundfonts.push_back(std::move(sound));
    DBG("loadSoundFont: Finished loading " + sfzFile.getFileNameWithoutExtension());
    return true;
}
