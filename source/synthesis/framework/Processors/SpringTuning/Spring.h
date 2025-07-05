/*
  ==============================================================================

    Spring.h
    Created: 3 Aug 2018 3:44:01pm
    Author:  Theo Trevisan, Mike Mulshine, Dan Trueman
    ------------------------------------------------------------------------

    Based on the Verlet mass/spring algorithm:
    Jakobsen, T. (2001). Advanced character physics.
        In IN PROCEEDINGS OF THE GAME DEVELOPERS CONFERENCE 2001, page 19.

    A Spring consists of two Particles set a RestingLength distance apart.
    The Spring will have a given Strength and Stiffness.

    The satisfyConstraints() method is where the main action happens, along with the
    Particle->integrate() method, which precedes the satisfyConstraints() call

  ==============================================================================
*/
#pragma once
#include "Particle.h"

//class Particle;
class Spring : public juce::ReferenceCountedObject
{
public:
    typedef juce::ReferenceCountedObjectPtr<Spring> Ptr;
    typedef juce::Array<Spring::Ptr> PtrArr;
    typedef juce::HashMap<int, Spring::Ptr> PtrMap;

    //Spring(Particle* firstPoint, Particle* secondPoint, double length, double str, int index, const juce::String& name, bool enabled);
	Spring(Particle::Ptr firstPoint, Particle::Ptr secondPoint, double length, double str, int index, const juce::String& name, bool enabled);

    Spring::Ptr copy();
//    std::shared_ptr<Spring> copy();
	bool compare(Spring* that);
	void print();

    Particle* getA();
    Particle* getB();

    int getIntervalIndex() { return intervalIndex; }
    inline void setAdjustedStrength(double strength) { adjustedStrength = strength; }
    inline double getAdjustedStrength() { return adjustedStrength; }
    void setRestingLength(double len) { restingLength = len; }
    double getRestingLength(void) { return restingLength; }
    void setLength(double len) { length = len; }
    double getLength(void) { return length; }

    bool getEnabled(void) { return enabled; }
    void setEnabled(bool e) { enabled = e; }
    void setName(juce::String s) { name = s;}
    juce::String getName(void) { return name;}

    /*
     in setStrength(), an adjustedStrength is calculated, based on both the given Strength
     parameter and the the given Stiffness parameter. It is scaled non-linearly to make the
     exposed parameter more intuitive and usable.
     */
    void setStrength(double newStrength);
    double getStrength() { return strength; }
    inline void setStiffness(double stiff) { stiffness = stiff; }
    inline double getStiffness(void) { return stiffness; }

    /*
     satisfyConstraints() is the main method in Spring
     It calculates the difference in position between the two particles,
     compares that to the defined resting distance between the particles,
     and then adjusts their target positions (unless one or both is locked).
     */
	void satisfyConstraints(void);

private:
	Particle* a;
	Particle* b;

	double strength;
    double adjustedStrength;
    double stiffness;
    double oldStiffness;

	double restingLength, length;
	int intervalIndex;
    juce::String name;
    bool enabled;
};

#pragma once
