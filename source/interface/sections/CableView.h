//
// Created by Davis Polito on 6/28/24.
//

#ifndef BITKLAVIER2_CABLEVIEW_H
#define BITKLAVIER2_CABLEVIEW_H

#pragma once
#include "Cable.h"
#include "PreparationSection.h"
#include "synth_base.h"

#include "ObjectLists/ConnectionsList.h"

class CableView : public PreparationSection::Listener,
                  public SynthSection,
                  public bitklavier::ConnectionList::Listener
{
public:
    explicit CableView (ConstructionSite& site, juce::UndoManager& um, SynthGuiData* data);

    // Request deletion of a specific connection ValueTree (e.g., from a focused Cable)
    void requestDeleteConnection (juce::ValueTree connectionVT);

    ~CableView() override;

    void paint (juce::Graphics& g) override;

    void resized() override;

    void mouseMove (const juce::MouseEvent& e) override;

    void mouseExit (const juce::MouseEvent& e) override;

    void mouseDown (const juce::MouseEvent& e) override;

    void mouseDrag (const juce::MouseEvent& e) override;

    void mouseUp (const juce::MouseEvent& e) override;

    bool cableBeingDragged() const;

    juce::Point<float> getCableMousePosition() const;

    void updateCablePositions();

    void updateComponents();

    void _update();

    //void portClicked(const juce::Point<int>& pos, juce::AudioProcessorGraph::Node::Ptr node) override;

    void paintBackground (juce::Graphics& g)
    {
    }

    Cable* getComponentForConnection (const juce::AudioProcessorGraph::Connection& conn) const
    {
        for (auto* cc : objects)
            if (cc->connection == conn)
                return cc;

        return nullptr;
    }

    juce::Point<int> currentPort;

    void dragConnector (const juce::MouseEvent& e) override;

    void beginConnectorDrag (juce::AudioProcessorGraph::NodeAndChannel source,
        juce::AudioProcessorGraph::NodeAndChannel dest,
        const juce::MouseEvent& e) override;

    void endDraggingConnector (const juce::MouseEvent& e) override;

    void connectionAdded (bitklavier::Connection*) override;
    void removeConnection (bitklavier::Connection*) override;
    void connectionListChanged() override;

    void reset() override;

    void setActivePiano()
    {
        DBG("CableView::setActivePiano - starting");
        if (connection_list != nullptr)
        {
            connection_list->deleteAllGui();
            connection_list->removeListener (this);
        }

        if (synth == nullptr)
        {
            DBG("CableView::setActivePiano - synth is null!");
            return;
        }

        connection_list = synth->getActiveConnectionList();
        if (connection_list == nullptr)
        {
            DBG("CableView::setActivePiano - connection_list is null!");
            return;
        }

        DBG("CableView::setActivePiano - rebuildAllGui. connection_list size: " + juce::String(connection_list->objects.size()));
        connection_list->addListener (this);
        connection_list->rebuildAllGui();
        DBG("CableView::setActivePiano - finished");
    }

    void renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate) override;
    void removeAllGuiListeners();
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void hitTestCables (juce::Point<float> pos);
private:
    OpenGlComponent* objectToDelete = nullptr;

    juce::CriticalSection open_gl_critical_section_;
    bitklavier::ConnectionList* connection_list;
    SynthBase* synth;
    //    void timerCallback() override;
    juce::Array<Cable*> objects;
    ConstructionSite& site;
    juce::UndoManager& undoManager;

    //juce::ValueTree connections_vt;

    float scaleFactor = 1.0f;
    bool isDraggingCable = false;
    std::optional<juce::Point<int>> mousePosition;
    std::unique_ptr<Cable> draggingConnector;

    juce::Point<int> portToPaint;

    bool mouseOverClickablePort();

    bool mouseDraggingOverOutputPort();

    juce::CriticalSection cableMutex;

    bool portGlow = false;

    //    struct PathGeneratorTask : juce::TimeSliceClient
    //    {
    //    public:
    //        explicit PathGeneratorTask (CableView& cv);
    //        ~PathGeneratorTask() override;
    //
    //        int useTimeSlice() override;
    //
    //    private:
    //        struct juce::TimeSliceThread : juce::TimeSliceThread
    //        {
    //            juce::TimeSliceThread() : juce::TimeSliceThread ("Cable Drawing Background juce::Thread") {}
    //        };
    //
    //        juce::SharedResourcePointer<juce::TimeSliceThread> sharedTimeSliceThread;
    //
    //        CableView& cableView;
    //    } pathTask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CableView)
};

#endif //BITKLAVIER2_CABLEVIEW_H
