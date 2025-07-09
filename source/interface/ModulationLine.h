//
// Created by Davis Polito on 2/5/25.
//

#ifndef BITKLAVIER2_MODULATIONGRAPHCONNECTION_H
#define BITKLAVIER2_MODULATIONGRAPHCONNECTION_H
#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>
//#include "open_gl_line.h"
#include "Identifiers.h"
#include <juce_audio_processors/juce_audio_processors.h>
class ConstructionSite;
class ModulationLineView;
class OpenGlLine;
class ModulationLine : public juce::Component {
public:
    ModulationLine(ConstructionSite *site, ModulationLineView* lineView,const juce::ValueTree& );

//    juce::AudioProcessorGraph::Connection connection{{{}, 0},
//                                                     {{}, 0}};

    void update();

    void dragStart(juce::Point<int> pos) {
        lastInputPos = pos;
//        resizeToFit();
    }

    void dragEnd(juce::Point<int> pos) {
        lastOutputPos = pos;
//        resizeToFit();

    }

//    void resizeToFit() {
//        juce::Point<float> p1, p2;
//        getPoints(p1, p2);
//
//        auto newBounds = juce::Rectangle<float>(p1, p2).expanded(4.0f).getSmallestIntegerContainer();
//
//        if (newBounds != getBounds())
//            setBounds(newBounds);
//        else
//            resized();
//
//        repaint();
//    }

    void getPoints(juce::Point<int> &p1, juce::Point<int> &p2) const;

      juce::ValueTree state;

    std::shared_ptr<OpenGlLine> line;
    juce::CachedValue<juce::AudioProcessorGraph::NodeID> src_id;
    juce::CachedValue<juce::AudioProcessorGraph::NodeID> dest_id;


private:
    using AtomicPoint = std::atomic<juce::Point<int>>;
    static_assert(AtomicPoint::is_always_lock_free, "Atomic point needs to be lock free!");
    AtomicPoint startPoint{};
    AtomicPoint endPoint{};
    std::atomic<float> scaleFactor = 1.0f;
    juce::Point<int> lastInputPos, lastOutputPos;

    ConstructionSite *site;
    ModulationLineView *lineView;
};

#endif //BITKLAVIER2_MODULATIONGRAPHCONNECTION_H
