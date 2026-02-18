//
// Created by Davis Polito on 1/17/24.
//

#ifndef BITKLAVIER2_CONSTRUCTIONSITE_H
#define BITKLAVIER2_CONSTRUCTIONSITE_H
#include "ObjectLists/PreparationList.h"
#include "CableView.h"
#include "ModulationLineView.h"
#include "PluginWindow.h"
#include "PreparationSelector.h"
#include "templates/Factory.h"

class OpenGlLine;
class SynthGuiInterface;
typedef Loki::Factory<std::unique_ptr<PreparationSection>, juce::Identifier, const juce::ValueTree&, SynthGuiInterface*> NodeFactory;
class ConstructionSite : public SynthSection,

                         public juce::DragAndDropContainer,
                         public juce::DragAndDropTarget,
                         // public juce::ChangeListener,
                         private PreparationList::Listener,
                         public PreparationSection::Listener,
                         public juce::ApplicationCommandTarget

{
public:
    ConstructionSite (const juce::ValueTree& v, juce::UndoManager& um, OpenGlWrapper& open_gl, SynthGuiData* data, juce::ApplicationCommandManager& _manager);
    ~ConstructionSite (void);

    ApplicationCommandTarget* getNextCommandTarget() override
    {
        return findFirstTargetParentComponent();
    }

    void getAllCommands (juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info) override;
    bool perform (const InvocationInfo& info) override;
    void itemIsBeingDragged (BKItem* thisItem, const juce::MouseEvent& e);
    void paintBackground (juce::Graphics& g) override;
    void addItem (int selection, bool center = false);
    void updateComponents();
    void reset() override;
    void redraw (void);

    BKPort* findPinAt (juce::Point<float> pos) const
    {
        for (auto& fc : plugin_components)
        {
            // NB: A Visual Studio optimiser error means we have to put this juce::Component* in a local
            // variable before trying to cast it, or it gets mysteriously optimised away..
            auto* comp = fc->getComponentAt (pos.toInt() - fc->getPosition());

            if (auto* pin = dynamic_cast<BKPort*> (comp))
                return pin;
        }

        return nullptr;
    }

    PreparationSection* getComponentForPlugin (juce::AudioProcessorGraph::NodeID nodeID) const;

    juce::Viewport* view;
    juce::Point<float> mouse;
    OpenGlWrapper& open_gl;
    juce::ValueTree parent;
    juce::ValueTree getState()
    {
        return parent;
    }

    void setActivePiano();

    void copyValueTree (const juce::ValueTree& vt)
    {
        parent.copyPropertiesFrom (vt, nullptr);
    }

    bool isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        return true;
    }

    void itemDragMove (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        mouse_drag_position_ = dragSourceDetails.localPosition;
    }

    void itemDropped (const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        // We handle this in dragOperationEnded
    }

    void dragOperationStarted (const juce::DragAndDropTarget::SourceDetails&)
    {
        //wsetMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    void dragOperationMoved (const juce::DragAndDropTarget::SourceDetails& source)
    {
    }

    void dragOperationEnded (const juce::DragAndDropTarget::SourceDetails& source)
    {
        // DBG("ConstructionSite::dragOperationEnded - item_dropped_on_prep_: " << (int)item_dropped_on_prep_);
        // DBG("ConstructionSite::dragOperationEnded - mouse_drag_position: " << mouse_drag_position_.toString());
        // DBG("ConstructionSite::dragOperationEnded - drag_offset: " << drag_offset_.toString());

        //setMouseCursor(juce::MouseCursor::ParentCursor);
        auto* targetComp = dynamic_cast<PreparationSection*>(getComponentAt(mouse_drag_position_));
        const bool droppedOnDifferentPrep = (item_dropped_on_prep_ && targetComp != nullptr && targetComp != source.sourceComponent.get());

        if (!droppedOnDifferentPrep)
        {
            for (auto& fc : plugin_components)
            {
                if (fc.get() == source.sourceComponent)
                {
                    // DBG("ConstructionSite::dragOperationEnded - Updating curr_point for " << fc->getName());
                    fc->undo.beginNewTransaction();
                    fc->curr_point = mouse_drag_position_ - drag_offset_;
                }
            }
            cableView._update();
            modulationLineView._update();
        }
        else
        {
            // DBG("ConstructionSite::dragOperationEnded - Dropped on different prep, skipping update");
        }
        item_dropped_on_prep_ = false;
    }
    bool item_dropped_on_prep_ = false;

    juce::Point<int> mouse_drag_position_;
    juce::Point<int> drag_offset_;
    juce::OwnedArray<PluginWindow> activePluginWindows;
    void createWindow (juce::AudioProcessorGraph::Node* node, PluginWindow::Type type);
    std::vector<std::unique_ptr<PreparationSection>> plugin_components;
    void renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate) override;
    void removeAllGuiListeners();
    void linkedPiano() override;

private:
    PreparationList* prep_list;
    bitklavier::ConnectionList* connection_list;
    juce::ApplicationCommandManager& commandManager;
    void moduleListChanged() {}
    void moduleAdded (PluginInstanceWrapper* newModule) override;
    void removeModule (PluginInstanceWrapper* moduleToRemove) override;
    void handlePluginPopup (int selection, int index);
    SynthGuiInterface* _parent;
    NodeFactory nodeFactory;
    juce::CriticalSection open_gl_critical_section_;
    std::shared_ptr<OpenGlLine> _line;

    bool editing_comment_;
    juce::OwnedArray<juce::HashMap<int, int>> pastemap;
    friend class ModulationLineView;
    ModulationLineView modulationLineView;

    bool connect;
    int lastX, lastY;
    bool multiple;
    bool held;

    friend class FooterSection;
    PreparationSelector preparationSelector;
    juce::LassoComponent<PreparationSection*> selectorLasso;

    friend class CableView;
    CableView cableView;

    juce::UndoManager& undo;

    void draw (void);
    void prepareItemDrag (BKItem* item, const juce::MouseEvent& e, bool center);
    void resized() override;
    void mouseDown (const juce::MouseEvent& eo) override;
    void mouseUp (const juce::MouseEvent& eo) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void deleteItem (BKItem* item);

    BKItem* getItemAtPoint (const int X, const int Y);

    JUCE_LEAK_DETECTOR (ConstructionSite)
};

#endif //BITKLAVIER2_CONSTRUCTIONSITE_H
