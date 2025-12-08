//
// Created by Davis Polito on 7/10/24.
//

#include "BKPort.h"
#include "paths.h"
#include "Identifiers.h"
#include "look_and_feel/default_look_and_feel.h"

BKPort::BKPort( juce::ValueTree v) : juce::Button("port"), state(v), image_component_(new OpenGlImageComponent())
{
    setLookAndFeel(DefaultLookAndFeel::instance());
    pin = juce::AudioProcessorGraph::NodeAndChannel{juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getParent().getProperty(IDs::nodeID)), v.getProperty(IDs::chIdx)};

    isInput.referTo(v, IDs::isIn, nullptr);
    setInterceptsMouseClicks(true, true);
    setAlwaysOnTop(true);
    image_component_->setComponent(this);
    auto path = Paths::portPaths();
}

void BKPort::mouseDown (const juce::MouseEvent& e)
{
    juce::AudioProcessorGraph::NodeAndChannel dummy { {}, 0 };
    for (auto listener: listeners_)
    {
        // if it's an input pin, drag from dummy to pin. otherwise pin to dummy
        listener->beginConnectorDrag(isInput ? dummy : pin,
                                 isInput ? pin : dummy,
                                 e);
    }
}

void BKPort::mouseDrag (const juce::MouseEvent& e)
{
    for (auto listener : listeners_)
        listener->dragConnector (e);
}

void BKPort::mouseUp (const juce::MouseEvent& e)
{
    for (auto listener : listeners_)
        listener->endDraggingConnector (e);
}