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
    ModulationLine(ConstructionSite *site, ModulationLineView* lineView,const juce::ValueTree& state);

    juce::AudioProcessorGraph::Connection connection{{{}, 0},
                                                     {{}, 0}};

    void update() {
        juce::Point<float> p1, p2;
        getPoints(p1, p2);

//        if (lastInputPos != p1 || lastOutputPos != p2)
        resizeToFit();
    }

    void dragStart(juce::Point<float> pos) {
        lastInputPos = pos;
        resizeToFit();
    }

    void dragEnd(juce::Point<float> pos) {
        lastOutputPos = pos;
        resizeToFit();

    }

    void resizeToFit() {
        juce::Point<float> p1, p2;
        getPoints(p1, p2);

        auto newBounds = juce::Rectangle<float>(p1, p2).expanded(4.0f).getSmallestIntegerContainer();

        if (newBounds != getBounds())
            setBounds(newBounds);
        else
            resized();

        repaint();
    }

    void getPoints(juce::Point<float> &p1, juce::Point<float> &p2) const;

      juce::ValueTree state;

    std::shared_ptr<OpenGlLine> line;



private:
    using AtomicPoint = std::atomic<juce::Point<float>>;
    static_assert(AtomicPoint::is_always_lock_free, "Atomic point needs to be lock free!");
    AtomicPoint startPoint{};
    AtomicPoint endPoint{};
    std::atomic<float> scaleFactor = 1.0f;
    juce::Point<float> lastInputPos, lastOutputPos;

    ConstructionSite *site;
    ModulationLineView *lineView;
};

#endif //BITKLAVIER2_MODULATIONGRAPHCONNECTION_H
