//
// Created by Dan Trueman on 7/29/25.
//

#include "BlendronicDelay.h"

BlendronicDelay::BlendronicDelay(float delayLength, float smoothValue, float smoothRate, int delayBufferSize, double sr) :
     dBufferSize(delayBufferSize),
     dDelayGain(1.0f),
     dDelayLength(delayLength),
     dSmoothValue(smoothValue),
     dSmoothRate(smoothRate),
     sampleRate(sr)
{
    delayLinear = std::make_unique<BKDelayL>(dDelayLength, dBufferSize, dDelayGain, sampleRate);
    dSmooth = std::make_unique<BKEnvelope>(dSmoothValue, dDelayLength, sampleRate);
    dSmooth->setRate(dSmoothRate);
    dEnv = std::make_unique<BKEnvelope>(1.0f, 1.0f, sampleRate);
    dEnv->setTime(5.0f);

//    shouldDuck = false;

    DBG("Create bdelay");
}

BlendronicDelay::~BlendronicDelay()
{
    DBG("Destroy bdelay");
}

void BlendronicDelay::scalePrevious(float coefficient, int offset, int channel)
{
//    if (loading) return;
    delayLinear->scalePrevious(coefficient, offset, channel);
}

void BlendronicDelay::tick(float* inL, float* inR)
{
    /**
     * todo: check to see if we still need this dEnv stuff
     * todo: confirm we don't need 'shouldDuck'
     * todo: deal with dOutputOpen, dInputOpen...
     */

    setDelayLength(dSmooth->tick());
    float env = dEnv->tick();

    delayLinear->tick(inL, inR);
    *inL *= env;
    *inR *= env;
}


/*
////////////////////////////////////////////////////////////////////////////////
   bitKlavier replacement for STK DelayL - limited implementation
////////////////////////////////////////////////////////////////////////////////
*/
BKDelayL::BKDelayL() :
       inPoint(0),
       outPoint(0),
       bufferSize(44100.),
       length(0.0),
       gain(1.0),
       lastFrameLeft(0),
       lastFrameRight(0),
       feedback(0.9),
       doNextOutLeft(false),
       doNextOutRight(false),
       loading(false),
       sampleRate(44100.)
{
    inputs = juce::AudioBuffer<float>(2, bufferSize);
    inputs.clear();
    setLength(0.0);
}

BKDelayL::BKDelayL(float delayLength, int bufferSize, float delayGain, double sr) :
     inPoint(0),
     outPoint(0),
     bufferSize(bufferSize),
     length(delayLength),
     gain(delayGain),
     lastFrameLeft(0),
     lastFrameRight(0),
     doNextOutLeft(false),
     doNextOutRight(false),
     loading(false),
     sampleRate(sr)
{
    inputs = juce::AudioBuffer<float>(2, bufferSize);
    inputs.clear();
    setLength(delayLength);
    feedback = 0.9;
}

BKDelayL::~BKDelayL()
{
}

void BKDelayL::setLength(float delayLength)
{
    length = delayLength;
    float outPointer = inPoint - length;
    if (inputs.getNumSamples() > 0)
        while (outPointer < 0) outPointer += inputs.getNumSamples();
    else outPointer = 0;

    outPoint = outPointer; //integer part
    alpha = outPointer - outPoint; //fractional part
    omAlpha = (float)1.0 - alpha;
    if (outPoint == inputs.getNumSamples()) outPoint = 0;
    doNextOutLeft = true;
    doNextOutRight = true;
}

void BKDelayL::setBufferSize(int size)
{
    const juce::ScopedLock sl (lock);
    loading = true;
    inputs.setSize(2, bufferSize);
    reset();
    bufferSize = size;
    inputs.clear();
    loading = false;
}

/**
 * todo: combine these into one function with channel arg
 * @return
 */
float BKDelayL::nextOutLeft()
{
//    if (doNextOutLeft)
    {
        nextOutput = inputs.getSample(0, outPoint) * omAlpha;
        if (outPoint + 1 < inputs.getNumSamples())
            nextOutput += inputs.getSample(0, outPoint + 1) * alpha;
        else
            nextOutput += inputs.getSample(0, 0) * alpha;
//        doNextOutLeft = false;
    }

    return nextOutput;
}

float BKDelayL::nextOutRight()
{
//    if (doNextOutRight)
    {
        nextOutput = inputs.getSample(1, outPoint) * omAlpha;
        if (outPoint + 1 < inputs.getNumSamples())
            nextOutput += inputs.getSample(1, outPoint + 1) * alpha;
        else
            nextOutput += inputs.getSample(1, 0) * alpha;
//        doNextOutRight = false;
    }

    return nextOutput;
}

//allows addition of samples without incrementing delay position value
void BKDelayL::addSample(float input, int offset, int channel)
{
    inputs.addSample(channel, (inPoint + offset) % inputs.getNumSamples(), input);
}

// used for clearing the oldest part of the buffer so that it doesn't linger
void BKDelayL::scalePrevious(float coefficient, int offset, int channel)
{
    //    if (loading) return;
    inputs.setSample(channel, (inPoint + offset) % inputs.getNumSamples(), inputs.getSample(channel, (inPoint + offset) % inputs.getNumSamples()) * coefficient);
}

void BKDelayL::tick(float* inL, float* inR)
{
    /**
     * todo: check on 'loading' and if we need it
     */
//    if (loading) return;

    if (inPoint >= inputs.getNumSamples()) inPoint = 0;

    /**
     * todo: is this doNext stuff for freezing? omit for now...
     */
    lastFrameLeft = nextOutLeft();
//    doNextOutLeft = true;
    lastFrameRight = nextOutRight();
//    doNextOutRight = true;

    if (++outPoint >= inputs.getNumSamples()) outPoint = 0;

    //add the current sample and feedback the last output as well
    inputs.addSample(0, inPoint, *inL + lastFrameLeft * feedback);
    inputs.addSample(1, inPoint, *inR + lastFrameRight * feedback);

    inPoint++;

    *inL = lastFrameLeft;
    *inR = lastFrameRight;
}

void BKDelayL::clear()
{
    inputs.clear();
}

void BKDelayL::reset()
{
    inPoint = 0;
    outPoint = 0;
    float outPointer = inPoint - length;
    while (outPointer < 0) outPointer += inputs.getNumSamples();

    outPoint = outPointer; //integer part
    alpha = outPointer - outPoint; //fractional part
    omAlpha = (float)1.0 - alpha;
    if (outPoint == inputs.getNumSamples()) outPoint = 0;
//    doNextOutLeft = true;
//    doNextOutRight = true;
}

/*
////////////////////////////////////////////////////////////////////////////////
   bitKlavier replacement for STK envelope - limited implementation
////////////////////////////////////////////////////////////////////////////////
*/
BKEnvelope::BKEnvelope() :
                           value(0.0f),
                           target(0.0f),
                           rate(1.0f),
                           sampleRate(44100.)
{
    state = 0;
}

BKEnvelope::BKEnvelope(float bValue, float bTarget, double sr) :
                                                                  value(bValue),
                                                                  target(bTarget),
                                                                  rate(1.0f),
                                                                  sampleRate(sr)
{
    state = 0;
}

BKEnvelope::~BKEnvelope()
{
}

float BKEnvelope::tick()
{
    if (state == 1) {
        if (target > value) {
            value += rate;
            if (value >= target) {
                value = target;
                state = 0;
            }
        }
        else {
            value -= rate;
            if (value <= target) {
                value = target;
                state = 0;
            }
        }
    }

    lastvalue = value;
    return value;
}
