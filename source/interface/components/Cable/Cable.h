//
// Created by Davis Polito on 6/28/24.
//

#ifndef BITKLAVIER2_CABLE_H
#define BITKLAVIER2_CABLE_H
#include "valuetree_utils/VariantConverters.h"
#include "CubicBezier.h"
#include "open_gl_image_component.h"
#include "Identifiers.h"


class CableView;
namespace CableConstants
{
    const juce::Colour cableColour (0xFFD0592C); // currently only used for "glow"
    constexpr float minCableThickness = 5.0f;
    constexpr float portCircleThickness = 1.5f;

    constexpr int getPortDistanceLimit (float scaleFactor) { return int (20.0f * scaleFactor); }
    constexpr auto portOffset = 50.0f;

    constexpr float floorDB = -60.0f;
} // namespace CableConstants
class ConstructionSite;
class Cable : public juce::Component {
public:
    Cable(ConstructionSite* site, CableView& cableView);
    // Cable(ConstructionSite* site, CableView& cableView, const juce::ValueTree& v);
    ~Cable();

    void paint (juce::Graphics& g) override;
//    void resized() override;
//    bool hitTest (int x, int y) override;
    //Connection connection;

    ConstructionSite* site;
    CableView& cableView;
    static constexpr std::string_view componentName = "Cable";
    void repaintIfNeeded (bool force = false);

    void updateStartPoint (bool repaintIfMoved = true);
    void updateEndPoint (bool repaintIfMoved = true);
    std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
    void redoImage() { image_component_->redrawImage (true);}
    std::shared_ptr<OpenGlImageComponent> image_component_;

    void setInput (juce::AudioProcessorGraph::NodeAndChannel newSource)
    {
        if (connection.source != newSource)
        {
            connection.source = newSource;
            state.setProperty(IDs::src,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.source.nodeID), nullptr);
            state.setProperty(IDs::srcIdx, connection.source.channelIndex, nullptr);

            update();
        }
    }

    void setOutput (juce::AudioProcessorGraph::NodeAndChannel newDest)
    {
        if (connection.destination != newDest)
        {
            connection.destination = newDest;
            state.setProperty(IDs::dest,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.destination.nodeID), nullptr);
            state.setProperty(IDs::destIdx, connection.destination.channelIndex, nullptr);
            update();
        }
    }

    void dragStart (juce::Point<float> pos)
    {
        lastInputPos = pos;
        resizeToFit();
    }

    void dragEnd (juce::Point<float> pos)
    {
        lastOutputPos = pos;
        resizeToFit();
    }

    void update()
    {
        juce::Point<float> p1, p2;
        getPoints (p1, p2);

        if (lastInputPos != p1 || lastOutputPos != p2)
            resizeToFit();
    }

    juce::ValueTree getValueTree();


    void resizeToFit()
    {
        juce::Point<float> p1, p2;
        getPoints (p1, p2);

        auto newBounds = juce::Rectangle<float> (p1, p2).expanded (4.0f).getSmallestIntegerContainer();

        if (newBounds != getBounds())
            setBounds (newBounds);
        else
            resized();

        repaint();
    }
    void getPoints (juce::Point<float>& p1, juce::Point<float>& p2) const;
    juce::AudioProcessorGraph::Connection connection { { {}, 0 }, { {}, 0 } };
    bool hitTest (int x, int y) override
    {
        auto pos = juce::Point<int> (x, y).toFloat();

        if (hitPath.contains (pos))
        {
            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (pos, distanceFromStart, distanceFromEnd);

            // avoid clicking the connector when over a pin
            return distanceFromStart > 7.0 && distanceFromEnd > 7.0;
        }

        return false;
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        dragging = false;
    }

    void mouseDrag (const juce::MouseEvent& e) override;



    void mouseUp (const juce::MouseEvent& e) override;


    void resized() override
    {
        juce::Point<float> p1, p2;
        getPoints (p1, p2);

        lastInputPos = p1;
        lastOutputPos = p2;

        p1 -= getPosition().toFloat();
        p2 -= getPosition().toFloat();

        linePath.clear();
        linePath.startNewSubPath (p1);
        linePath.cubicTo (p1.x, p1.y + (p2.y - p1.y) * 0.33f,
                          p2.x, p1.y + (p2.y - p1.y) * 0.66f,
                          p2.x, p2.y);

        juce::PathStrokeType wideStroke (8.0f);
        wideStroke.createStrokedPath (hitPath, linePath);

        juce::PathStrokeType stroke (2.5f);
        stroke.createStrokedPath (linePath, linePath);

        auto arrowW = 5.0f;
        auto arrowL = 4.0f;

        juce::Path arrow;
        arrow.addTriangle (-arrowL, arrowW,
                           -arrowL, -arrowW,
                           arrowL, 0.0f);

        arrow.applyTransform (juce::AffineTransform()
                                      .rotated (juce::MathConstants<float>::halfPi - (float) atan2 (p2.x - p1.x, p2.y - p1.y))
                                      .translated ((p1 + p2) * 0.5f));

        linePath.addPath (arrow);
        linePath.setUsingNonZeroWinding (true);
        juce::MessageManager::callAsync (
                [safeComp = juce::Component::SafePointer<Cable> (this)]
                {
                    if (auto* comp = safeComp.getComponent())
                        comp->redoImage();
                    //comp->repaint (cableBounds);
                });
    }

    void getDistancesFromEnds (juce::Point<float> p, double& distanceFromStart, double& distanceFromEnd) const
    {
        juce::Point<float> p1, p2;
        getPoints (p1, p2);

        distanceFromStart = p1.getDistanceFrom (p);
        distanceFromEnd   = p2.getDistanceFrom (p);
    }

    void setValueTree(const juce::ValueTree& v)
    {
        state = v;
        connection.source = {juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::src)), v.getProperty(IDs::srcIdx)};
        connection.destination = {juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::dest)), v.getProperty(IDs::destIdx)};
        src_id.referTo(state, IDs::src, nullptr);
        dest_id.referTo(state, IDs::dest, nullptr);
        update();
    }
    juce::ValueTree state {IDs::CONNECTION};
    juce::CachedValue<juce::AudioProcessorGraph::NodeID> src_id;
    juce::CachedValue<juce::AudioProcessorGraph::NodeID> dest_id;



private:
    float getCableThickness() const;
    void drawCableShadow (juce::Graphics& g, float thickness);
    void drawCableEndCircle (juce::Graphics& g, juce::Point<float> centre, juce::Colour colour) const;
    void drawCable (juce::Graphics& g, juce::Point<float> start, juce::Point<float> end);
    //CableView& cableView;
    //const BoardComponent* board = nullptr;

    //chowdsp::PopupMenuHelper popupMenu;

    juce::Path cablePath {};
    int numPointsInPath = 0;
    CubicBezier bezier;
    float cableThickness = 0.0f;

    using AtomicPoint = std::atomic<juce::Point<float>>;
    static_assert (AtomicPoint::is_always_lock_free, "Atomic point needs to be lock free!");
    AtomicPoint startPoint {};
    AtomicPoint endPoint {};
    std::atomic<float> scaleFactor = 1.0f;
    juce::Point<float> lastInputPos, lastOutputPos;
    juce::Path linePath, hitPath;
    bool dragging = false;

    juce::Colour startColour;
    juce::Colour endColour;
    juce::Range<float> levelRange = { CableConstants::floorDB, 0.0f };
    float levelDB = levelRange.getStart();

    juce::Path createCablePath (juce::Point<float> start, juce::Point<float> end, float scaleFactor);
    juce::CriticalSection pathCrit;
};


#endif //BITKLAVIER2_CABLE_H
