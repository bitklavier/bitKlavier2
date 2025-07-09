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
     * this are callbacks for the UI, to update values internally
     *  - note that "drag" does not need a callback, since it can be used directly without anything further
     *  - all the rest of these require additional setup
     */
    void rateChanged();
    void tetherStiffnessChanged();
    void intervalStiffnessChanged();
    void intervalScaleChanged();
    void intervalFundamentalChanged();
    void tetherScaleChanged();
    void tetherFundamentalChanged();

    void simulate();
    inline void stop(void);

	void addParticle(int pc);
	void removeParticle(int pc);
    Particle* getParticle(int note);

	void addNote(int noteIndex);
	void removeNote(int noteIndex);
    void removeAllNotes(void);
	void addSpringsByNote(int pc);
	void removeSpringsByNote(int removeIndex);

    inline void setTetherWeights(juce::Array<float> weights);
    inline void setSpringMode(int which, bool on);
    inline void setRate(double r, bool start = true);
    inline void setScaleId(TuningSystem which);
    inline void setActive(bool status);
    void setSpringWeight(int which, double weight);
    void setTetherWeight(int which, double weight);

    inline juce::Array<float> getTetherWeights(void);
    inline juce::Array<float> getSpringWeights(void);
    double getTetherWeightGlobal();
    double getTetherWeightSecondaryGlobal();
    bool getSpringMode(int which);
    double getFrequency(int index);
    PitchClass getTetherFundamental();
    juce::Array<Particle*> getTetherParticles(void);
    juce::Array<Spring*> getTetherSprings(void);
    juce::Array<Particle*> getParticles(void);
    juce::Array<Spring*> getEnabledSprings(void);
    juce::String getTetherSpringName(int which);
    juce::String getSpringName(int which);
    double getTetherWeight(int which);
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
    juce::Array<bool> springMode;

    juce::Array<float> tetherTuning;
    TuningSystem tetherTuningId;
    PitchClass tetherFundamental;

    juce::Array<Particle*> particleArray;
    juce::HashMap<int, Spring*> springArray; // efficiency fix: make this ordered by spring interval

    juce::Array<Particle*>  tetherParticleArray;
    juce::Array<Spring*>    tetherSpringArray;
    juce::Array<Spring*>    enabledSpringArray;
    juce::Array<Spring*>    enabledParticleArray;

    float springWeights[13];
    void addSpring(Spring* spring);

    void hiResTimerCallback(void) override;
};



//********* below not needed? *********//

//	void toggleNote(int noteIndex);
//	void updateNotes();
//    void updateFreq();

//	void addSpringsByInterval(double interval);
//	void removeSpringsByInterval(double interval);
//	void adjustSpringsByInterval(double interval, double stiffness);


//    inline void setStiffness(double stiff);
//inline void setTetherStiffness(double stiff);
//inline void setIntervalStiffness(double stiff);
//    void stiffnessChanged();

//	void toggleSpring();

//    void setTetherFundamental(PitchClass newfundamental);
//    void setIntervalTuning(juce::Array<float> tuning);
//    void setIntervalFundamental(Fundamental newfundamental);

//    inline double getRate(void);
//    inline double getStiffness(void);
//    inline double getTetherStiffness(void);
//    inline double getIntervalStiffness(void);
//    inline void setDrag(double newdrag);
//    inline double getDrag(void);
//    bool getFundamentalSetsTether();
//    void setFundamentalSetsTether(bool s);

//    void setTetherWeightGlobal(double s);
//    void setTetherWeightSecondaryGlobal(double s);

//    bool getSpringModeButtonState(int which);
//    Fundamental getIntervalFundamental();
//    PitchClass getIntervalFundamentalActive();
//    juce::Array<float> getIntervalTuning(void);

//juce::Array<float> getTetherTuning(void);
//    void setUsingFundamentalForIntervalSprings(bool use);
//    bool getUsingFundamentalForIntervalSprings(void);

//	void printParticles();
//	void printActiveParticles();
//	void printActiveSprings();
//
//	bool checkEnabledParticle(int index);

//    Particle::PtrArr& getTetherParticles(void);

//    inline bool getActive(void);
//    inline TuningSystem getScaleId(void);

//	bool pitchEnabled(int index);

//    void retuneIndividualSpring(Spring::Ptr spring);

//    inline void setSpringWeights(juce::Array<float> weights);