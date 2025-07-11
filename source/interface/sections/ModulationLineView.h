//
// Created by Davis Polito on 1/31/25.
//

#ifndef BITKLAVIER2_MODULATIONLINEVIEW_H
#define BITKLAVIER2_MODULATIONLINEVIEW_H
#include "PreparationSection.h"
#include "ModulationLine.h"
class ConstructionSite;
class ModulationLineView : public PreparationSection::Listener,
                            public SynthSection,
                           public tracktion::engine::ValueTreeObjectList<ModulationLine>
{
public:
    explicit ModulationLineView(ConstructionSite &site, juce::UndoManager& um);
    ~ModulationLineView();
    ConstructionSite &site;

    void renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) override;

    juce::Point<int> mouse_drag_position_;
    juce::Component* current_source_;

    void resized() override;
    void paintBackground(juce::Graphics &g)
    {
    }

    void reset() override;
   std::shared_ptr<OpenGlLine> line_;

    //preparation seciton listerner
    void beginConnectorDrag(juce::AudioProcessorGraph::NodeAndChannel source,
                                    juce::AudioProcessorGraph::NodeAndChannel dest,
                                    const juce::MouseEvent& e) override {}
    void dragConnector(const juce::MouseEvent& e) override {}
    void endDraggingConnector(const juce::MouseEvent& e) override {}
    void preparationDropped(const juce::MouseEvent& e, juce::Point<int>) override;
    void preparationDragged(juce::Component*, const juce::MouseEvent&e) override;
    void modulationDropped(const juce::ValueTree& source, const juce::ValueTree& dest) override;
    void resetDropped(const juce::ValueTree& source, const juce::ValueTree& dest) override;
    void tuningDropped(const juce::ValueTree &source, const juce::ValueTree &dest) override;
    void midifilterDropped(const juce::ValueTree &source, const juce::ValueTree &dest) override;


    void _update() override;

    juce::UndoManager& undoManager;
    void deleteConnectionsWithId(juce::AudioProcessorGraph::NodeID delete_id);



    ///valutreeobectlist:
    ModulationLine* createNewObject(const juce::ValueTree& v) override;
    void deleteObject (ModulationLine* at) override;


    void newObjectAdded (ModulationLine*) override;
    void objectRemoved (ModulationLine*) override     { resized();}//resized(); }
    void objectOrderChanged() override              {resized(); }//resized(); }
    // void valueTreeParentChanged (juce::ValueTree&) override;
    void valueTreeRedirected (juce::ValueTree&) override ;
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override {
        tracktion::engine::ValueTreeObjectList<ModulationLine>::valueTreePropertyChanged(v, i);
    }
    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType (IDs::MODCONNECTION) || v.hasType(IDs::TUNINGCONNECTION) || v.hasType(IDs::RESETCONNECTION);
    }
    juce::CriticalSection open_gl_lock;
};

#endif //BITKLAVIER2_MODULATIONLINEVIEW_H
