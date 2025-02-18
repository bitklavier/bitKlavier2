//
// Created by Davis Polito on 6/28/24.
//
#include "CableView.h"
#include "Cable.h"

#include "ConstructionSite.h"
using namespace CableConstants;

Cable::Cable (ConstructionSite* site, CableView& cableView) : juce::Component (Cable::componentName.data()),
                                                                                            image_component_(new OpenGlImageComponent()),
                                                                                            site(site),
                                                                                            cableView(cableView)


{
    //this->startPoint.store(startPoint.toFloat());
    //cableView = cableView;
    setAlwaysOnTop(true);
    image_component_->setComponent(this);
    startColour = juce::Colours::black;
    endColour = juce::Colours::black;
    cableThickness = getCableThickness();
    state.setProperty(IDs::src,"",nullptr);
    state.setProperty(IDs::dest,"",nullptr);
    state.setProperty(IDs::srcIdx,"",nullptr);
    state.setProperty(IDs::destIdx,"",nullptr);
    createUuidProperty(state);
}


Cable::~Cable()
{
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

    g.setGradientFill (juce::ColourGradient { startColour, start, endColour, end, false });
    {
        juce::ScopedLock sl (pathCrit);
        g.strokePath (linePath, juce::PathStrokeType (cableThickness, juce::PathStrokeType::JointStyle::curved));
    }

//    drawCableEndCircle (g, start, startColour);
//    drawCableEndCircle (g, end, endColour);
}

void Cable::paint (juce::Graphics& g)
{
    drawCable (g, startPoint, endPoint);
    //redoImage();
}

