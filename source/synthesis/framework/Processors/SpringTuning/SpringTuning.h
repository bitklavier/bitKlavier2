/*
  ==============================================================================

    SpringTuning.h
    Created: 3 Aug 2018 3:43:46pm
    Author:  Theo Trevisan, Mike Mulshine, Dan Trueman

    Based on the Verlet mass/spring algorithm:
    Jakobsen, T. (2001). Advanced character physics.
        In PROCEEDINGS OF THE GAME DEVELOPERS CONFERENCE 2001, page 19.

    in bitKlavier, we use two arrays of springs:
        * the interval springs, which connect all active notes to one another
            at rest lengths given by whatever interval tuning system is being used
        * the tether springs, which connect all active notes to an array of "anchor"
            notes, set by an "anchor" scale (usually ET), to prevent uncontrolled drift

    The user sets the strengths of all these strings, either directly, or using some
    automated algorithms, and the system runs, adding and subtracting notes/springs
    from these arrays as they are played.

    For more information, see:
        "Playing with Tuning in bitKlavier"
        Trueman, Bhatia, Mulshine, Trevisan
        Computer Music Journal, 2020

    The main method is simulate(), which is called repeatedly in a timer()

  ==============================================================================
*/

/**
 * todo: implement individual interval weights, and whether to use Local or Fundamental springs
 */

/**
 * todo: rewrite ALL the spring code to remove JUCE dependencies
 *  would be nice for it to be more modular, and might have other benefits as well
 */

#pragma once
#include "SpringTuningUtilities.h"
#include "Particle.h"
#include "Spring.h"
#include "SpringTuningParams.h"

class Particle;
class Spring;
class SpringTuning : private juce::HighResolutionTimer
{
public:
    SpringTuning(SpringTuningParams &params);
    ~SpringTuning();

    /**
     * these first functions are callbacks for the UI, to update values internally
     *  - note that "drag" does not need a callback, since it can be used directly without anything further
     *      there are others like that as well...
     *  - all the rest of these require additional setup
     */
    void rateChanged();
    void tetherStiffnessChanged();
    void intervalStiffnessChanged();
    void intervalScaleChanged();
    void intervalFundamentalChanged();
    void tetherScaleChanged();
    void tetherFundamentalChanged();

    inline void setRate(double r, bool start = true);
    inline void setActive(bool status);

    void simulate();
    inline void stop(void);

	void addParticle(int pc);
	void removeParticle(int pc);

	void addNote(int noteIndex);
	void removeNote(int noteIndex);
    void addSpring(Spring* spring);
	void addSpringsByNote(int pc);
	void removeSpringsByNote(int removeIndex);

    void setTetherWeight(int which, double weight);
    double getTetherWeightGlobal();
    double getTetherWeightSecondaryGlobal();
    bool getSpringMode(int which);
    double getFrequency(int index);
    PitchClass getTetherFundamental();

    juce::Array<Particle*> getParticles(void);
    int getLowestActiveParticle();
    int getHighestActiveParticle();
    double getSpringWeight(int which);

    void findFundamental();
    void updateTetherTuning();
    void retuneIndividualSpring(Spring* spring);
    void retuneAllActiveSprings(void);

    SpringTuningParams &sparams;
    void print();


private:
    juce::CriticalSection lock;

    bool usingFundamentalForIntervalSprings; //only false when in "none" mode
    bool useLowestNoteForFundamental;
    bool useHighestNoteForFundamental;
    bool useLastNoteForFundamental;
    bool useAutomaticFundamental; //uses findFundamental() to set fundamental automatically, based on what is played

    juce::Array<float> intervalTuning;
    PitchClass intervalFundamentalActive; //the one actually used currently, changed by auto/last/highest/lowest modes

    juce::Array<float> tetherTuning;
    PitchClass tetherFundamental;

    juce::Array<Particle*> particleArray;
    juce::HashMap<int, Spring*> springArray; // efficiency fix: make this ordered by spring interval

    juce::Array<Particle*>  tetherParticleArray;
    juce::Array<Spring*>    tetherSpringArray;
    juce::Array<Spring*>    enabledSpringArray;
    juce::Array<Spring*>    enabledParticleArray;

    void hiResTimerCallback(void) override;
};