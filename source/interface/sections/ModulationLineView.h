//
// Created by Davis Polito on 1/31/25.
//

#ifndef BITKLAVIER2_MODULATIONLINEVIEW_H
#define BITKLAVIER2_MODULATIONLINEVIEW_H
#include "ModulationLine.h"
#include "ObjectLists/ModConnectionsList.h"
#include "PreparationSection.h"
class ConstructionSite;
class ModulationLineView : public PreparationSection::Listener,
                           public SynthSection,
                           public bitklavier::ModConnectionList::Listener

{
public:
    explicit ModulationLineView (ConstructionSite& site, juce::UndoManager& um, SynthGuiData* data);
    ~ModulationLineView();
    ConstructionSite& site;

    void renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate) override;

    juce::Point<int> mouse_drag_position_;
    juce::Component* current_source_;

    void resized() override;
    void paintBackground (juce::Graphics& g)
    {
    }

    void reset() override;
    std::shared_ptr<OpenGlLine> line_;

    //preparation seciton listerner
    void beginConnectorDrag (juce::AudioProcessorGraph::NodeAndChannel source,
        juce::AudioProcessorGraph::NodeAndChannel dest,
        const juce::MouseEvent& e) override {}
    void dragConnector (const juce::MouseEvent& e) override {}
    void endDraggingConnector (const juce::MouseEvent& e) override {}
    void preparationDropped (const juce::MouseEvent& e, juce::Point<int>) override;
    void preparationDragged (juce::Component*, const juce::MouseEvent& e) override;
    void modulationDropped (const juce::ValueTree& source, const juce::ValueTree& dest) override;
    void resetDropped (const juce::ValueTree& source, const juce::ValueTree& dest) override;
    void tuningDropped (const juce::ValueTree& source, const juce::ValueTree& dest) override;
    void modConnectionAdded (bitklavier::ModConnection*) override;
    void modConnectionListChanged() override;
    void removeModConnection (bitklavier::ModConnection*) override;

    void _update() override;

    juce::UndoManager& undoManager;
    // void deleteConnectionsWithId(juce::AudioProcessorGraph::NodeID delete_id);
    void setActivePiano();
    void removeAllGuiListeners();
    juce::Array<ModulationLine*> objects;
    bitklavier::ModConnectionList* connection_list;
    juce::CriticalSection open_gl_lock;
    juce::ValueTree mod_connections_vt;
};

#endif //BITKLAVIER2_MODULATIONLINEVIEW_H
