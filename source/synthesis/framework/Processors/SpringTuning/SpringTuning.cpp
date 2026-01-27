/*
  ==============================================================================

    SpringTuning.cpp
    Created: 3 Aug 2018 3:43:46pm
    Author:  Theo Trevisan, Mike Mulshine, Dan Trueman
    Revise:  ./dlt 2025 for bitKlavier2

    Based on the Verlet mass/spring algorithm:
    Jakobsen, T. (2001). Advanced character physics.
     In IN PROCEEDINGS OF THE GAME DEVELOPERS CONFERENCE 2001, page 19.

  ==============================================================================
*/

#include "SpringTuning.h"
#include "SpringTuningUtilities.h"

SpringTuning::SpringTuning(SpringTuningParams &params, std::array<std::atomic<float>, 12> &circularTuningCustom) : sparams(params), customTuning(circularTuningCustom)
{
    particleArray.ensureStorageAllocated(128);
    tetherParticleArray.ensureStorageAllocated(128);

    enabledSpringArray.ensureStorageAllocated(128);
    enabledSpringArray.clear();

    tetherFundamental = PitchClass::C;
    useLowestNoteForFundamental = false;
    useHighestNoteForFundamental = false;
    useLastNoteForFundamental = false;
    usingFundamentalForIntervalSprings = true;

    // Converting std::string to juce::String takes time so
    // convert before the look
    // (was happening implicitly at spring->setName() and costing a lot of time
    juce::Array<juce::String> labels;
    for (auto label : intervalLabels)
        labels.add(juce::String(label));

    for (int i = 0; i < 128; i++)
    {
        // Active particle
        int pc = (i % 12);
        int octave = (int)(i / 12);

        Particle* p1 = new Particle(i * 100, i, notesInAnOctave[pc]);
        p1->setOctave(octave);
        p1->setEnabled(false);
        p1->setLocked(false);
        p1->setNote(i);
        particleArray.add(p1);

        // Tether particle
        Particle* p2 = new Particle(i * 100, i, notesInAnOctave[pc]);
        p2->setOctave(octave);
        p2->setEnabled(false);
        p2->setLocked(true);
        p2->setNote(i);
        tetherParticleArray.add(p2);

        Spring* s = new Spring(p1, p2, 0.0, 0.5, 0, labels.getUnchecked(0), false);
        tetherSpringArray.add(s);
    }

    /*
     * initialize primary user params to defaults
     */
    dragChanged();
    intervalFundamentalChanged();
    intervalScaleChanged();
    tetherScaleChanged();
    tetherFundamentalChanged();
    tetherStiffnessChanged();
    intervalStiffnessChanged();
}

SpringTuning::~SpringTuning()
{
    stopTimer();
    DBG("SpringTuning: stopping timer");
};

inline void SpringTuning::stop(void)
{
    stopTimer();
    DBG("SpringTuning: stopping timer");
}

void SpringTuning::rateChanged() // called from UI
{
    setRate(sparams.rate->getCurrentValue(), true);
}

void SpringTuning::dragChanged() // called from UI
{
    dragLocal = 1. - sparams.drag->getCurrentValue();
}

void SpringTuning::tetherStiffnessChanged()
{
    for (auto spring : tetherSpringArray)
    {
        spring->setStiffness(sparams.tetherStiffness->getCurrentValue());
    }
}

void SpringTuning::intervalStiffnessChanged()
{
    for (auto spring : enabledSpringArray)
    {
        spring->setStiffness(sparams.intervalStiffness->getCurrentValue());
    }
}

void SpringTuning::intervalScaleChanged()
{
    if(sparams.scaleId->get() == TuningSystem::Custom)
    {
        std::array<float, 12> newtuningv;
        int i=0;
        for (auto& offs : customTuning) newtuningv[i++] = offs * .01;
        copyStdArrayIntoJuceArray(newtuningv, intervalTuning);
    }
    else
    {
        auto newtuningv = getOffsetsFromTuningSystem(sparams.scaleId->get());
        copyStdArrayIntoJuceArray(newtuningv, intervalTuning);
    }

}

void SpringTuning::intervalFundamentalChanged()
{
    auto newfundamental = sparams.intervalFundamental->get();

    if(newfundamental < Fundamental::none) {
        intervalFundamentalActive = getPitchClassFromInt(intFromFundamental(newfundamental));
    }

    if(newfundamental == Fundamental::none) usingFundamentalForIntervalSprings = false;
    else usingFundamentalForIntervalSprings = true;

    if(newfundamental == Fundamental::lowest) useLowestNoteForFundamental = true;
    else useLowestNoteForFundamental = false;

    if(newfundamental == Fundamental::highest) useHighestNoteForFundamental = true;
    else useHighestNoteForFundamental = false;

    if(newfundamental == Fundamental::last) useLastNoteForFundamental = true;
    else useLastNoteForFundamental = false;

    if(newfundamental == Fundamental::automatic) {
        useAutomaticFundamental = true;
        intervalFundamentalActive = PitchClass::C; //init, probably not necessary
    }
    else useAutomaticFundamental = false;
}

void SpringTuning::tetherScaleChanged()
{
    auto newtuningv = getOffsetsFromTuningSystem(sparams.scaleId_tether->get());
    copyStdArrayIntoJuceArray(newtuningv, tetherTuning); // so i don't have to rewrite all this with std:vectors right now...

    updateTetherTuning();
}

void SpringTuning::updateTetherTuning()
{
    // DBG("updateTetherTuning, fundamental = " << intFromPitchClass(tetherFundamental));
    const juce::ScopedLock sl (lock);

    for (int i = 0; i < 128; i++)
    {
        tetherParticleArray[i]->setX( (i * 100.0) + 100. * tetherTuning[(i - intFromPitchClass(tetherFundamental)) % 12] );
        tetherParticleArray[i]->setRestX( (i * 100.0) + 100. * tetherTuning[(i - intFromPitchClass(tetherFundamental)) % 12] );

        particleArray[i]->setRestX( (i * 100.0) + 100. * tetherTuning[(i - intFromPitchClass(tetherFundamental)) % 12] );
    }
}

void SpringTuning::tetherFundamentalChanged()
{
    tetherFundamental = sparams.tetherFundamental->get();
    updateTetherTuning();
}

void SpringTuning::setRate(double r, bool start)
{
    if (start) {
        startTimer(1000 / sparams.rate->getCurrentValue());
        DBG("SpringTuning: starting timer");
    }
    else {
        stopTimer();
        DBG("SpringTuning: stopping timer");
    }
}

bool SpringTuning::getSpringMode(int which)
{
    juce::String whichSpringId = "useLocalOrFundamental_" + juce::String(which);
    for ( auto &param_ : *sparams.getBoolParams())
    {
        if(param_->getParameterID() == whichSpringId)
        {
            return param_->get();
        }
    }
    return false;
}

/**
 * simulate() first moves through the entire particle array and "integrates" their position,
 * moving them based on their "velocities" and the drag values
 * it then moves through both spring arrays (the tether springs and interval springs) and
 * calls satisfyConstraints(), which updates the spring values based on the spring strengths,
 * stiffnesses, and offsets from their rest lengths. This in turn updates the target positions
 * for the two particles associated with each spring.
*/
void SpringTuning::simulate()
{
    const juce::ScopedLock sl (lock);

    // update particle positions based on current velocities
    for (auto particle : particleArray)
    {
		if (particle->getEnabled() && !particle->getLocked())
        {
            //particle->integrate(sparams.drag->getCurrentValue());
            particle->integrate(dragLocal);
        }
	}

    // apply tether spring forces to all particles
    for (auto spring : tetherSpringArray)
    {
        if (spring->getEnabled())
        {
            //DBG("tetherSpringArray spring->getA()->getX() = " << spring->getA()->getX() << " spring->getB()->getX() = " << spring->getB()->getX());
            spring->satisfyConstraints();
        }
    }

    // apply interval spring forces to all particles
	for (auto spring : enabledSpringArray)
	{
	    //DBG("enabledSpringArray spring->getA()->getX() = " << spring->getA()->getX() << " spring->getB()->getX() = " << spring->getB()->getX());
        spring->satisfyConstraints();
	}
}

double SpringTuning::getSpringWeight(int which)
{
    juce::String whichWeightId = "intervalWeight_" + juce::String(which);
    for ( auto &param_ : *sparams.getFloatParams())
    {
        if(param_->getParameterID() == whichWeightId)
        {
            return param_->getCurrentValue();
        }
    }

    return 0.5;
}

void SpringTuning::setTetherWeight(int which, double weight)
{
    const juce::ScopedLock sl (lock);

    // DBG("SpringTuning::setTetherWeight " + String(which) + " " + String(weight));
    Spring* spring = tetherSpringArray[which];

    spring->setStrength(weight);

    Particle* a = spring->getA();
    Particle* b = spring->getB();
    Particle* use = nullptr;
    Particle* tethered = tetherParticleArray[which];

    if (a != tethered)  use = a;
    else                use = b;

    if (use != nullptr)
    {
        if (weight == 1.0)
        {
            use->setX(use->getRestX());
            use->setLocked(true);
        }
        else
        {
            use->setLocked(false);
            if (weight == 0.0)
            {
                tethered->setEnabled(false);
            }
        }
    }
}

void SpringTuning::addParticle(int note)
{
    const juce::ScopedLock sl (lock);
    particleArray[note]->setEnabled(true);
    tetherParticleArray[note]->setEnabled(true);
    // DBG("addParticle, restX = " << tetherParticleArray[note]->getRestX() << " X = " << tetherParticleArray[note]->getX());
}

void SpringTuning::removeParticle(int note)
{
    const juce::ScopedLock sl (lock);
    Particle* p = particleArray[note];
    p->setEnabled(false);
    tetherParticleArray[note]->setEnabled(false);
}

PitchClass SpringTuning::getTetherFundamental()
{
    return tetherFundamental;
}

double SpringTuning::getTetherWeightGlobal()
{
    return sparams.tetherWeightGlobal->getCurrentValue();
}

double SpringTuning::getTetherWeightSecondaryGlobal()
{
    return sparams.tetherWeightSecondaryGlobal->getCurrentValue();
}

void SpringTuning::addNote(int note)
{
    addParticle(note);

    if(useLowestNoteForFundamental)
    {
        // DBG("lowest current note = " + String(getLowestActiveParticle()));
        intervalFundamentalActive = getPitchClassFromInt(getLowestActiveParticle() % 12);
    }
    else if(useHighestNoteForFundamental)
    {
        // DBG("highest current note = " + String(getHighestActiveParticle()));
        intervalFundamentalActive = getPitchClassFromInt(getHighestActiveParticle() % 12);
    }
    else if(useLastNoteForFundamental)
    {
        // DBG("last note = " + String(note));
        intervalFundamentalActive = getPitchClassFromInt(note % 12);
    }
    else if(useAutomaticFundamental)
    {
        findFundamental(); //sets intervalFundamental internally
    }

    addSpringsByNote(note);
    if(useLowestNoteForFundamental || useHighestNoteForFundamental || useLastNoteForFundamental || useAutomaticFundamental) retuneAllActiveSprings();

    /*
     * update this here for the UI
     */
    sparams.tCurrentSpringTuningFundamental->setParameterValue(intervalFundamentalActive);
}

void SpringTuning::removeNote(int note)
{
    removeParticle(note);

    if(useLowestNoteForFundamental)
    {
        // DBG("lowest current note = " + String(getLowestActiveParticle()));
        intervalFundamentalActive = getPitchClassFromInt(getLowestActiveParticle() % 12);
    }
    else if(useHighestNoteForFundamental)
    {
        // DBG("highest current note = " + String(getHighestActiveParticle()));
        intervalFundamentalActive = getPitchClassFromInt(getHighestActiveParticle() % 12);
    }
    else if(useAutomaticFundamental)
    {
        findFundamental();
    }

    removeSpringsByNote(note);

    if(useLowestNoteForFundamental || useHighestNoteForFundamental || useAutomaticFundamental)
        retuneAllActiveSprings();
}

void SpringTuning::findFundamental()
{
    const juce::ScopedLock sl (lock);

    //create sorted array of notes
    juce::Array<int> enabledNotes;
    for (int i=127; i>=0; i--)
    {
        if(particleArray[i]->getEnabled())
        {
            enabledNotes.insert(0, i);
        }
    }

    //if(enabledNotes.size() > 1)
    {
        int fundamental_57 = -1;
        int fundamental_48 = -1;
        int fundamental_39 = -1;

        for(int i=enabledNotes.size() - 1; i>0; i--)
        {
            for(int j=i-1; j>=0; j--)
            {
                int interval = (enabledNotes[i] - enabledNotes[j]) % 12;

                if(interval == 7)
                {
                    fundamental_57 = enabledNotes[j] % 12;
                    //DBG("Fifth fundamental 5 = " + String(fundamental_57));
                }
                else if(interval == 5)
                {
                    fundamental_57 = enabledNotes[i] % 12;
                    //DBG("Fifth fundamental 4 = " + String(fundamental_57));
                }
                else if(interval == 4)
                {
                    fundamental_48 = enabledNotes[j] % 12;
                    //DBG("Third fundamental 3 = " + String(fundamental_48));
                }
                else if(interval == 8)
                {
                    fundamental_48 = enabledNotes[i] % 12;
                    //DBG("Third fundamental 6 = " + String(fundamental_48));
                }
                else if(interval == 3)
                {
                    fundamental_39 = (enabledNotes[j] - 4) % 12;
                    //DBG("MinorThird fundamental 3 = " + String(fundamental_39));
                }
                else if(interval == 9)
                {
                    fundamental_39 = (enabledNotes[i] - 4) % 12;
                    //DBG("MinorThird fundamental 6 = " + String(fundamental_39));
                }
            }
        }

        if(fundamental_57 > -1)
        {
            intervalFundamentalActive = getPitchClassFromInt(fundamental_57);
        }
        else if(fundamental_48 > -1)
        {
            intervalFundamentalActive = getPitchClassFromInt(fundamental_48);
        }
        else if(fundamental_39 > -1)
        {
            intervalFundamentalActive = getPitchClassFromInt(fundamental_39);
        }
    }

//    sparams.tCurrentSpringTuningFundamental->setParameterValue(intervalFundamentalActive);
}

void SpringTuning::addSpring(Spring* spring)
{
    const juce::ScopedLock sl (lock);
    if (enabledSpringArray.contains(spring)) return;

    int interval = spring->getIntervalIndex();
    spring->setEnabled(true);
    spring->setStiffness(sparams.intervalStiffness->getCurrentValue());
    //spring->setStrength(springWeights[interval]);
    spring->setStrength(getSpringWeight(interval));
    enabledSpringArray.add(spring);

//    DBG("addSpring          = " + juce::String(spring->getIntervalIndex()) +
//         "\nresting length   = " + juce::String(spring->getRestingLength()) +
//         "\nstrength         = " + juce::String(spring->getStrength()) +
//         "\nstiffness        = " + juce::String(spring->getStiffness()) +
//         "\nenabled          = " + juce::String((int)spring->getEnabled()));

    retuneIndividualSpring(spring);
}

void SpringTuning::addSpringsByNote(int note)
{
    const juce::ScopedLock sl (lock);

    for (auto p : particleArray)
    {
        int otherNote = p->getNote();
        if (otherNote == note) continue;

        if (p->getEnabled())
        {
            int upperNote = note > otherNote ? note : otherNote;
            int lowerNote = note < otherNote ? note : otherNote;
            int hash = (upperNote << 16 | lowerNote);

            if (!springArray.contains(hash))
            {
                float diff = upperNote - lowerNote;

                int interval = (int)diff % 12;
                int octInterval = interval;

                if (diff != 0 && interval == 0)
                {
                    octInterval = 12;
                }

                if (usingFundamentalForIntervalSprings)
                {
                    int scaleDegree1 = particleArray.getUnchecked(upperNote)->getNote();
                    int scaleDegree2 = particleArray.getUnchecked(lowerNote)->getNote();;

                    diff = fabs(100. *
                    ((scaleDegree1 +
                      intervalTuning.getUnchecked((scaleDegree1 - intFromPitchClass(intervalFundamentalActive)) % 12)) -
                    (scaleDegree2 +
                     intervalTuning.getUnchecked((scaleDegree2 - intFromPitchClass(intervalFundamentalActive)) % 12))));
                }
                else diff = diff * 100 + intervalTuning.getUnchecked(interval) * 100;

                springArray.set(hash, new Spring(particleArray.getUnchecked(lowerNote),
                                                 particleArray.getUnchecked(upperNote),
                                                 diff, //rest length in cents
                                                 0.5,
                                                 octInterval,
                                                 intervalLabels[octInterval],
                                                 false));
            }
            addSpring(springArray[hash]);
        }
    }

    tetherSpringArray[note]->setEnabled(true);

    if(sparams.fundamentalSetsTether->get() == true)
    {
        for (auto tether : tetherSpringArray)
        {
            if(tether->getEnabled())
            {
                int tnoteA = tether->getA()->getNote();
                int tnoteB = tether->getB()->getNote();

                if(tnoteA % 12 == intFromPitchClass(getTetherFundamental()) || tnoteB % 12 == intFromPitchClass(getTetherFundamental()))
                {
                    setTetherWeight(tnoteA, getTetherWeightGlobal());
                }
                else
                {
                    setTetherWeight(tnoteA, getTetherWeightSecondaryGlobal());
                }
            }
        }
    }
}

void SpringTuning::retuneIndividualSpring(Spring* spring)
{
    int interval = spring->getIntervalIndex();

    //set spring length locally, for all if !usingFundamentalForIntervalSprings, or for individual springs as set by L/F
    if(!usingFundamentalForIntervalSprings || !getSpringMode(interval))
    {
        int diff = spring->getA()->getRestX() - spring->getB()->getRestX();
        //DBG("retuneIndividualSpring, resting length = " << fabs(diff) + 100. * (intervalTuning[interval] - tetherTuning[interval]));
        spring->setRestingLength(fabs(diff) + 100. * (intervalTuning[interval] - tetherTuning[interval]));
    }

    //otherwise, set resting length to interval scale relative to intervalFundamental (F)
    else
    {
        int scaleDegree1 = spring->getA()->getNote();
        int scaleDegree2 = spring->getB()->getNote();

        float diff =  100. *
                         ((scaleDegree2 + intervalTuning[(scaleDegree2 - intFromPitchClass(intervalFundamentalActive)) % 12])
                     -   (scaleDegree1 + intervalTuning[(scaleDegree1 - intFromPitchClass(intervalFundamentalActive)) % 12]));

        spring->setRestingLength(fabs(diff));
    }
}

void SpringTuning::retuneAllActiveSprings(void)
{
    const juce::ScopedLock sl (lock);

    for (auto spring : enabledSpringArray)
    {
        retuneIndividualSpring(spring);
    }
}

void SpringTuning::removeSpringsByNote(int note)
{
    const juce::ScopedLock sl (lock);

	Particle* p = particleArray[note];

    int size = enabledSpringArray.size();
    for (int i = (size-1); i >= 0; i--)
	{
        Spring* spring = enabledSpringArray[i];
        Particle* a = spring->getA();
        Particle* b = spring->getB();

		if (spring->getEnabled() && ((a == p) || (b == p)))
        {
            spring->setEnabled(false);
            enabledSpringArray.remove(i);
        }
	}

    tetherSpringArray[note]->setEnabled(false);
}

double SpringTuning::getFrequency(int note, float globalRefA4)
{
    const juce::ScopedLock sl (lock);

    auto particles = getParticles();
    double x = particles[note]->getX();
//    int octave = particles[note]->getOctave();
    double freq = mtof(x * .01, globalRefA4);

//    DBG("SpringTuning::getFrequency for " + juce::String(note) +
//        " x = " + juce::String(x) +
//        " octave = " + juce::String(octave) +
//        " output frequency = " + juce::String(midi));

    return freq;
}

void SpringTuning::print()
{
    DBG("~ ~ ~ ~ ~ ~ ~ Spring Tuning ~ ~ ~ ~ ~ ~");
    DBG("scaleId                        = " + sparams.scaleId->getCurrentValueAsText());
    DBG("  intervalFundamental          = " + sparams.intervalFundamental->getCurrentValueAsText());
    DBG("tether scaleId                 = " + sparams.scaleId_tether->getCurrentValueAsText());
    DBG("  tetherFundamental =          = " + sparams.tetherFundamental->getCurrentValueAsText());
    DBG("rate                           = " + sparams.rate->getCurrentValueAsText());
    DBG("drag                           = " + sparams.drag->getCurrentValueAsText());
    DBG("intervalStiffness              = " + sparams.intervalStiffness->getCurrentValueAsText());
    DBG("tetherStiffness                = " + sparams.tetherStiffness->getCurrentValueAsText());
    DBG("tetherWeightGlobal             = " + sparams.tetherWeightGlobal->getCurrentValueAsText());
    DBG("tetherWeightSecondaryGlobal    = " + sparams.tetherWeightSecondaryGlobal->getCurrentValueAsText());

//	for (int i = 0; i < 128; i++)
//	{
//        juce::String printStatus = "";
//		if (particleArray[i]->getEnabled())
//        {
//            DBG("particle: " + juce::String(i) + " " + juce::String(particleArray[i]->getX()));
//        }
//	}
}

int SpringTuning::getLowestActiveParticle()
{
    const juce::ScopedLock sl (lock);

    int lowest = 0;

    while(lowest < particleArray.size())
    {
        if(particleArray[lowest]->getEnabled()) return lowest;

        lowest++;
    }

    return lowest;
}

int SpringTuning::getHighestActiveParticle()
{
    const juce::ScopedLock sl (lock);

    int highest = particleArray.size() - 1;

    while(highest >= 0)
    {
        if(particleArray[highest]->getEnabled()) return highest;

        highest--;
    }

    return highest;
}

juce::Array<Particle*> SpringTuning::getParticles(void)
{
    return particleArray;
}

void SpringTuning::hiResTimerCallback(void)
{
    // DBG("Spring Tuning timer callback");
    if (sparams.active->get())
    {
        simulate();
    }
}
