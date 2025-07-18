/*
  ==============================================================================

    Particle.h
    Created: 3 Aug 2018 3:44:24pm
    Author:  Theo Trevisan, Mike Mulshine, Dan Trueman

    Based on the Verlet mass/spring algorithm:
    Jakobsen, T. (2001). Advanced character physics.
        In IN PROCEEDINGS OF THE GAME DEVELOPERS CONFERENCE 2001, page 19.

    addX() and subX() update the position of the particle, usually to reflect the
    force being applied to the particle (as called by satisfyConstraints()) by,
    in this case, a virtual spring.

    integrate() also changes the position, based on the difference between
    the new position and the old position, scaled by the drag value.
    this is essentially velocity * time, where velocity is proportional to the
    change in position (new - old), and time is set by the call rate (so is implicit).

  ==============================================================================
*/

#pragma once
#include "SpringTuningUtilities.h"
//#include <juce_core/juce_core.h>

//class Particle : public juce::ReferenceCountedObject
//{
//public:
//
//    typedef juce::ReferenceCountedObjectPtr<Particle> Ptr;
//    typedef juce::Array<Particle::Ptr> PtrArr;
class Particle
{
public:

	Particle(double xVal, int n, juce::String s);

    void setRestX(double);
    double getRestX();

    void setX(double);
	double getX();

//    Particle::Ptr copy();
//    Particle* copy();
    std::unique_ptr<Particle> copy();
	bool compare(Particle* that);
	void print();

    /*
     addX and subX adjust the position for the particle;
     essentially this is being used to simulate the force that is
     being applied to the particle, called in satisfyConstraints()
     */
	void addX(double that);
	void subX(double that);

    /*
     integrate() actually changes the position of the particle,
     based on the difference between the new position and the old position,
     scaled by the drag value. essentially, this is velocity (diff) * time (implicit)
     */
	void integrate(double drag);

    bool getEnabled(void)   { return enabled; }
    void setEnabled(bool e) { enabled = e; }

	void confirmEnabled();

    void setNote(int newNote) { note = newNote;}
    int getNote(void){return note;}

    bool getLocked(void) {return locked;}
    void setLocked(bool lock) { locked = lock;}

    void setName(juce::String s) { name = s;}
    juce::String getName(void) { return name;}

    void setOctave(int o) { octave = o;}
    int getOctave(void) { return octave;}

private:
	double x;
    int octave;
    double restX;
	double prevX;
    bool enabled;
    bool locked;
    int note;
    juce::String name;
};
