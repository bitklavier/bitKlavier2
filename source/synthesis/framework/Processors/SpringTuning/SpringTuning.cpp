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

//using namespace std;

//SpringTuning::SpringTuning(SpringTuning::Ptr st, SpringTuningParams &params) : sparams(params)
SpringTuning::SpringTuning(SpringTuningParams &params) : sparams(params)
{
    particleArray.ensureStorageAllocated(128);
    tetherParticleArray.ensureStorageAllocated(128);

    enabledSpringArray.ensureStorageAllocated(128);
    enabledSpringArray.clear();

    tetherTuning = juce::Array<float>({0,0,0,0,0,0,0,0,0,0,0,0});

    springMode.ensureStorageAllocated(12);
    for(int i=0; i<12; i++) springMode.insert(i, true);

    intervalTuning = juce::Array<float>({0.0, 0.117313, 0.039101, 0.156414, -0.13686, -0.019547, -0.174873, 0.019547, 0.136864, -0.15641, -0.311745, -0.11731});

    useLowestNoteForFundamental = false;
    useHighestNoteForFundamental = false;
    useLastNoteForFundamental = false;

    setFundamentalSetsTether(true);        //needs UI toggle; when on, don't show tether sliders for current notes, only the two tether weight sliders
    tetherFundamental = C;

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

    numNotes = 0;
//    if (st != nullptr) copy(st);

    /**
     * todo: remove true here
     */
    setRate(sparams.rate->getCurrentValue(), true);
    setIntervalFundamental((PitchClass)(automatic + 1));
}

SpringTuning::~SpringTuning()
{
    stopTimer();
    DBG("SpringTuning: stopping timer");
};

inline void SpringTuning::stop(void) { stopTimer(); DBG("SpringTuning: stopping timer");}

//void SpringTuning::copy(SpringTuning::Ptr st)
void SpringTuning::copy(SpringTuning* st)
{
    /**
     * todo: is this copying ok with the sparams? do we have a new sparams that these values are copying into?
     */
    // DBG("SpringTuning::copy called!!");
    sparams.rate->setParameterValue(st->getRate());
    sparams.stiffness->setParameterValue(st->getStiffness());
    sparams.active->setParameterValue(st->getActive());
    sparams.drag->setParameterValue(st->getDrag());
    sparams.intervalStiffness->setParameterValue(st->getIntervalStiffness());
    sparams.tetherStiffness->setParameterValue(st->getTetherStiffness());

    for (int i = 0; i < 13; i++)
    {
        springWeights[i] = st->springWeights[i];
    }

    for (int i=0; i<12; i++)
    {
        setSpringMode(i, st->getSpringMode(i));
    }

    sparams.scaleId->setParameterValue(st->getScaleId());

    setIntervalTuning(st->getIntervalTuning());
    setTetherTuning(st->getTetherTuning());

    setSpringWeights(st->getSpringWeights());
    setTetherWeights(st->getTetherWeights());

    setIntervalFundamental(st->getIntervalFundamental());
    //setUsingFundamentalForIntervalSprings(st->getUsingFundamentalForIntervalSprings());

    setFundamentalSetsTether(st->getFundamentalSetsTether());
    setTetherWeightGlobal(st->getTetherWeightGlobal());
    setTetherWeightSecondaryGlobal(st->getTetherWeightSecondaryGlobal());
}

inline void SpringTuning::setRate(double r, bool start)
{
    //rate = r;
    sparams.rate->setParameterValue(r);
    if (start) {
        startTimer(1000 / sparams.rate->getCurrentValue());
        DBG("SpringTuning: starting timer");
    }
    else {
        stopTimer();
        DBG("SpringTuning: stopping timer");
    }
}

inline void SpringTuning::setStiffness(double stiff)
{
    sparams.stiffness->setParameterValue(stiff);
    stiffnessChanged();
}

inline void SpringTuning::setTetherStiffness(double stiff)
{
    sparams.tetherStiffness->setParameterValue(stiff);
    tetherStiffnessChanged();
}

inline void SpringTuning::setIntervalStiffness(double stiff)
{
    sparams.intervalStiffness->setParameterValue(stiff);
    intervalStiffnessChanged();
}

void SpringTuning::stiffnessChanged()
{
    for (auto spring : enabledSpringArray)
    {
        spring->setStiffness(sparams.stiffness->getCurrentValue());
    }

    for (auto spring : tetherSpringArray)
    {
        spring->setStiffness(sparams.stiffness->getCurrentValue());
    }
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

inline double SpringTuning::getRate(void) { return sparams.rate->getCurrentValue(); }
inline double SpringTuning::getStiffness(void) { return sparams.stiffness->getCurrentValue(); }
inline double SpringTuning::getTetherStiffness(void) { return sparams.tetherStiffness->getCurrentValue(); }
inline double SpringTuning::getIntervalStiffness(void) { return sparams.intervalStiffness->getCurrentValue(); }
inline void SpringTuning::setDrag(double newdrag) { sparams.drag->setParameterValue(newdrag); }
inline double SpringTuning::getDrag(void) { return sparams.drag->getCurrentValue(); }
bool SpringTuning::getFundamentalSetsTether() { return sparams.fundamentalSetsTether->get(); }
void SpringTuning::setFundamentalSetsTether(bool s) { sparams.fundamentalSetsTether->setParameterValue(s); }
double SpringTuning::getTetherWeightGlobal() { return sparams.tetherWeightGlobal->getCurrentValue(); }
double SpringTuning::getTetherWeightSecondaryGlobal() { return sparams.tetherWeightSecondaryGlobal->getCurrentValue(); }
void SpringTuning::setTetherWeightGlobal(double s) { sparams.tetherWeightGlobal->setParameterValue(s); }
void SpringTuning::setTetherWeightSecondaryGlobal(double s) { sparams.tetherWeightSecondaryGlobal->setParameterValue(s); }
bool SpringTuning::getSpringMode(int which) {return springMode.getUnchecked(which);}
bool SpringTuning::getSpringModeButtonState(int which) {return springMode.getUnchecked(which);}
PitchClass SpringTuning::getIntervalFundamental(void) { return sparams.intervalFundamental->get(); }
PitchClass SpringTuning::getIntervalFundamentalActive(void) { return intervalFundamentalActive; }
juce::Array<float> SpringTuning::getIntervalTuning(void){return intervalTuning;}
PitchClass SpringTuning::getTetherFundamental(void) {return tetherFundamental;}
juce::Array<float> SpringTuning::getTetherTuning(void) {return tetherTuning;}
void SpringTuning::setUsingFundamentalForIntervalSprings(bool use) { usingFundamentalForIntervalSprings = use; }
bool SpringTuning::getUsingFundamentalForIntervalSprings(void) { return usingFundamentalForIntervalSprings; }
inline void SpringTuning::setScaleId(TuningSystem which) { sparams.scaleId->setParameterValue(which); }
inline void SpringTuning::setActive(bool status) { sparams.active->setParameterValue(status); }
inline bool SpringTuning::getActive(void) { return sparams.active->get(); }
inline TuningSystem SpringTuning::getScaleId(void) { return sparams.scaleId->get(); }

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


void SpringTuning::setTetherTuning(juce::Array<float> tuning)
{
    const juce::ScopedLock sl (lock);

    tetherTuning = tuning;

    for (int i = 0; i < 128; i++)
    {
        tetherParticleArray[i]->setX( (i * 100.0) + tetherTuning[(i-tetherFundamental) % 12] );
        tetherParticleArray[i]->setRestX( (i * 100.0) + tetherTuning[(i-tetherFundamental) % 12] );

        //DBG("rest X: " + String((i * 100.0) + tetherTuning[i % 12]));
        particleArray[i]->setRestX( (i * 100.0) + tetherTuning[(i-tetherFundamental) % 12] );
    }
}

void SpringTuning::setTetherFundamental(PitchClass newfundamental)
{
    tetherFundamental = newfundamental;
    setTetherTuning(getTetherTuning());
}

void SpringTuning::setIntervalTuning(juce::Array<float> tuning)
{
    intervalTuning = tuning;
}

void SpringTuning::setIntervalFundamental(PitchClass newfundamental)
{
    sparams.intervalFundamental->setParameterValue(newfundamental);
    intervalFundamentalChanged();
}
void SpringTuning::intervalFundamentalChanged()
{
    auto newfundamental = sparams.intervalFundamental->get();

    if(newfundamental < none) intervalFundamentalActive = newfundamental;

    if(newfundamental == none) setUsingFundamentalForIntervalSprings(false);
    else setUsingFundamentalForIntervalSprings(true);

    if(newfundamental == lowest) useLowestNoteForFundamental = true;
    else useLowestNoteForFundamental = false;

    if(newfundamental == highest) useHighestNoteForFundamental = true;
    else useHighestNoteForFundamental = false;

    if(newfundamental == last) useLastNoteForFundamental = true;
    else useLastNoteForFundamental = false;

    if(newfundamental == automatic) useAutomaticFundamental = true;
    else useAutomaticFundamental = false;

    setTetherFundamental(intervalFundamentalActive);
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

void SpringTuning::toggleSpring()
{
	//tbd
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

Particle* SpringTuning::getParticle(int note) { return particleArray[note];}
//Particle::Ptr SpringTuning::getParticle(int note) { return particleArray[note];}

void SpringTuning::addNote(int note)
{
    addParticle(note);
    DBG("SpringTuning::addNote, useAutomaticFundamental " + juce::String((int)useAutomaticFundamental));

    if(useLowestNoteForFundamental)
    {
        // DBG("lowest current note = " + String(getLowestActiveParticle()));
        intervalFundamentalActive = (PitchClass)(getLowestActiveParticle() % 12);

        if(sparams.fundamentalSetsTether->get()) setTetherFundamental(intervalFundamentalActive);
    }
    else if(useHighestNoteForFundamental)
    {
        // DBG("highest current note = " + String(getHighestActiveParticle()));
        intervalFundamentalActive = (PitchClass)(getHighestActiveParticle() % 12);

        if(sparams.fundamentalSetsTether->get()) setTetherFundamental(intervalFundamentalActive);
    }
    else if(useLastNoteForFundamental)
    {
        // DBG("last note = " + String(note));
        intervalFundamentalActive = (PitchClass)(note % 12);

        if(sparams.fundamentalSetsTether->get()) setTetherFundamental(intervalFundamentalActive);
    }
    else if(useAutomaticFundamental)
    {
        findFundamental(); //sets intervalFundamental internally
        if(sparams.fundamentalSetsTether->get()) setTetherFundamental(intervalFundamentalActive);
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
        intervalFundamentalActive = (PitchClass)(getLowestActiveParticle() % 12);
    }
    else if(useHighestNoteForFundamental)
    {
        // DBG("highest current note = " + String(getHighestActiveParticle()));
        intervalFundamentalActive = (PitchClass)(getHighestActiveParticle() % 12);
    }
    else if(useAutomaticFundamental)
    {
        findFundamental();
        if(sparams.fundamentalSetsTether->get()) setTetherFundamental(intervalFundamentalActive);
    }

    removeSpringsByNote(note);

    if(useLowestNoteForFundamental || useHighestNoteForFundamental || useAutomaticFundamental) retuneAllActiveSprings();
}

void SpringTuning::removeAllNotes(void)
{
    for (int i = 0; i < 128; i++) removeNote(i);
}

void SpringTuning::toggleNote(int noteIndex)
{
    const juce::ScopedLock sl (lock);

	int convertedIndex = noteIndex; // just in case a midi value is passed accidentally

	if (particleArray[convertedIndex]->getEnabled())
	{
		removeNote(convertedIndex);
	}
	else
	{
		addNote(convertedIndex);
	}
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
             //DBG("enabledNotes = " + String(i));
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
            // DBG("CHOICE = " + String(fundamental_57));
            intervalFundamentalActive = (PitchClass(fundamental_57));
        }
        else if(fundamental_48 > -1)
        {
            // DBG("CHOICE = " + String(fundamental_48));
            intervalFundamentalActive = (PitchClass(fundamental_48));

        }
        else if(fundamental_39 > -1)
        {
            // DBG("CHOICE = " + String(fundamental_39));
            intervalFundamentalActive = (PitchClass(fundamental_39));

        }
    }
}

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

void SpringTuning::addSpring(Spring* spring)
{
    const juce::ScopedLock sl (lock);

    if (enabledSpringArray.contains(spring)) return;
    int interval = spring->getIntervalIndex();

    spring->setEnabled(true);
    spring->setStiffness(sparams.intervalStiffness->getCurrentValue());
    spring->setStrength(springWeights[interval]);
    enabledSpringArray.add(spring);

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
                    //int intervalFundamental = 0; //temporary, will set in preparation

                    diff = fabs(100. *
                    ((scaleDegree1 +
                      intervalTuning.getUnchecked((scaleDegree1 - (int)intervalFundamentalActive) % 12)) -
                    (scaleDegree2 +
                     intervalTuning.getUnchecked((scaleDegree2 - (int)intervalFundamentalActive) % 12))));
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

    if(getFundamentalSetsTether())
    {
        for (auto tether : tetherSpringArray)
        {
            if(tether->getEnabled())
            {
                int tnoteA = tether->getA()->getNote();
                int tnoteB = tether->getB()->getNote();

                if(tnoteA % 12 == getTetherFundamental() || tnoteB % 12 == getTetherFundamental())
                {
                    // DBG("SpringTuning::addSpringsByNote getTetherWeightGlobal = " + String((int)getTetherWeightGlobal()));
                    setTetherWeight(tnoteA, getTetherWeightGlobal());
                }
                else{
                    setTetherWeight(tnoteA, getTetherWeightSecondaryGlobal());
                }
            }
        }
    }


}

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

void SpringTuning::retuneIndividualSpring(Spring* spring)
{
    int interval = spring->getIntervalIndex();

    //set spring length locally, for all if !usingFundamentalForIntervalSprings, or for individual springs as set by L/F
    if(!usingFundamentalForIntervalSprings ||
        !getSpringMode(interval - 1))
    {
        int diff = spring->getA()->getRestX() - spring->getB()->getRestX();
        spring->setRestingLength(fabs(diff) + intervalTuning[interval]);
    }

    //otherwise, set resting length to interval scale relative to intervalFundamental (F)
    else
    {
        int scaleDegree1 = spring->getA()->getNote();
        int scaleDegree2 = spring->getB()->getNote();
        //int intervalFundamental = 0; //temporary, will set in preparation

        float diff =    (100. * scaleDegree2 + intervalTuning[(scaleDegree2 - (int)intervalFundamentalActive) % 12]) -
                     (100. * scaleDegree1 + intervalTuning[(scaleDegree1 - (int)intervalFundamentalActive) % 12]);

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
//    Particle::PtrArr particles = tuning->getTuning()->prep->getParticles();
//      double x = particles[getCurrentlyPlayingKey()]->getX();
//    int octave = particles[getCurrentlyPlayingKey()]->getOctave();
//    //double transpOffset = (currentMidiNoteNumber - getCurrentlyPlayingKey()) * 100.;
//    double transpOffset = (cookedNote - getCurrentlyPlayingKey()) * 100.;
//    //DBG("BKPianoSamplerVoice::updatePitch cookedNote = " + String(cookedNote));
//        double midi = Utilities::clip(0, ftom(Utilities::centsToFreq((x + transpOffset) - 1200.0 * octave)


auto particles = getParticles();
    double x = particles[note]->getX();
    int octave = particles[note]->getOctave();
    //double transpOffset = (currentMidiNoteNumber - getCurrentlyPlayingKey()) * 100.;
    //double transpOffset = (cookedNote - getCurrentlyPlayingKey()) * 100.;
    //DBG("BKPianoSamplerVoice::updatePitch cookedNote = " + String(cookedNote));
        double midi = Utilities::centsToFreq(x - 1200.0 * octave);

        return midi;

	//return Utilities::centsToFreq((int) particleArray[note]->getX());
}

void SpringTuning::print()
{
    DBG("~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~");
	for (int i = 0; i < 128; i++)
	{
        juce::String printStatus = "";
		if (particleArray[i]->getEnabled())
        {
            DBG(juce::String(i));
        }
	}
}

void SpringTuning::printParticles()
{
    const juce::ScopedLock sl (lock);

	for (int i = 0; i < 128; i++)
	{
		if(particleArray[i]->getEnabled()) particleArray[i]->print();
	}
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

void SpringTuning::printActiveParticles()
{
    const juce::ScopedLock sl (lock);

	for (int i = 0; i < 128; i++)
	{
		if (particleArray[i]->getEnabled()) particleArray[i]->print();
	}
}

void SpringTuning::printActiveSprings()
{
    const juce::ScopedLock sl (lock);

	for (auto spring : springArray)
	{
		if (spring->getEnabled()) spring->print();
	}
}

bool SpringTuning::checkEnabledParticle(int index)
{
    const juce::ScopedLock sl (lock);
	return particleArray[index]->getEnabled();
}

//Particle::PtrArr& SpringTuning::getTetherParticles(void) { return tetherParticleArray;}
//Spring::PtrArr& SpringTuning::getTetherSprings(void) { return tetherSpringArray;}
//Particle::PtrArr& SpringTuning::getParticles(void) { return particleArray;}
//Spring::PtrMap& SpringTuning::getSprings(void) { return springArray; }
//Spring::PtrArr& SpringTuning::getEnabledSprings(void) { return enabledSpringArray;}

juce::Array<Particle*> SpringTuning::getTetherParticles(void) { return tetherParticleArray;}
juce::Array<Spring*> SpringTuning::getTetherSprings(void) { return tetherSpringArray;}
juce::Array<Particle*> SpringTuning::getParticles(void) { return particleArray;}
//juce::HashMap<int, Spring*> SpringTuning::getSprings(void) { return springArray; }
juce::Array<Spring*>SpringTuning::getEnabledSprings(void) { return enabledSpringArray;}

void SpringTuning::hiResTimerCallback(void)
{
    // DBG("Spring Tuning timer callback");
    if (sparams.active->get())
    {
        simulate();
    }
}

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