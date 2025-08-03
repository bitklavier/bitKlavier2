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
    /**
     * todo: check to make sure sampleRate is being updated correctly
     *          - i'm assuming the constructor is called every time the sample rate is changed?
     */
    delayLinear = std::make_unique<BKDelayL>(dDelayLength, dBufferSize, dDelayGain, sampleRate);
    dSmooth = std::make_unique<BKEnvelope>(dSmoothValue, dDelayLength, sampleRate);
    dSmooth->setRate(dSmoothRate);

    DBG("Create bdelay");
}

BlendronicDelay::~BlendronicDelay()
{
    DBG("Destroy bdelay");
}

void BlendronicDelay::scalePrevious(float coefficient, int offset, int channel)
{
    delayLinear->scalePrevious(coefficient, offset, channel);
}

void BlendronicDelay::tick(float* inL, float* inR)
{
    setDelayLength(dSmooth->tick());
    delayLinear->tick(inL, inR);
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
    nextOutput = inputs.getSample(0, outPoint) * omAlpha;
    if (outPoint + 1 < inputs.getNumSamples())
        nextOutput += inputs.getSample(0, outPoint + 1) * alpha;
    else
        nextOutput += inputs.getSample(0, 0) * alpha;

    return nextOutput;
}

float BKDelayL::nextOutRight()
{
    nextOutput = inputs.getSample(1, outPoint) * omAlpha;
    if (outPoint + 1 < inputs.getNumSamples())
        nextOutput += inputs.getSample(1, outPoint + 1) * alpha;
    else
        nextOutput += inputs.getSample(1, 0) * alpha;

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
    // check the inPoint boundary
    if (inPoint >= inputs.getNumSamples()) inPoint = 0;

    /*
     * if the input is not open, we don't add samples to the delay.
     *  it still runs, however, continuing to process what it already had,
     *  sending it out (if dOutputOpen is true, below)
     */
    if(dInputOpen)
    {
        inputs.addSample(0, inPoint, *inL);
        inputs.addSample(1, inPoint, *inR);
    }

    // get our delayed outputs from both channels
    lastFrameLeft = nextOutLeft();
    lastFrameRight = nextOutRight();

    // increment the position samples are read from and check its end boundary
    if (++outPoint >= inputs.getNumSamples()) outPoint = 0;

    // feedback of last output is added here now
    inputs.addSample(0, inPoint, lastFrameLeft * feedback);
    inputs.addSample(1, inPoint, lastFrameRight * feedback);

    // increment the position samples are written to
    inPoint++;

    /*
     * if the output is not open, we leave the samples passing through untouched
     *  internally, the delay still runs, taking input samples, feeding back, etc...
     *  but we just don't send anything out
     */
    if(dOutputOpen)
    {
        *inL = lastFrameLeft;
        *inR = lastFrameRight;
    }
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
