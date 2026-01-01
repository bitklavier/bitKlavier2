//
// Created by Myra Norton on 12/22/25.
//

#pragma once
#include "FullInterface.h"
#include "PreparationSection.h"
#include "CompressorProcessor.h"

// valueInDecibel = params.
class MeterNeedle : public juce::Component, public juce::AsyncUpdater
{
public:
    MeterNeedle()
    {
        valueInDecibel = 0.0f;
        minValue = -80;
        maxValue = 0;
        sAngle = 0.0f;
        eAngle = 0.0f;
        needleColour = juce::Colour(0xff00838f);
    };

    void prepare(const float& s, const float& e)
    {
        sAngle = s;
        eAngle = e;
    };

    void paint(juce::Graphics& g) override
    {
        DBG("MeterNeedle::paint");

        const auto bounds = area.toFloat();
        const float centreX = bounds.getX() + bounds.getWidth() * 0.5f;
        const float centreY = bounds.getY() + bounds.getHeight();
        const float needleLength = juce::jmin(bounds.getWidth() * 0.75f, bounds.getHeight() * 0.75f);

        g.setColour(needleColour);
        redrawNeedle(g, centreX, centreY, needleLength);
    };

    void resized() override
    {
        area = getLocalBounds().reduced(3);
        repaint();
    };

    void update(const float& val)
    {
        if (val != valueInDecibel)
        {
            DBG("needle showing=" << (int) isShowing()
            << " visible=" << (int) isVisible()
            << " bounds=" << getBounds().toString());

            valueInDecibel = val;
        }

        repaint();
        DBG("update needle, dB = " << val);
        triggerAsyncUpdate();
    };

    void redrawNeedle(juce::Graphics& g, float centreX, float centreY, float length)
    {
        DBG("redraw needle");
        float val = std::clamp(valueInDecibel, static_cast<float>(minValue), static_cast<float>(maxValue));
        float mapped = juce::jmap(val, static_cast<float>(minValue), static_cast<float>(maxValue), sAngle, eAngle);
        mapped -= mapped > 2 * juce::MathConstants<float>::pi ? juce::MathConstants<float>::twoPi : 0.0f;
        const float x2 = centreX + sin(mapped) * length;
        const float y2 = centreY - cos(mapped) * length;
        g.drawArrow({centreX, centreY, x2, y2}, 2.0f, 0, 0);
    };

    void setNeedleEnabled(bool state)
    {
        needleColour = state
                   ? juce::Colour(0xff00838f)
                   : juce::Colour(statusOutline).withAlpha(0.35f);
        repaint();
    };

private:

    void handleAsyncUpdate() override
    {
        DBG("MeterNeedle::handleAsyncUpdate repaint chain");
        if (auto* p = getParentComponent())
        {
            p->repaint (getBounds());
            if (auto* gp = p->getParentComponent())
                gp->repaint (p->getBounds());
        }

        repaint();

        if (auto* peer = getPeer())
            peer->performAnyPendingRepaintsNow(); // force the OS peer to paint immediately

    }

    juce::Rectangle<int> area;
    juce::Colour needleColour;
    float valueInDecibel;
    int minValue, maxValue;
    float sAngle, eAngle;
    uint32_t statusOutline{0xff00838f};

};

class MeterBackground : public juce::Component
{
public:

    MeterBackground()
    {
        sAngle = 0.0f;
        eAngle = 0.0f;
        minValue = -30;
        maxValue = 0;
        step = 5;
        indicatorColour = juce::Colour(bg_LightGrey);
        backgroundApp = juce::Colour(bg_App);
        //setBufferedToImage(true);
    };
    void prepare(const float& s, const float& e)
    {
        sAngle = s;
        eAngle = e;
    };
    void paint(juce::Graphics& g) override
    {
        const auto bounds = meterArea.toFloat();
        const float centreX = bounds.getX() + bounds.getWidth() * 0.5f;
        const float centreY = bounds.getY() + bounds.getHeight();
        const float needleLength = juce::jmin(bounds.getWidth() * 0.7f, bounds.getHeight() * 0.7f);

        g.setColour(backgroundApp);
        g.fillRoundedRectangle(meterArea.toFloat(), 1);

        g.setColour(indicatorColour);
        drawIndicators(g, centreX, centreY, needleLength);
    };
    void resized() override
    {
        meterArea = getLocalBounds().reduced(3);
        repaint();
    };
    void drawIndicators(juce::Graphics& g, float centreX, float centreY, float length)
    {
        const auto indices = (abs(maxValue - minValue) / step) + 1;
        int val = minValue;

        for (int i = 0; i < indices; ++i)
        {
            float mapped = juce::jmap(static_cast<float>(val), static_cast<float>(minValue), static_cast<float>(maxValue), sAngle,
                                eAngle);
            mapped -= mapped > 2 * juce::MathConstants<float>::pi ? juce::MathConstants<float>::twoPi : 0.0f;
            const float x2 = centreX + sin(mapped) * length;
            const float y2 = centreY - cos(mapped) * length;
            const float rX = centreX - x2;
            const float rY = centreY - y2;
            const float rLength = sqrt(juce::square(rX) + juce::square(rY));
            const float nX = rX / rLength;
            const float nY = rY / rLength;
            const float xCof = nX * 7;
            const float yCof = nY * 7;

            g.drawArrow({x2, y2, x2 - xCof, y2 - yCof}, 2.0f, 0, 0);
            val += step;
        }
    };

    void setIndicatorEnabled(bool state)
    {
        indicatorColour = state
            ? juce::Colour(bg_LightGrey)
            : juce::Colour(bg_LightGrey).withAlpha(0.35f);
        repaint();
    };

    //Backgrounds
    uint32_t bg_App{0xff424242};
    uint32_t bg_DarkGrey{0xff212121};
    uint32_t bg_MidGrey{0xff616161};
    uint32_t bg_LightGrey{0xff9e9e9e};
private:
    juce::Rectangle<int> meterArea;
    juce::Colour indicatorColour;
    juce::Colour backgroundApp;
    int minValue, maxValue;
    float sAngle, eAngle;
    int step;
};

class BKCompressorMeter : public juce::Component {
public:
    // enum Mode { IN = 1, OUT, GR };

    BKCompressorMeter(CompressorParams *_params);
    void paint(juce::Graphics& g) override;
    void resized() override;
    // void setMode(int m);
    // void modeBoxChanged();
    void update(const float& val);
    // int getMode();
    float getValue();
    void setGUIEnabled(bool state);
    juce::FlexBox mBox;
    juce::Rectangle<int> boxArea;

    CompressorParams *params;
private:
    MeterBackground meterBg;
    MeterNeedle needle;
    // juce::ComboBox modeBox;
    juce::ComboBox *selectBox;
    juce::Colour backgroundDarkGrey;
    // int meterMode;
    float valueInDecibel;
    float startAngle, endAngle;
};



