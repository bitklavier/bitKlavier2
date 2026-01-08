//
// Created by Davis Polito on 1/17/24.
//

#ifndef BITKLAVIER2_CONSTRUCTIONSITE_H
#define BITKLAVIER2_CONSTRUCTIONSITE_H
#include "../../common/ObjectLists/PreparationList.h"
#include "CableView.h"
#include "ModulationLineView.h"
#include "PluginWindow.h"
#include "PreparationSelector.h"
#include "common.h"
#include "templates/Factory.h"

class OpenGlLine;
class SynthGuiInterface;
typedef Loki::Factory<std::unique_ptr<PreparationSection>, juce::Identifier, const juce::ValueTree&, SynthGuiInterface*> NodeFactory;
class ConstructionSite : public SynthSection,

                         public juce::DragAndDropContainer,
                         // public juce::ChangeListener,
                         private PreparationList::Listener,
                         public PreparationSection::Listener,
                         public juce::ApplicationCommandTarget

{
public:
    ConstructionSite (const juce::ValueTree& v, juce::UndoManager& um, OpenGlWrapper& open_gl, SynthGuiData* data, juce::ApplicationCommandManager& _manager);
    ~ConstructionSite (void);

    void redraw (void);

    ApplicationCommandTarget* getNextCommandTarget() override
    {
        return findFirstTargetParentComponent();
    }
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo (juce::CommandID id, juce::ApplicationCommandInfo& info) override;
    bool perform (const InvocationInfo& info) override;
    // void initializeCommandManager();

    // bool keyPressed (const juce::KeyPress& k, juce::Component* c) override;
    void itemIsBeingDragged (BKItem* thisItem, const juce::MouseEvent& e);

    void paintBackground (juce::Graphics& g) override;

    void addItem (bitklavier::BKPreparationType type, bool center = false);

    // void changeListenerCallback(juce::ChangeBroadcaster *source) override
    //
    // {
    //     updateComponents();
    // }

    void updateComponents();
    void reset() override;

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

    void dragOperationStarted (const juce::DragAndDropTarget::SourceDetails&)
    {
        //wsetMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    void dragOperationEnded (const juce::DragAndDropTarget::SourceDetails& source)
    {
        //setMouseCursor(juce::MouseCursor::ParentCursor);
        if (!item_dropped_on_prep_)
        {
            for (auto& fc : plugin_components)
            {
                if (fc.get() == source.sourceComponent)
                {
                    fc->undo.beginNewTransaction();
                    fc->curr_point = mouse_drag_position_;
                }
            }
            cableView._update();
            modulationLineView._update();
        }
        item_dropped_on_prep_ = false;
    }
    bool item_dropped_on_prep_ = false;

    juce::Point<int> mouse_drag_position_;
    juce::OwnedArray<PluginWindow> activePluginWindows;
    void createWindow (juce::AudioProcessorGraph::Node* node, PluginWindow::Type type);
    std::vector<std::unique_ptr<PreparationSection>> plugin_components;
    void renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate) override;
    void removeAllGuiListeners();

private:
    PreparationList* prep_list;
    juce::ApplicationCommandManager& commandManager;
    void moduleListChanged() {}
    void moduleAdded (PluginInstanceWrapper* newModule) override;
    void removeModule (PluginInstanceWrapper* moduleToRemove) override;
    void handlePluginPopup (int selection, int index);
    SynthGuiInterface* _parent;
    NodeFactory nodeFactory;
    juce::CriticalSection open_gl_critical_section_;
    std::shared_ptr<OpenGlLine> _line;

    bool edittingComment;

    juce::OwnedArray<juce::HashMap<int, int>> pastemap;
    friend class ModulationLineView;
    ModulationLineView modulationLineView;

    bool connect;
    int lastX, lastY;
    bool multiple;
    bool held;

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
