//
// Created by Davis Polito on 1/31/25.
//

#include "ModulationLineView.h"
#include "ConstructionSite.h"
#include "fullInterface.h"
#include "modulation_manager.h"
#include "open_gl_line.h"

ModulationLineView::ModulationLineView (ConstructionSite& site, juce::UndoManager& um, SynthGuiData* data)
    : SynthSection ("modlineview"), site (site),

      current_source_ (nullptr),
      undoManager (um),
      connection_list (data->synth->getActiveModConnectionList())
{
    setInterceptsMouseClicks (false, false);
    setAlwaysOnTop (true);
    //    addAndMakeVisible(line_.get());
    connection_list->addListener (this);
}

void ModulationLineView::reset()
{
    DBG ("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();

    // parent = _parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO).getChildWithName(IDs::MODCONNECTIONS);
//    if(connection_list)
//    {
//        connection_list->removeListener(this);
//    }
//    connection_list = nullptr;
}
void ModulationLineView::removeAllGuiListeners()
{
    if(connection_list)
        connection_list->removeListener(this);
    connection_list = nullptr;
}

ModulationLineView::~ModulationLineView()
{
    // freeObjects();
}

void ModulationLineView::renderOpenGlComponents (OpenGlWrapper& open_gl, bool animate)
{
    juce::ScopedLock lock (open_gl_lock);
    for (auto line : objects)
    {
        if (line != nullptr && line - isVisible())
            line->line->render (open_gl, animate);
    }
}

//probably want to move this to construction site leaving here for now
void ModulationLineView::preparationDragged (juce::Component* comp, const juce::MouseEvent& e)
{
    current_source_ = comp;
    mouse_drag_position_ = getLocalPoint (current_source_, e.getPosition());
    juce::Rectangle<int> global_bounds = getLocalArea (current_source_, current_source_->getLocalBounds());
    juce::Point<int> global_start = global_bounds.getCentre();
}

void ModulationLineView::preparationDropped (const juce::MouseEvent& e, juce::Point<int>)
{
    if (current_source_ == nullptr)
        return;
    mouse_drag_position_ = getLocalPoint (current_source_, e.getPosition());
    auto comp = getComponentAt (mouse_drag_position_.x, mouse_drag_position_.y);
    if (comp)
    {
        DBG (comp->getName());
    }
    site.mouse_drag_position_ = mouse_drag_position_;
    current_source_ = nullptr;
}

void ModulationLineView::modulationDropped (const juce::ValueTree& source, const juce::ValueTree& destination)
{
    auto sourceId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
        source.getProperty (IDs::nodeID, -1));
    auto destId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
        destination.getProperty (IDs::nodeID, -1));

    //protects against short/accidental drag drops
    if (sourceId == destId)
        return;
    //modconnections will not hold a source index they simpl represent a connection btwn a mod and a prep

    //right now is does make a connection on the backend but i think this should probably change once all this is fully implemented
    //actual connections should be children of modconnection vtrees
    //does this add a connection to the backend audio proceessor graph or just the value tree - davis -- 4/25/25
    juce::ValueTree _connection (IDs::MODCONNECTION);
    _connection.setProperty (IDs::isMod, 1, nullptr);
    _connection.setProperty (IDs::src, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (sourceId), nullptr);
    _connection.setProperty (IDs::dest, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (destId), nullptr);
    undoManager.beginNewTransaction();
    connection_list->appendChild (_connection, &undoManager);
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getGui()->modulation_manager->added();
}

void ModulationLineView::resetDropped (const juce::ValueTree& source, const juce::ValueTree& destination)
{
    auto sourceId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
        source.getProperty (IDs::nodeID, -1));
    auto destId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
        destination.getProperty (IDs::nodeID, -1));

    //protects against short/accidental drag drops
    if (sourceId == destId)
        return;
    //modconnections will not hold a source index they simpl represent a connection btwn a mod and a prep

    //right now is does make a connection on the backend but i think this should probably change once all this is fully implemented
    //actual connections should be children of modconnection vtrees
    //does this add a connection to the backend audio proceessor graph or just the value tree - davis -- 4/25/25
    juce::ValueTree _connection (IDs::RESETCONNECTION);
    _connection.setProperty (IDs::isMod, 1, nullptr);
    _connection.setProperty (IDs::src, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (sourceId), nullptr);
    _connection.setProperty (IDs::dest, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (destId), nullptr);
    undoManager.beginNewTransaction();
    connection_list->appendChild (_connection, &undoManager);
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    // _parent->getGui()->modulation_manager->added();
}

void ModulationLineView::tuningDropped (const juce::ValueTree& source, const juce::ValueTree& dest)
{
    auto sourceId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (
        source.getProperty (IDs::nodeID, -1));
    auto destId = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar (dest.getProperty (IDs::nodeID, -1));
    //protects against short/accidental drag drops
    if (sourceId == destId)
        return;
    //modconnections will not hold a source index they simpl represent a connection btwn a mod and a prep
    juce::ValueTree _connection (IDs::TUNINGCONNECTION);
    _connection.setProperty (IDs::isMod, false, nullptr);
    _connection.setProperty (IDs::src, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (sourceId), nullptr);
    _connection.setProperty (IDs::dest, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar (destId), nullptr);
    undoManager.beginNewTransaction();
    connection_list->appendChild (_connection, &undoManager);
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getGui()->modulation_manager->added();
}

void ModulationLineView::resized()
{
    for (auto line : objects)
    {
        line->setBounds (getLocalBounds());
    }
}

void ModulationLineView::_update()
{
    for (auto line : objects)
    {
        line->update();
    }
}

void ModulationLineView::modConnectionAdded (bitklavier::ModConnection* c)
{
    auto* line = new ModulationLine (&site, this, c->state);
    objects.add (line);
    //right now this doesn't need to do anything since this function mainly is used to alert the audio thread
    // that some thing has change.
    // in our case a new line does not cause any audio thread behaviour to occur
}

void ModulationLineView::removeModConnection (bitklavier::ModConnection* c)
{
    DBG ("removemodconnection");
    ModulationLine* at;
    int index = 0;
    for (auto obj : objects)
    {
        if (obj->state == c->state)
        {
            at = obj;
            break;
        }
        index++;
    }
    if(at == nullptr)
        return;
    DBG ("index " + juce::String (index));
    if ((juce::OpenGLContext::getCurrentContext() == nullptr))
    {
        at->setVisible (false);
        site.open_gl.context.executeOnGLThread ([this, &at] (juce::OpenGLContext& openGLContext) {
            //can't use destroy component since lineveiw renders the lines directy from the array as opposed to adding them
            // to the open_gl_components vector... why?
            // idk maybe try to make it instantiate using addopenglcomponent and find out.. for now
            // i'll kep neing laxy --davis --4/25/25
            at->line->destroy (this->site.open_gl);
            //DBG("delete Line");
        },
            true);
    }
    DBG ("delete line object");
    {
        juce::ScopedLock lock (open_gl_lock);
        objects.remove (index);
    }
    delete at;
}

void ModulationLineView::setActivePiano()
{
    //    DBG ("setPIano CableView");
    if (connection_list != nullptr)
    {
        connection_list->deleteAllGui();
        connection_list->removeListener (this);
    }

    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    connection_list = interface->getSynth()->getActiveModConnectionList();
    mod_connections_vt = connection_list->getValueTree();
    connection_list->addListener (this);
    connection_list->rebuildAllGui();
}

void ModulationLineView::modConnectionListChanged()
{
}

// void ModulationLineView::newObjectAdded(ModulationLine * line) {
//
//
// }

// void ModulationLineView::deleteConnectionsWithId(juce::AudioProcessorGraph::NodeID delete_id)
// {
//     for (auto connection : objects){
//         if (connection->src_id == delete_id || connection->dest_id == delete_id){
//             .removeChild (connection->state, &undoManager);
//             //deleteObject (connection);
//         }
//     }
// }

// void ModulationLineView::valueTreeRedirected(juce::ValueTree &) {
//     deleteAllObjects();
//     rebuildObjects();
//     for(auto object : objects)
//     {
//         newObjectAdded(object);
//     }
//     _update();
//     resized();
// }
