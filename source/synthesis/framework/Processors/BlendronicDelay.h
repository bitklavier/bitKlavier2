//
// Created by Dan Trueman on 7/29/25.
//

#ifndef BITKLAVIER0_BLENDRONICDELAY_H
#define BITKLAVIER0_BLENDRONICDELAY_H

#pragma once
#include <juce_dsp/juce_dsp.h>

/*
////////////////////////////////////////////////////////////////////////////////
   bitKlavier replacement for STK DelayL - limited implementation
////////////////////////////////////////////////////////////////////////////////
*/

class BKDelayL
{
public:
    //constructors
    BKDelayL();
    BKDelayL(float delayLength, int bufferSize, float delayGain, double sr);
    ~BKDelayL();

    //accessors
    inline const float getLength() const noexcept { return length; }
    inline const float getBufferSize() const noexcept { return bufferSize; }
    inline const float getGain() const noexcept { return gain; }
    inline const float lastOutLeft() const noexcept { return lastFrameLeft; }
    inline const float lastOutRight() const noexcept { return lastFrameRight; }

    inline const float getSample(int c, int i) const noexcept
    {
        if (loading) return 0.0f;
        return inputs.getSample(c, i);
    }

    inline const juce::AudioBuffer<float>* getBuffer() const noexcept { return &inputs; }

    //mutators
    void setLength(float delayLength);
    void setBufferSize(int size);
    inline void setGain(float delayGain) { gain = delayGain; }
    inline void setFeedback(float fb) { feedback = fb; }
    inline int getInPoint() { return inPoint; }
    inline int getOutPoint() { return outPoint; }

    float nextOutLeft();
    float nextOutRight();
    void addSample(float input, int offset, int channel);
    void scalePrevious(float coefficient, int offset, int channel);
    void tick(float* inL, float* inR);
    void clear();
    void reset();

    inline void setSampleRate(double sr) { sampleRate = sr; }

    juce::AudioBuffer<float> inputs;

private:
    juce::CriticalSection lock;

    int inPoint;
    int outPoint;
    int bufferSize;
    float length;
    float gain;
    float lastFrameLeft;
    float lastFrameRight;
    float alpha;
    float omAlpha;
    float feedback;
    float nextOutput;
    bool doNextOutLeft;
    bool doNextOutRight;

    bool loading;
    double sampleRate;
};

/*
////////////////////////////////////////////////////////////////////////////////
   bitKlavier replacement for STK envelope - limited implementation
////////////////////////////////////////////////////////////////////////////////
*/

class BKEnvelope
{
public:
    //constructors
    BKEnvelope();
    BKEnvelope(float bValue, float bDuration, double sr);
    ~BKEnvelope();

    //accessors
    inline const float getValue() const noexcept { return value; }
    inline const float getTarget() const noexcept { return target; }
    inline const int getState() const noexcept { return state; }

    //mutators
    inline void setValue(float envelopeValue) { value = envelopeValue; }
    inline void setTarget(float envelopeTarget) { target = envelopeTarget; state = (target != value) ? 1 : 0; }
    inline void setRate(float sr) { rate = sr; }
    inline void setTime(float time) { rate = 1.0 / ( time * sampleRate * 0.001 ); }//DBG("new rate = " + String(rate));} // time in ms for envelope to go from 0-1. need to update for sampleRate
    inline void setSampleRate(double sr) { sampleRate = sr; }

    //! Set target = 1.
    void keyOn( void ) { this->setTarget( 1.0 ); };

    //! Set target = 0.
    void keyOff( void ) { this->setTarget( 0.0 ); };

    float tick();
    float lastOut() { return lastvalue; }

private:
    float value;
    float lastvalue;
    float target;
    float rate;
    int state;

    double sampleRate;
};

class BlendronicDelay
{
public:
    BlendronicDelay(float delayLength, float smoothValue, float smoothDuration, int delayBufferSize, double sr);
    ~BlendronicDelay();

    inline const juce::AudioBuffer<float>* getDelayBuffer() const noexcept { return delayLinear->getBuffer(); }
    inline const int getInPoint() const noexcept { return delayLinear->getInPoint(); }

    inline void setDelayLength(float delayLength) { dDelayLength = delayLength; delayLinear->setLength(delayLength); }
    inline void setDelayTargetLength(float delayLength) { dSmooth->setTarget(delayLength); }
    inline void setFeedback(float fb) { delayLinear->setFeedback(fb); }
    inline void setSampleRate(double sr) { sampleRate = sr; delayLinear->setSampleRate(sr); dSmooth->setSampleRate(sr); }
    inline void clear() { delayLinear->clear(); /*delayLinear->reset();*/ }

    /*
     * leave for now, might have this as an available user param as in the old bK
     */
//    inline void setBufferSize(int bufferSize)
//    {
//        dBufferSize = bufferSize;
//        delayLinear->setBufferSize(dBufferSize);
//    }

    inline void setSmoothRate(float smoothRate)
    {
        dSmoothRate = smoothRate;
        dSmooth->setRate(smoothRate);
    }

    void scalePrevious(float coefficient, int offset, int channel);

    void tick(float* inL, float* inR);

private:
    std::unique_ptr<BKDelayL> delayLinear;
    std::unique_ptr<BKEnvelope> dSmooth;
    std::unique_ptr<BKEnvelope> dEnv;
    float dBufferSize;
    float dDelayGain;
    float dDelayLength;
    float dSmoothValue;
    float dSmoothRate;
//    bool dInputOpen;
//    bool dOutputOpen;
    bool shouldDuck;

    double sampleRate;
};

#endif //BITKLAVIER0_BLENDRONICDELAY_H
