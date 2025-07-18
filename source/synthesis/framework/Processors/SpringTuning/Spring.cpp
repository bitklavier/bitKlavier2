/*
  ==============================================================================

    Spring.cpp
    Created: 3 Aug 2018 3:44:01pm
    Author:  Theo

  ==============================================================================
*/

#include "Spring.h"

//Spring::Spring(Particle* firstPoint, Particle* secondPoint, double length, double str, int index, const juce::String& name, bool enabled) :

//Spring::Spring(Particle::Ptr firstPoint, Particle::Ptr secondPoint, double length, double str, int index, const juce::String& name, bool enabled) :
Spring::Spring(Particle* firstPoint, Particle* secondPoint, double length, double str, int index, const juce::String& name, bool enabled) :
	a(firstPoint),
	b(secondPoint),
    strength(str),
	restingLength(length),
	intervalIndex(index),
    name(name),
    enabled(enabled)
{

}

//Spring::Ptr Spring::copy(void)
//{
//    Spring::Ptr copySpring = new Spring(a, b, restingLength, strength, intervalIndex, name, enabled);
//	return copySpring;
//}

std::unique_ptr<Spring> Spring::copy(void)
//Spring* Spring::copy(void)
{
    //Spring* scopy = new Spring(a, b, restingLength, strength, intervalIndex, name, enabled);
    std::unique_ptr<Spring> scopy = std::make_unique<Spring>(a, b, restingLength, strength, intervalIndex, name, enabled);
    return scopy;
}

bool Spring::compare(Spring* that)
{
	return (a->compare(that->getA()) &&
		b->compare(that->getB()) &&
		strength == that->getStrength());
}

void Spring::print()
{
	DBG("Point A:");
	a->print();
	DBG("\nPoint B:");
	b->print();
	DBG("Strength: " + juce::String(strength));
}

Particle* Spring::getA() { return a; }
Particle* Spring::getB() { return b; }

void Spring::setStrength(double newStrength)
{
	strength = newStrength;

    // adjusted strength is scaled non-linearly to make the exposed parameter more intuitive and usable
    double warpCoeff = 100.;
    adjustedStrength = 0.6 * stiffness * (pow(warpCoeff, strength) - 1.) / (warpCoeff - 1.); //replace with dt_asymwarp, for clarity
    // > ~0.6 and the system can become unstable...
}

/*
 This is where the action happens. The positions of the two particles attached to the spring
 are adjusted, based on how much their separation diverges from the preset restingLength,
 and also by the preset spring strength. Essentially this is a force factor that results
 in a change in position, simplified from position = 0.5 * acceleration * t^2,
 and acceleration = force/mass
 */
void Spring::satisfyConstraints(void)
{
    // difference of the particle positions; basically the current length of the spring, signed
    double diff = b->getX() - a->getX();

    // length of spring; don't need this here, but other functions will need it, so we update
    length = abs(diff);

	if (diff == 0.0) return;

    // update the strength factor, if stiffness has changed
    if(stiffness != oldStiffness)
    {
        oldStiffness = stiffness;
        setStrength(strength);
    }

    // adjust the spring length based on how far it is from the resting length, and how strong it is
    double increment = (diff - restingLength) * adjustedStrength;

    // send the new positions to the particles, if they are not locked
    // we are essentially applying a force to the particles here
    if (!a->getLocked())
    {
        a->addX(increment);
    }
    if (!b->getLocked())
    {
        b->subX(increment);
    }

}
