/*
  ==============================================================================

    SpringTuning.cpp
    Created: 3 Aug 2018 3:43:46pm
    Author:  Theo Trevisan, Mike Mulshine, Dan Trueman

    Based on the Verlet mass/spring algorithm:
    Jakobsen, T. (2001). Advanced character physics.
     In IN PROCEEDINGS OF THE GAME DEVELOPERS CONFERENCE 2001, page 19.

  ==============================================================================
*/

#include "SpringTuning.h"
#include "SpringTuningUtilities.h"

SpringTuning::SpringTuning(SpringTuningParams &params) : sparams(params)
{
    particleArray.ensureStorageAllocated(128);
    tetherParticleArray.ensureStorageAllocated(128);

    enabledSpringArray.ensureStorageAllocated(128);
    enabledSpringArray.clear();

    springMode.ensureStorageAllocated(12);
    for(int i=0; i<12; i++) springMode.insert(i, true);

    tetherFundamental = PitchClass::C;
    useLowestNoteForFundamental = false;
    useHighestNoteForFundamental = false;
    useLastNoteForFundamental = false;
    usingFundamentalForIntervalSprings = true;

    /**
     * todo: check these initialization vals, probably should be overwritten by params in SpringTuningParams
     */
    for (int i = 0; i < 13; i++) springWeights[i] = 0.5;
    springWeights[7] = 0.75;
    springWeights[9] = 0.25;
    setSpringMode(6, false);

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
    intervalFundamentalChanged();
    intervalScaleChanged();
    tetherScaleChanged();
    tetherFundamentalChanged();
    tetherStiffnessChanged();
    intervalStiffnessChanged();

    /**
     * todo: remove true here; this starts the Timer on construction, which we don't want ultimately
     *          should only be on when spring tuning is the selected tuning
     */
    setRate(sparams.rate->getCurrentValue(), true);

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
    auto newtuningv = getOffsetsFromTuningSystem(sparams.scaleId->get());
    copyStdArrayIntoJuceArray(newtuningv, intervalTuning);
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
    const juce::ScopedLock sl (lock);

    for (int i = 0; i < 128; i++)
    {
        tetherParticleArray[i]->setX( (i * 100.0) + tetherTuning[(i - intFromPitchClass(tetherFundamental)) % 12] );
        tetherParticleArray[i]->setRestX( (i * 100.0) + tetherTuning[(i - intFromPitchClass(tetherFundamental)) % 12] );

        particleArray[i]->setRestX( (i * 100.0) + tetherTuning[(i - intFromPitchClass(tetherFundamental)) % 12] );
    }
}

void SpringTuning::tetherFundamentalChanged()
{
    updateTetherTuning();
}

inline void SpringTuning::setRate(double r, bool start)
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

inline juce::Array<float> SpringTuning::getTetherWeights(void)
{
    juce::Array<float> weights;
    for (auto spring : getTetherSprings())
    {
        weights.add(spring->getStrength());
    }

    return weights;
}

inline void SpringTuning::setTetherWeights(juce::Array<float> weights)
{
    for (int i = 0; i < 128; i++)
    {
        tetherSpringArray[i]->setStrength(weights[i]);
    }
}

inline juce::Array<float> SpringTuning::getSpringWeights(void)
{
    juce::Array<float> weights;

    for (int i = 0; i < 12; i++)
    {
        weights.add(getSpringWeight(i));
    }

    return weights;
}

inline void SpringTuning::setSpringWeights(juce::Array<float> weights)
{
    for (int i = 0; i < 12; i++)
    {
        setSpringWeight(i, weights[i]);
    }
}

inline void SpringTuning::setSpringMode(int which, bool on)
{
    springMode.set(which, on);
}

bool SpringTuning::getSpringMode(int which)
{
    return springMode.getUnchecked(which);
}

/*
simulate() first moves through the entire particle array and "integrates" their position,
moving them based on their "velocities" and the drag values

it then moves through both spring arrays (the tether springs and interval springs) and
calls satisfyConstraints(), which updates the spring values based on the spring strengths,
stiffnesses, and offsets from their rest lengths. This in turn updates the target positions
for the two particles associated with each spring.
*/
void SpringTuning::simulate()
{
    const juce::ScopedLock sl (lock);

    // update particle positions based on current velocities
    for (auto particle : particleArray)
    {
		if (particle->getEnabled() && !particle->getLocked())
        {
            particle->integrate(sparams.drag->getCurrentValue());
        }
	}

    // apply tether spring forces to all particles
    for (auto spring : tetherSpringArray)
    {
        if (spring->getEnabled())
        {
            spring->satisfyConstraints();
        }
    }

    // apply interval spring forces to all particless
	for (auto spring : enabledSpringArray)
	{
        spring->satisfyConstraints();
	}
}

void SpringTuning::setSpringWeight(int which, double weight)
{
    const juce::ScopedLock sl (lock);

    which += 1;

    springWeights[which] = weight;

    for (auto spring : enabledSpringArray)
    {
        if (spring->getIntervalIndex() == which)
        {
            spring->setStrength(weight);

            // DBG("reweighting interval " + String(which) +  " to " + String(weight));
        }
    }
}

double SpringTuning::getSpringWeight(int which)
{
    which += 1;

    return springWeights[which];
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

double SpringTuning::getTetherWeight(int which)
{
    const juce::ScopedLock sl (lock);
    return tetherSpringArray[which]->getStrength();
}

juce::String SpringTuning::getTetherSpringName(int which)
{
    const juce::ScopedLock sl (lock);
    return tetherSpringArray[which]->getName();
}

juce::String SpringTuning::getSpringName(int which)
{
    const juce::ScopedLock sl (lock);
    for (auto spring : springArray)
    {
        if (spring->getIntervalIndex() == which) return spring->getName();
    }
    return "";
}

void SpringTuning::addParticle(int note)
{
    const juce::ScopedLock sl (lock);
    particleArray[note]->setEnabled(true);
    tetherParticleArray[note]->setEnabled(true);
}

void SpringTuning::removeParticle(int note)
{
    const juce::ScopedLock sl (lock);
    Particle* p = particleArray[note];
    p->setEnabled(false);
    tetherParticleArray[note]->setEnabled(false);
}

Particle* SpringTuning::getParticle(int note)
{
    return particleArray[note];
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

void SpringTuning::removeAllNotes(void)
{
    for (int i = 0; i < 128; i++) removeNote(i);
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

    sparams.tCurrentSpringTuningFundamental->setParameterValue(intervalFundamentalActive);
}

void SpringTuning::addSpring(Spring* spring)
{
    const juce::ScopedLock sl (lock);
    if (enabledSpringArray.contains(spring)) return;

    int interval = spring->getIntervalIndex();
    spring->setEnabled(true);
    spring->setStiffness(sparams.intervalStiffness->getCurrentValue());
    spring->setStrength(springWeights[interval]);
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
    if(!usingFundamentalForIntervalSprings || !getSpringMode(interval - 1))
    {
        int diff = spring->getA()->getRestX() - spring->getB()->getRestX();
        spring->setRestingLength(fabs(diff) + 100. * intervalTuning[interval]);
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

double SpringTuning::getFrequency(int note)
{
    auto particles = getParticles();
    double x = particles[note]->getX();
    int octave = particles[note]->getOctave();
    double midi = mtof(x * .01, 440.);
    /**
     * todo: need to get globalTuningReference in here
     */

//    DBG("SpringTuning::getFrequency for " + juce::String(note) +
//        " x = " + juce::String(x) +
//        " octave = " + juce::String(octave) +
//        " output frequency = " + juce::String(midi));

    return midi;
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

juce::Array<Spring*> SpringTuning::getTetherSprings(void)
{
    return tetherSpringArray;
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






//****************************************************************************************************************//

/*
 * don't think we need the below
 */


//    juce::ValueTree getState(void)
//    {
//        juce::ValueTree prep("springtuning");
//
//        rate.getState(prep, "rate");
//        drag.getState(prep, "drag");
//        tetherStiffness.getState(prep, "tetherStiffness");
//        intervalStiffness.getState(prep, "intervalStiffness");
//        stiffness.getState(prep, "stiffness");
//        active.getState(prep, "active");
//        scaleId.getState(prep, "intervalTuningId");
//        intervalFundamental.getState(prep, "intervalFundamental");
//        fundamentalSetsTether.getState(prep, "fundamentalSetsTether");
//        tetherWeightGlobal.getState(prep, "tetherWeightGlobal");
//        tetherWeightSecondaryGlobal.getState(prep, "tetherWeightSecondaryGlobal");
//
//        //prep.setProperty( "usingFundamentalForIntervalSprings", (int)usingFundamentalForIntervalSprings, 0);
//
//        juce::ValueTree tethers( "tethers");
//        juce::ValueTree springs( "springs");
//        juce::ValueTree intervalScale("intervalScale");
//        juce::ValueTree springMode("springMode");
//
//        for (int i = 0; i < 128; i++)
//        {
//            tethers.setProperty( "t"+juce::String(i), getTetherWeight(i), 0 );
//        }
//
//        for (int i = 0; i < 12; i++)
//        {
//            springs.setProperty( "s"+juce::String(i), getSpringWeight(i), 0 );
//            intervalScale.setProperty("s"+juce::String(i), intervalTuning[i], 0);
//            springMode.setProperty("s"+juce::String(i), (int)getSpringMode(i), 0);
//        }
//        prep.addChild(tethers, -1, 0);
//        prep.addChild(springs, -1, 0);
//        prep.addChild(intervalScale, -1, 0);
//        prep.addChild(springMode, -1, 0);
//
//        return prep;
//    }
//
//    // for unit-testing
//    inline void randomize()
//    {
//        juce::Random::getSystemRandom().setSeedRandomly();
//
//        double r[200];
//
//        for (int i = 0; i < 200; i++)  r[i] = (juce::Random::getSystemRandom().nextDouble());
//        int idx = 0;
//
//        rate = r[idx++];
//        drag = r[idx++];
//        tetherStiffness = r[idx++];
//        intervalStiffness = r[idx++];
//        stiffness = r[idx++];
//        active = r[idx++];
//        scaleId = (TuningSystem) (int) (r[idx++] * TuningSystem::TuningSystemNil);
//        intervalFundamental = (PitchClass) (int) (r[idx++] * PitchClass::none);
//
//        for (int i = 0; i < 128; i++)
//        {
//            setTetherWeight(i, r[idx++]);
//        }
//
//        for (int i = 0; i < 12; i++)
//        {
//            setSpringWeight(i, r[idx++]);
//            intervalTuning.setUnchecked(i, r[idx++] - 0.5);
//            setSpringMode(i, r[idx++]);
//        }
//    }
//
//    void setState(juce::XmlElement* e)
//    {
//        active.setState(e, "active", false);
//        rate.setState(e, "rate", 100);
//        drag.setState(e, "drag", 0.1);
//
//        stiffness.setState(e, "stiffness", 1.0);
//        setStiffness(stiffness.value);
//        tetherStiffness.setState(e, "tetherStiffness", 0.5);
//        setTetherStiffness(tetherStiffness.value);
//        intervalStiffness.setState(e, "intervalStiffness", 0.5);
//        setIntervalStiffness(intervalStiffness.value);
//
//        scaleId.setState(e, "intervalTuningId", TuningSystem::Just);
//        intervalFundamental.setState(e, "intervalFundamental", PitchClass::C);
//        intervalFundamentalChanged();
//        fundamentalSetsTether.setState(e, "fundamentalSetsTether", false);
//        tetherWeightGlobal.setState(e, "tetherWeightGlobal", 0.5);
//        tetherWeightSecondaryGlobal.setState(e, "tetherWeightSecondaryGlobal", 0.1);
//
//        // Starts the timer if active, stops it otherwise
//        setRate(getRate(), getActive());
//        // Make sure all springs and particles are updated
//        setIntervalTuning(getIntervalTuning());
//        setTetherTuning(getTetherTuning());
//        setSpringWeights(getSpringWeights());
//        setTetherWeights(getTetherWeights());
//        setIntervalFundamental(getIntervalFundamental());
////        setUsingFundamentalForIntervalSprings(getUsingFundamentalForIntervalSprings());
//        setFundamentalSetsTether(getFundamentalSetsTether());
//        setTetherWeightGlobal(getTetherWeightGlobal());
//        setTetherWeightSecondaryGlobal(getTetherWeightSecondaryGlobal());
//
//        for (auto sub : e->getChildIterator())
//        {
//            if (sub->hasTagName("intervalScale"))
//            {
//                juce::Array<float> scale;
//                for (int i = 0; i < 12; i++)
//                {
//                    juce::String attr = sub->getStringAttribute("s" + juce::String(i));
//
//                    if (attr == "") scale.add(0.0);
//                    else            scale.add(attr.getFloatValue());
//                }
//
//                setIntervalTuning(scale);
//            }
//            else if (sub->hasTagName("tethers"))
//            {
//                juce::Array<float> scale;
//                for (int i = 0; i < 128; i++)
//                {
//                    juce::String attr = sub->getStringAttribute("t" + juce::String(i));
//
//                    if (attr == "")
//                    {
//                        setTetherWeight(i, 0.2);
//                    }
//                    else
//                    {
//                        setTetherWeight(i, attr.getDoubleValue());
//                    }
//                }
//            }
//            else if (sub->hasTagName("springs"))
//            {
//                for (int i = 0; i < 12; i++)
//                {
//                    juce::String attr = sub->getStringAttribute("s" + juce::String(i));
//
//                    if (attr == "")
//                    {
//                        setSpringWeight(i, 0.5);
//                    }
//                    else
//                    {
//                        setSpringWeight(i, attr.getDoubleValue());
//                    }
//                }
//            }
//            else if (sub->hasTagName("springMode"))
//            {
//                for (int i = 0; i < 12; i++)
//                {
//                    juce::String attr = sub->getStringAttribute("s" + juce::String(i));
//
//                    if (attr == "")
//                    {
//                        setSpringMode(i, false);
//                    }
//                    else
//                    {
//                        setSpringMode(i, (bool)attr.getIntValue());
//                        // DBG("setState::setSpringMode " + String(attr.getIntValue()));
//                    }
//                }
//            }
//        }
//    }



/**
 * shouldn't need this copy constructor
 *      preps will be copied by valueTree
 *
 */
//void SpringTuning::copy(SpringTuning::Ptr st)
//void SpringTuning::copy(SpringTuning* st)
//{

//    // DBG("SpringTuning::copy called!!");
//    sparams.rate->setParameterValue(st->getRate());
////    sparams.stiffness->setParameterValue(st->getStiffness());
//    sparams.active->setParameterValue(st->getActive());
//    sparams.drag->setParameterValue(st->getDrag());
//    sparams.intervalStiffness->setParameterValue(st->getIntervalStiffness());
//    sparams.tetherStiffness->setParameterValue(st->getTetherStiffness());
//
//    for (int i = 0; i < 13; i++)
//    {
//        springWeights[i] = st->springWeights[i];
//    }
//
//    for (int i=0; i<12; i++)
//    {
//        setSpringMode(i, st->getSpringMode(i));
//    }
//
//    sparams.scaleId->setParameterValue(st->getScaleId());
//
//    setIntervalTuning(st->getIntervalTuning());
//    setTetherTuning(st->getTetherTuning());
//
//    setSpringWeights(st->getSpringWeights());
//    setTetherWeights(st->getTetherWeights());
//
//    setIntervalFundamental(st->getIntervalFundamental());
//    //setUsingFundamentalForIntervalSprings(st->getUsingFundamentalForIntervalSprings());
//
//    setFundamentalSetsTether(st->getFundamentalSetsTether());
//    setTetherWeightGlobal(st->getTetherWeightGlobal());
//    setTetherWeightSecondaryGlobal(st->getTetherWeightSecondaryGlobal());
//}



//inline void SpringTuning::setTetherStiffness(double stiff)
//{
//    sparams.tetherStiffness->setParameterValue(stiff);
//    tetherStiffnessChanged();
//}
//
//inline void SpringTuning::setIntervalStiffness(double stiff)
//{
//    sparams.intervalStiffness->setParameterValue(stiff);
//    intervalStiffnessChanged();
//}

//void SpringTuning::stiffnessChanged()
//{
//    for (auto spring : enabledSpringArray)
//    {
//        spring->setStiffness(sparams.stiffness->getCurrentValue());
//    }
//
//    for (auto spring : tetherSpringArray)
//    {
//        spring->setStiffness(sparams.stiffness->getCurrentValue());
//    }
//}



//void SpringTuning::retuneIndividualSpring(Spring::Ptr spring)
//{
//    int interval = spring->getIntervalIndex();
//
//    //set spring length locally, for all if !usingFundamentalForIntervalSprings, or for individual springs as set by L/F
//    if(!usingFundamentalForIntervalSprings ||
//       !getSpringMode(interval - 1))
//    {
//        int diff = spring->getA()->getRestX() - spring->getB()->getRestX();
//        spring->setRestingLength(fabs(diff) + intervalTuning[interval]);
//    }
//
//    //otherwise, set resting length to interval scale relative to intervalFundamental (F)
//    else
//    {
//        int scaleDegree1 = spring->getA()->getNote();
//        int scaleDegree2 = spring->getB()->getNote();
//        //int intervalFundamental = 0; //temporary, will set in preparation
//
//        float diff =    (100. * scaleDegree2 + intervalTuning[(scaleDegree2 - (int)intervalFundamentalActive) % 12]) -
//        (100. * scaleDegree1 + intervalTuning[(scaleDegree1 - (int)intervalFundamentalActive) % 12]);
//
//        spring->setRestingLength(fabs(diff));
//    }
//}



//void SpringTuning::addSpring(Spring::Ptr spring)
//{
//    const juce::ScopedLock sl (lock);
//
//    if (enabledSpringArray.contains(spring)) return;
//    int interval = spring->getIntervalIndex();
//
//    spring->setEnabled(true);
//    spring->setStiffness(sparams.intervalStiffness->getCurrentValue());
//    spring->setStrength(springWeights[interval]);
//    enabledSpringArray.add(spring);
//
//    retuneIndividualSpring(spring);
//}



//void SpringTuning::setTetherFundamental(PitchClass newfundamental)
//{
//    tetherFundamental = newfundamental;
//    updateTetherTuning();
//}

//void SpringTuning::setIntervalTuning(juce::Array<float> tuning)
//{
//    intervalTuning = tuning;
//}


//inline double SpringTuning::getRate(void) { return sparams.rate->getCurrentValue(); }
//inline double SpringTuning::getTetherStiffness(void) { return sparams.tetherStiffness->getCurrentValue(); }
//inline double SpringTuning::getIntervalStiffness(void) { return sparams.intervalStiffness->getCurrentValue(); }
//inline void SpringTuning::setDrag(double newdrag) { sparams.drag->setParameterValue(newdrag); }
//inline double SpringTuning::getDrag(void) { return sparams.drag->getCurrentValue(); }

//bool SpringTuning::getFundamentalSetsTether() { return sparams.fundamentalSetsTether->get(); }
//void SpringTuning::setFundamentalSetsTether(bool s) { sparams.fundamentalSetsTether->setParameterValue(s); }

//juce::Array<float> SpringTuning::getTetherTuning(void) {return tetherTuning;}
//void SpringTuning::setUsingFundamentalForIntervalSprings(bool use) { usingFundamentalForIntervalSprings = use; }
//bool SpringTuning::getUsingFundamentalForIntervalSprings(void) { return usingFundamentalForIntervalSprings; }
//inline void SpringTuning::setScaleId(TuningSystem which) { sparams.scaleId->setParameterValue(which); }
//inline void SpringTuning::setActive(bool status) { sparams.active->setParameterValue(status); }

//bool SpringTuning::getSpringModeButtonState(int which) {return springMode.getUnchecked(which);}
//Fundamental SpringTuning::getIntervalFundamental() { return sparams.intervalFundamental->get(); }
//PitchClass SpringTuning::getIntervalFundamentalActive() { return intervalFundamentalActive; }
//juce::Array<float> SpringTuning::getIntervalTuning(void){return intervalTuning;}
//PitchClass SpringTuning::getTetherFundamental() {return tetherFundamental;}


//double SpringTuning::getTetherWeightGlobal() { return sparams.tetherWeightGlobal->getCurrentValue(); }
//double SpringTuning::getTetherWeightSecondaryGlobal() { return sparams.tetherWeightSecondaryGlobal->getCurrentValue(); }

//
//inline bool SpringTuning::getActive(void) { return sparams.active->get(); }
//inline TuningSystem SpringTuning::getScaleId(void) { return sparams.scaleId->get(); }

//void SpringTuning::printParticles()
//{
//    const juce::ScopedLock sl (lock);
//
//    for (int i = 0; i < 128; i++)
//    {
//        if(particleArray[i]->getEnabled()) particleArray[i]->print();
//    }
//}


//void SpringTuning::toggleNote(int noteIndex)
//{
//    const juce::ScopedLock sl (lock);
//
//	int convertedIndex = noteIndex; // just in case a midi value is passed accidentally
//
//	if (particleArray[convertedIndex]->getEnabled())
//	{
//		removeNote(convertedIndex);
//	}
//	else
//	{
//		addNote(convertedIndex);
//	}
//}


//Particle::PtrArr& SpringTuning::getTetherParticles(void) { return tetherParticleArray;}
//Spring::PtrArr& SpringTuning::getTetherSprings(void) { return tetherSpringArray;}
//Particle::PtrArr& SpringTuning::getParticles(void) { return particleArray;}
//Spring::PtrMap& SpringTuning::getSprings(void) { return springArray; }
//Spring::PtrArr& SpringTuning::getEnabledSprings(void) { return enabledSpringArray;}


//juce::Array<Particle*> SpringTuning::getTetherParticles(void) { return tetherParticleArray;}
//juce::HashMap<int, Spring*> SpringTuning::getSprings(void) { return springArray; }
//juce::Array<Spring*>SpringTuning::getEnabledSprings(void) { return enabledSpringArray;}


//void SpringTuning::printActiveParticles()
//{
//    const juce::ScopedLock sl (lock);
//
//	for (int i = 0; i < 128; i++)
//	{
//		if (particleArray[i]->getEnabled()) particleArray[i]->print();
//	}
//}
//
//void SpringTuning::printActiveSprings()
//{
//    const juce::ScopedLock sl (lock);
//
//	for (auto spring : springArray)
//	{
//		if (spring->getEnabled()) spring->print();
//	}
//}
//
//bool SpringTuning::checkEnabledParticle(int index)
//{
//    const juce::ScopedLock sl (lock);
//	return particleArray[index]->getEnabled();
//}
