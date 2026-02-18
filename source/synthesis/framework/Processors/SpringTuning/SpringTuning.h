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
 * todo: rewrite ALL the spring code to remove JUCE dependencies (other than the timer)
 *  so, no juce::Arrays, etc...
 *  would be nice for it to be more modular, and might have other benefits as well
 */

/**
 * todo: have this run on its own thread, so that its timer is independent from the UI
 *       - would also be nice to find a way for it to work properly if rendering in non-real-time
 *       - could possibly have it simply count samples in the processBlock and run simulate() on its own thread
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
    SpringTuning(SpringTuningParams &params, std::array<std::atomic<float>, 12> &circularTuningCustom);
    ~SpringTuning();

    /**
     * todo: add a callback for drag, to do the 1 - drag conversion?
     */

    /**
     * these first functions are callbacks for the UI, to update values internally
     */
    void rateChanged();
    void dragChanged();
    void tetherStiffnessChanged();
    void intervalStiffnessChanged();
    void intervalScaleChanged();
    void intervalFundamentalChanged();
    void tetherScaleChanged();
    void tetherFundamentalChanged();

    void setRate(double r, bool start = true);
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
    double getTetherFrequency(int index, float globalRefA4);
    bool getSpringMode(int which);
    double getFrequency(int index, float globalRefA4);
    PitchClass getTetherFundamental();

    juce::Array<Particle*> getParticles(void);
    int getLowestActiveParticle();
    int getHighestActiveParticle();
    double getSpringWeight(int which);

    void findFundamental();
    void updateTetherTuning();
    void retuneIndividualSpring(Spring* spring);
    void retuneAllActiveSprings(void);

    juce::Array<Spring*>& getTetherSprings(void) { return tetherSpringArray; }

    SpringTuningParams &sparams;
    std::array<std::atomic<float>, 12> &customTuning;
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

    float dragLocal; //internally, work with 1. - drag as given in the UI

    void hiResTimerCallback(void) override;
};