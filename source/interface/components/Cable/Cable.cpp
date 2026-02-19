///
// Created by Davis Polito on 6/28/24.
//
#include "CableView.h"
#include "Cable.h"
#include "ConstructionSite.h"
#include "Preparations/PreparationSection.h"
using namespace CableConstants;

Cable::Cable (ConstructionSite* site, CableView& cableView) : juce::Component (Cable::componentName.data()),
                                                                                            image_component_(new OpenGlImageComponent()),
                                                                                            site(site),
                                                                                            cableView(cableView)


{
    //this->startPoint.store(startPoint.toFloat());
    //cableView = cableView;
    setAlwaysOnTop(true);
    setWantsKeyboardFocus (true);
    image_component_->setComponent(this);
    startColour = juce::Colours::lightgoldenrodyellow;
    endColour = juce::Colours::goldenrod;
    cableThickness = getCableThickness();
    // src_id.referTo(state, IDs::src, nullptr);
    // dest_id.referTo(state, IDs::dest, nullptr);
    // DBG("create cable");
}


Cable::~Cable()
{
    // DBG("destory cable");
}

juce::ValueTree Cable::getValueTree() {
    auto source = site->getState().getChildWithProperty(IDs::nodeID,
                                                   juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.source.nodeID));

    auto destination = site->getState().getChildWithProperty(IDs::nodeID,
                                                        juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.destination.nodeID));
//    if(source.isValid())
//    {
//        source.appendChild(state,nullptr);
//    }
//    if(destination.isValid())
//    {
//        destination.appendChild(state.createCopy(),nullptr);
//    }
    if(source.isValid() && destination.isValid())
        site->getState().appendChild(state,nullptr);


    return state;
}

void Cable::getPoints (juce::Point<float>& p1, juce::Point<float>& p2) const
{
    p1 = lastInputPos;
    p2 = lastOutputPos;

    if (auto* src = site->getComponentForPlugin (connection.source.nodeID))
        p1 = src->getPinPos (connection.source.channelIndex, false);

    if (auto* dest = site->getComponentForPlugin (connection.destination.nodeID))
        p2 = dest->getPinPos (connection.destination.channelIndex, true);
}

void Cable::updateStartPoint (bool repaintIfMoved)
{

}

void Cable::mouseDown (const juce::MouseEvent&)
{
    // set all other cables to be unselected
    // for (auto connection : site->connection_list)
    // {
    //     connection->state.setProperty (IDs::isSelected, 0, nullptr);
    // }
    state.setProperty (IDs::isSelected, 1, nullptr);
    dragging = false;
    grabKeyboardFocus();
    resized();
}

bool Cable::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::deleteKey)
    {
        // Let the CableView handle safe deletion through its connection list and undo manager
        cableView.requestDeleteConnection (state);
        return true;
    }

    return false;
}

void Cable::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging)
    {
    cableView.dragConnector (e);
    }
    else if (e.mouseWasDraggedSinceMouseDown())
    {
    dragging = true;

//    graph.graph.removeConnection (connection);

    double distanceFromStart, distanceFromEnd;
    getDistancesFromEnds (getPosition().toFloat() + e.position, distanceFromStart, distanceFromEnd);
    const bool isNearerSource = (distanceFromStart < distanceFromEnd);

    juce::AudioProcessorGraph::NodeAndChannel dummy { {}, 0 };

    cableView.beginConnectorDrag (isNearerSource ? dummy : connection.source,
    isNearerSource ? connection.destination : dummy,
    e);
    }
}

void Cable::mouseUp (const juce::MouseEvent& e)
{
    if (dragging)
    cableView.endDraggingConnector (e);
}

void Cable::updateEndPoint (bool repaintIfMoved) {

}

juce::Path Cable::createCablePath (juce::Point<float> start, juce::Point<float> end, float sf)
{
    const auto pointOff = portOffset + sf;
    bezier = CubicBezier (start, start.translated (pointOff, 0.0f), end.translated (-pointOff, 0.0f), end);
    numPointsInPath = (int) start.getDistanceFrom (end) + 1;
    juce::Path bezierPath;
    bezierPath.preallocateSpace ((numPointsInPath + 1) * 3);
    bezierPath.startNewSubPath (start);
    for (int i = 1; i < numPointsInPath; ++i)
    {
        const auto nextPoint = bezier.getPointOnCubicBezier ((float) i / (float) numPointsInPath);
        bezierPath.lineTo (nextPoint);
    }
    bezierPath.lineTo (end);

    return std::move (bezierPath);
}

void Cable::repaintIfNeeded (bool force)
{

}

void Cable::resizeToFit()
{
    juce::Point<float> p1, p2;
    getPoints (p1, p2);

    auto newBounds = juce::Rectangle<float> (p1, p2);

    if (connection.source.isMIDI())
    {
        if (p2.y >= p1.y)
        {
            float midX = (p1.x + p2.x) * 0.5f;

            newBounds = newBounds.getUnion (juce::Rectangle<float> (p1.x, p1.y - 20.0f, 1.0f, 1.0f))
                                .getUnion (juce::Rectangle<float> (midX, p1.y - 20.0f, 1.0f, 1.0f))
                                .getUnion (juce::Rectangle<float> (midX, p2.y + 20.0f, 1.0f, 1.0f))
                                .getUnion (juce::Rectangle<float> (p2.x, p2.y + 20.0f, 1.0f, 1.0f));
        }
    }
    else
    {
        if (p2.x <= p1.x)
        {
            const float portSegment = 20.0f;
            float midY = (p1.y + p2.y) * 0.5f;
            newBounds = newBounds.getUnion (juce::Rectangle<float> (p1.x + portSegment, p1.y, 1.0f, 1.0f))
                                .getUnion (juce::Rectangle<float> (p1.x + portSegment, midY, 1.0f, 1.0f))
                                .getUnion (juce::Rectangle<float> (p2.x - portSegment, midY, 1.0f, 1.0f))
                                .getUnion (juce::Rectangle<float> (p2.x - portSegment, p2.y, 1.0f, 1.0f));
        }
    }

    newBounds = newBounds.expanded (10.0f); // extra margin for stroke/arrow

    auto integerBounds = newBounds.getSmallestIntegerContainer();
    if (integerBounds != getBounds())
        setBounds (integerBounds);
    else
        resized();

    repaint();
}

void Cable::resized()
{
    juce::Point<float> p1, p2;
    getPoints (p1, p2);

    lastInputPos = p1;
    lastOutputPos = p2;

    p1 -= getPosition().toFloat();
    p2 -= getPosition().toFloat();

    const float midY_original = (p1.y + p2.y) * 0.5f;

    linePath.clear();
    linePath.startNewSubPath (p1);

    /*
     * Gemini suggests the following, perhaps for the future:
     * If you want that smooth, "liquid" look found in modular synth software (like VCV Rack),
     * you can use a hybrid approach that blends both X and Y offsets. Instead of an if/else,
     * you can use a fixed X-offset for horizontal flow but add a minimum "slack" value so
     * the curve never feels too stiff when the points are close together.
     *
     * CP1x = P1x + max(dx * 0.5, 30.))
     *
     * This keeps the curve exiting the "port" horizontally before dipping.
     */

    if (!connection.source.isMIDI()) // for audio connections, running out the right-side ports of preps
    {
        if (p2.x > p1.x)
        {
            // 1. Calculate the distance between points
            float dx = std::abs(p2.x - p1.x);
            float dy = std::abs(p2.y - p1.y);

            // 2. Determine how much "bend" to apply based on the largest distance
            // This ensures the curve scales with the length of the path.
            float offset = (dx > dy) ? dx * 0.5f : dy * 0.5f;

            linePath.clear();
            linePath.startNewSubPath(p1);

            linePath.cubicTo (p1.x + offset, p1.y,
                                 p2.x - offset, p2.y,
                                 p2.x, p2.y);
        }
        else
        {
            const float portSegment = 10.0f;
            float midY = (p1.y + p2.y) * 0.5f;

            linePath.lineTo (p1.x + portSegment, p1.y);
            linePath.lineTo (p1.x + portSegment, midY);
            linePath.lineTo (p2.x - portSegment, midY);
            linePath.lineTo (p2.x - portSegment, p2.y);
            linePath.lineTo (p2.x, p2.y);
        }
    }
    else if (p2.y < p1.y) // audio and MIDI connections, running vertically from, say, Keymap to other preps
    {
        linePath.cubicTo (p1.x, p1.y + (p2.y - p1.y) * 0.33f,
                         p2.x, p1.y + (p2.y - p1.y) * 0.66f,
                         p2.x, p2.y);
    }
    else // case when the output port is above the input port, to make a more readable path
    {
        const float portSegment = 10.0f;
        float midX = (p1.x + p2.x) * 0.5f;

        linePath.lineTo (p1.x, p1.y - portSegment);
        linePath.lineTo (midX, p1.y - portSegment);
        linePath.lineTo (midX, p2.y + portSegment);
        linePath.lineTo (p2.x, p2.y + portSegment);
        linePath.lineTo (p2.x, p2.y);
    }

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

    if (!connection.source.isMIDI())
    {
        if (p2.x > p1.x)
        {
            // Calculate the cubic bezier midpoint and tangent
            float dx = std::abs(p2.x - p1.x);
            float dy = std::abs(p2.y - p1.y);
            float offset = (dx > dy) ? dx * 0.5f : dy * 0.5f;

            juce::Point<float> cp1 { p1.x + offset, p1.y };
            juce::Point<float> cp2 { p2.x - offset, p2.y };

            // B(0.5) = 0.125*P0 + 0.375*P1 + 0.375*P2 + 0.125*P3
            juce::Point<float> midPoint = p1 * 0.125f + cp1 * 0.375f + cp2 * 0.375f + p2 * 0.125f;

            // B'(0.5) = 0.75*(P1-P0) + 1.5*(P2-P1) + 0.75*(P3-P2)
            juce::Point<float> tangent = (cp1 - p1) * 0.75f + (cp2 - cp1) * 1.5f + (p2 - cp2) * 0.75f;

            arrow.applyTransform (juce::AffineTransform()
                                          .rotated ((float) atan2 (tangent.y, tangent.x))
                                          .translated (midPoint));
        }
        else
        {
            // midpoint of the cross segment
            arrow.applyTransform (juce::AffineTransform()
                                          .rotated (juce::MathConstants<float>::pi) // pointing left
                                          .translated ((p1.x + p2.x) * 0.5f, midY_original));
        }
    }
    else if (p2.y < p1.y)
    {
        juce::Point<float> cp1 { p1.x, p1.y + (p2.y - p1.y) * 0.33f };
        juce::Point<float> cp2 { p2.x, p1.y + (p2.y - p1.y) * 0.66f };

        juce::Point<float> midPoint = p1 * 0.125f + cp1 * 0.375f + cp2 * 0.375f + p2 * 0.125f;
        juce::Point<float> tangent = (cp1 - p1) * 0.75f + (cp2 - cp1) * 1.5f + (p2 - cp2) * 0.75f;

        arrow.applyTransform (juce::AffineTransform()
                                      .rotated ((float) atan2 (tangent.y, tangent.x))
                                      .translated (midPoint));
    }
    else
    {
        // Arrow at the halfway point along the path.
        // For the 5-segment path, the midpoint is always on the vertical segment at midX.
        float midX = (p1.x + p2.x) * 0.5f;

        arrow.applyTransform (juce::AffineTransform()
                                      .rotated (juce::MathConstants<float>::halfPi) // pointing down
                                      .translated (midX, midY_original));
    }

    linePath.addPath (arrow);
    linePath.setUsingNonZeroWinding (true);

    juce::MessageManager::callAsync (
            [safeComp = juce::Component::SafePointer<Cable> (this)]
            {
                if (auto* comp = safeComp.getComponent())
                    comp->redoImage();
                //comp->repaint (cableBounds);
            });

    // highlight cable if selected
    if (state.getProperty (IDs::isSelected)) {
        juce::Path selectionPath;
        juce::PathStrokeType highlightStroke (4.0f); // Slightly wider than the main cable
        highlightStroke.createStrokedPath (selectionPath, linePath);
        linePath.addPath(selectionPath);
    }
}

float Cable::getCableThickness() const
{
    auto levelMult = std::pow (juce::jmap (levelDB, floorDB, 0.0f, 0.0f, 1.0f), 0.9f);
    return minCableThickness * (1.0f + 0.9f * levelMult);
}

void Cable::drawCableShadow (juce::Graphics& g, float thickness)
{
    auto cableShadow = [this]
    {
        juce::ScopedLock sl (pathCrit);
        return juce::Path { linePath };
    }();
    cableShadow.applyTransform (juce::AffineTransform::translation (0.0f, thickness * 0.6f));
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.strokePath (cableShadow, juce::PathStrokeType (minCableThickness, juce::PathStrokeType::JointStyle::curved));
}

void Cable::drawCableEndCircle (juce::Graphics& g, juce::Point<float> centre, juce::Colour colour) const
{
    auto circle = (juce::Rectangle { minCableThickness, minCableThickness } * 2.4f * scaleFactor.load()).withCentre (centre);
    g.setColour (colour);
    g.fillEllipse (circle);

    g.setColour (juce::Colours::white);
    g.drawEllipse (circle, portCircleThickness);
}

void Cable::drawCable (juce::Graphics& g, juce::Point<float> start, juce::Point<float> end)
{
    drawCableShadow (g, cableThickness);

    if (state.getProperty (IDs::isSelected)) {
        g.setColour(juce::Colours::white);
        juce::ScopedLock sl (pathCrit);
        g.strokePath (linePath, juce::PathStrokeType (cableThickness + 1.5f));
    }

    g.setGradientFill (juce::ColourGradient { startColour, start, endColour, end, false });
    {
        juce::ScopedLock sl (pathCrit);
        g.fillPath (linePath);
        // Also stroke it to give it some weight if needed, or just leave it filled if it's already stroked
        // But since we are using fillPath, the linePath must have been converted to a stroked path.
    }
}

void Cable::paint (juce::Graphics& g)
{
    drawCable (g, startPoint, endPoint);
    //redoImage();
}

