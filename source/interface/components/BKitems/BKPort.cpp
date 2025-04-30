//
// Created by Davis Polito on 7/10/24.
//

#include "BKPort.h"
#include "paths.h"
#include "Identifiers.h"

#include "look_and_feel/default_look_and_feel.h"
BKPort::BKPort( juce::ValueTree v) :
juce::Button("port"),
state(v),
image_component_(new OpenGlImageComponent())

{
    setLookAndFeel(DefaultLookAndFeel::instance());
    pin = juce::AudioProcessorGraph::NodeAndChannel{juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::nodeID)), v.getProperty(IDs::chIdx)};

    isInput.referTo(v, IDs::isIn, nullptr);
    setInterceptsMouseClicks(true, true);
    setAlwaysOnTop(true);
    image_component_->setComponent(this);
    auto path = Paths::portPaths();

//    if (auto node = _parent->getSynth()->getNodeForId(pin.nodeID))
//    {
//        juce::String tip;
//
//        if (pin.isMIDI())
//        {
//            tip = isInput ? "MIDI Input"
//                          : "MIDI Output";
//        }
//        else
//        {
//            auto& processor = *node->getProcessor();
//            auto channel = processor.getOffsetInBusBufferForAbsoluteChannelIndex (isInput, pin.channelIndex, busIdx);
//
//            if (auto* bus = processor.getBus (isInput, busIdx))
//                tip = bus->getName() + ": " + juce::AudioChannelSet::getAbbreviatedChannelTypeName (bus->getCurrentLayout().getTypeOfChannel (channel));
//            else
//                tip = (isInput ? "Main Input: "
//                               : "Main Output: ") + juce::String (pin.channelIndex + 1);
//
//        }

//        setTooltip (tip);
//    }
}
//void BKPort::mouseDown(const juce::MouseEvent& e)
//{
//    //getParentComponent()->mouseDown(e);]
//    for(auto listener : listeners_)
//    {
//        listener->portClicked(getBounds().getCentre());
//    }
//}
//
//void BKPort::mouseEnter(const juce::MouseEvent& e)
//{
//    //getParentComponent()->mouseDown(e);
//}


//void BKPort::mouseDoubleClick(const juce::MouseEvent& e)
//{
//
//}

void BKPort::mouseDown (const juce::MouseEvent& e)
{
    juce::AudioProcessorGraph::NodeAndChannel dummy { {}, 0 };
    for (auto listener: listeners_)
    {
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