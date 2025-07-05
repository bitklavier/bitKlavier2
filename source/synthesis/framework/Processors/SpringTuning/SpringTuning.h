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
 * todo: rewrite ALL the spring code to remove JUCE dependencies
 *  would be nice for it to be more modular, and might have other benefits as well
 */

#pragma once
//#include "SpringTuningUtilities.h"
#include "Particle.h"
#include "Spring.h"
#include "SpringTuningParams.h"

//class Particle;
//class Spring;
class SpringTuning : public juce::ReferenceCountedObject, private juce::HighResolutionTimer
{
public:
    typedef juce::ReferenceCountedObjectPtr<SpringTuning> Ptr;

    SpringTuning(SpringTuning::Ptr st, SpringTuningParams &params);
    ~SpringTuning();
    void copy(SpringTuning::Ptr st);

    /*
     simulate() first moves through the entire particle array and "integrates" their position,
     moving them based on their "velocities" and the drag values

     it then moves through both spring arrays (the tether springs and interval springs) and
     calls satisfyConstraints(), which updates the spring values based on the spring strengths,
     stiffnesses, and offsets from their rest lengths. This in turn updates the target positions
     for the two particles associated with each spring.
     */

    void simulate();
	void toggleSpring();
	void addParticle(int pc);
	void removeParticle(int pc);
    Particle* getParticle(int note);
//    Particle::Ptr getParticle(int note);

	void addNote(int noteIndex);
	void removeNote(int noteIndex);
    void removeAllNotes(void);
	void toggleNote(int noteIndex);
	void updateNotes();
    void updateFreq();
	void addSpringsByNote(int pc);
	void removeSpringsByNote(int removeIndex);
	void addSpringsByInterval(double interval);
	void removeSpringsByInterval(double interval);
	void adjustSpringsByInterval(double interval, double stiffness);

    inline void stop(void);

    inline void setStiffness(double stiff);
    inline void setTetherStiffness(double stiff);
    inline void setIntervalStiffness(double stiff);
    void stiffnessChanged();
    void tetherStiffnessChanged();
    void intervalStiffnessChanged();

    inline juce::Array<float> getTetherWeights(void);
    inline void setTetherWeights(juce::Array<float> weights);
    inline juce::Array<float> getSpringWeights(void);
    inline void setSpringWeights(juce::Array<float> weights);
    inline void setSpringMode(int which, bool on);

    inline void setRate(double r, bool start = true);
    inline double getRate(void);
    inline double getStiffness(void);
    inline double getTetherStiffness(void);
    inline double getIntervalStiffness(void);
    inline void setDrag(double newdrag);
    inline double getDrag(void);
    bool getFundamentalSetsTether();
    void setFundamentalSetsTether(bool s);
    double getTetherWeightGlobal();
    double getTetherWeightSecondaryGlobal();
    void setTetherWeightGlobal(double s);
    void setTetherWeightSecondaryGlobal(double s);
    bool getSpringMode(int which);
    bool getSpringModeButtonState(int which);
    PitchClass getIntervalFundamental(void);
    PitchClass getIntervalFundamentalActive(void);
    juce::Array<float> getIntervalTuning(void);
    PitchClass getTetherFundamental(void);
    juce::Array<float> getTetherTuning(void);
    void setUsingFundamentalForIntervalSprings(bool use);
    bool getUsingFundamentalForIntervalSprings(void);
    inline void setScaleId(TuningSystem which);
    inline void setActive(bool status);
    inline bool getActive(void);
    inline TuningSystem getScaleId(void);

	double getFrequency(int index);
	bool pitchEnabled(int index);

	void print();
	void printParticles();
	void printActiveParticles();
	void printActiveSprings();

	bool checkEnabledParticle(int index);

    Particle::PtrArr& getTetherParticles(void);
    Spring::PtrArr& getTetherSprings(void);
    Particle::PtrArr& getParticles(void);
    Spring::PtrMap& getSprings(void);
    Spring::PtrArr& getEnabledSprings(void);
    juce::String getTetherSpringName(int which);
    juce::String getSpringName(int which);

    void setTetherTuning(juce::Array<float> tuning);
    void setTetherFundamental(PitchClass newfundamental);
    void setIntervalTuning(juce::Array<float> tuning);
    void setIntervalFundamental(PitchClass newfundamental);
    void intervalFundamentalChanged();
    void findFundamental();
    void retuneIndividualSpring(Spring::Ptr spring);
    void retuneAllActiveSprings(void);

    void setSpringWeight(int which, double weight);
    double getSpringWeight(int which);

    void setTetherWeight(int which, double weight);
    double getTetherWeight(int which);

    int getLowestActiveParticle();
    int getHighestActiveParticle();

    SpringTuningParams &sparams;

private:
    juce::CriticalSection lock;

    int numNotes; // number of enabled notes
    bool usingFundamentalForIntervalSprings; //only false when in "none" mode
    bool useLowestNoteForFundamental;
    bool useHighestNoteForFundamental;
    bool useLastNoteForFundamental;
    bool useAutomaticFundamental; //uses findFundamental() to set fundamental automatically, based on what is played

    juce::Array<float> intervalTuning;
    PitchClass intervalFundamentalActive; //one actually used in the moment, changed by auto/last/highest/lowest modes
    juce::Array<bool> springMode;

    juce::Array<float> tetherTuning;
    TuningSystem tetherTuningId;
    PitchClass tetherFundamental;

    Particle::PtrArr    particleArray;
    Spring::PtrMap      springArray; // efficiency fix: make this ordered by spring interval

    Particle::PtrArr    tetherParticleArray;
    Spring::PtrArr      tetherSpringArray;

    Spring::PtrArr      enabledSpringArray;
    Spring::PtrArr      enabledParticleArray;

    float springWeights[13];
    void addSpring(Spring::Ptr spring);

    void hiResTimerCallback(void) override;
};
