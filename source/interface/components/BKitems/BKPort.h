//
// Created by Davis Polito on 7/10/24.
//

#ifndef BITKLAVIER2_BKPORT_H
#define BITKLAVIER2_BKPORT_H
#include "valuetree_utils/VariantConverters.h"
#include "open_gl_image_component.h"
#include "common.h"
class SynthGuiInterface;
class BKPort : public juce::Button
{
public:
    BKPort( juce::ValueTree v);
    class Listener
    {
    public:
        virtual ~Listener() = default;
        //virtual void portClicked(const juce::Point<int>& pos) = 0;
        virtual void beginConnectorDrag(juce::AudioProcessorGraph::NodeAndChannel source,
                                        juce::AudioProcessorGraph::NodeAndChannel dest,
                                        const juce::MouseEvent& e) = 0;
        virtual void dragConnector(const juce::MouseEvent& e) = 0;
        virtual void endDraggingConnector(const juce::MouseEvent& e) = 0;
    };

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    std::vector<Listener*> listeners_;
    void mouseDown (const juce::MouseEvent& e) override;
    //void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void resized()
    {
        redoImage();
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        juce::Path port_fill;
        auto bounds = getLocalBounds();
        const float strokeWidth = 2.0f; // if you plan to stroke later, this is useful

        // 1. Inset bounds slightly if needed for stroke safety (optional)
        //auto insetBounds = bounds.toFloat().reduced(strokeWidth / 2.0f);

        // 2. Build the half-circle (bottom half)
        port_fill.addPieSegment(bounds.toFloat(),
                                  juce::MathConstants<float>::pi,      // start: 180°
                                juce::MathConstants<float>::twoPi,   // end: 360°
                                0.0f);

        // 3. Apply rotation depending on port type
        float angle = 0.0f;
        if (isInput)
            angle = pin.isMIDI() ? juce::MathConstants<float>::pi / 2.0f   // 90°
                                 : juce::MathConstants<float>::pi;         // 180°
        else
            angle = pin.isMIDI() ? 3.0f * juce::MathConstants<float>::pi / 2.0f  // 270°
                                 : 0.0f;

        port_fill.applyTransform(juce::AffineTransform::rotation(angle, bounds.getCentreX(),bounds.getCentreY()));

        // 4. Draw the filled path
        g.setColour(juce::Colours::lightgrey); // or whatever you want
        g.fillPath(port_fill);

        // g.setColour(juce::Colours::white);
        // g.strokePath(arc_outline, juce::PathStrokeType(1.0f));

    }

    juce::AudioProcessorGraph::NodeAndChannel pin;
    juce::CachedValue<bool> isInput;
    int busIdx = 0;
    std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
    void redoImage() { image_component_->redrawImage (true); } //image_component_.redrawImage(true); }
    std::shared_ptr<OpenGlImageComponent> image_component_;
    juce::ValueTree state;
};




#endif //BITKLAVIER2_BKPORT_H
