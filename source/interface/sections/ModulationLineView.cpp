//
// Created by Davis Polito on 1/31/25.
//

#include "ModulationLineView.h"
#include "ConstructionSite.h"
#include "fullInterface.h"
#include "modulation_manager.h"
#include "open_gl_line.h"
ModulationLineView::ModulationLineView(ConstructionSite &site) : SynthSection("modlineview"), site(site),
tracktion::engine::ValueTreeObjectList<ModulationLine>(site.getState().getParent().getChildWithName(IDs::MODCONNECTIONS)),current_source_(nullptr)//, line_(std::make_shared<OpenGlLine>(nullptr, nullptr, this))
{
    setInterceptsMouseClicks(false, false);
    setAlwaysOnTop(true);
//    addAndMakeVisible(line_.get());

}
void ModulationLineView::reset()
{

    DBG("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();

    if (_parent->getSynth()->getCriticalSection().tryEnter())
    {
              //safe to do on message thread because we have locked processing if this is called
        parent = _parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO).getChildWithName(IDs::MODCONNECTIONS);
        _parent->getSynth()->getCriticalSection().exit();
    }
    else
    {
        jassertfalse; // The message thread was NOT holding the lock
    }
    //leave it open for duration ofall restes

}
ModulationLineView::~ModulationLineView()
{
    freeObjects();
}

void ModulationLineView::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
  for (auto line : objects)
  {

      line->line->render(open_gl, animate);
  }
}

//probably want to move this to construction site leaving here for now
void ModulationLineView::preparationDragged(juce::Component *comp, const juce::MouseEvent &e) {
    current_source_ = comp;
    mouse_drag_position_ = getLocalPoint(current_source_, e.getPosition());
    juce::Rectangle<int> global_bounds = getLocalArea(current_source_, current_source_->getLocalBounds());
    juce::Point<int> global_start = global_bounds.getCentre();
}
void ModulationLineView::preparationDropped(const juce::MouseEvent& e, juce::Point<int>)
{
    if(current_source_ == nullptr)
        return;
    mouse_drag_position_ = getLocalPoint(current_source_, e.getPosition());
    auto comp =  getComponentAt(mouse_drag_position_.x, mouse_drag_position_.y);
    if(comp)
    {

        DBG(comp->getName());
    }
    site.mouse_drag_position_ = mouse_drag_position_;
}


void ModulationLineView::modulationDropped(const juce::ValueTree &source, const juce::ValueTree &destination)
{
    auto sourceId =juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(source.getProperty(IDs::nodeID,-1));
    auto destId =juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar( destination.getProperty(IDs::nodeID,-1));

    //protects against short/accidental drag drops
    if (sourceId == destId)
        return;
    //modconnections will not hold a source index they simpl represent a connection btwn a mod and a prep

    //right now is does make a connection on the backend but i think this should probably change once all this is fully implemented
    //actual connections should be children of modconnection vtrees
    juce::ValueTree _connection(IDs::MODCONNECTION);
    _connection.setProperty(IDs::isMod, 1, nullptr);
    _connection.setProperty(IDs::src,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(sourceId), nullptr);
    _connection.setProperty(IDs::dest,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(destId), nullptr);
    parent.addChild(_connection, -1, nullptr);
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getGui()->modulation_manager->added();
}
void ModulationLineView::resized()
{
    for(auto line: objects)
    {
        line->setBounds(getLocalBounds());
    }
}
void ModulationLineView::_update()
{
   for(auto line: objects)
   {
       line->update();
   }
}

ModulationLine* ModulationLineView::createNewObject(const juce::ValueTree &v) {
    auto *line = new ModulationLine(&site,this,v);
    return line;
}


void ModulationLineView::deleteObject(ModulationLine *at) {
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getSynth()->processorInitQueue.try_enqueue([this]
                                                       {
                                                           SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
                                                           //_parent->getSynth()->removeConnection(connection);
        //need to find all connections and remove them
                                                       });
    if ((juce::OpenGLContext::getCurrentContext() == nullptr))
    {


        at->setVisible(false);
        site.open_gl.context.executeOnGLThread([this,&at](juce::OpenGLContext &openGLContext) {
            this->destroyOpenGlComponent(*(at->line), this->site.open_gl);
        },true);
    } else
        delete at;
}


void ModulationLineView::newObjectAdded(ModulationLine *) {
//    auto interface

}


void ModulationLineView::valueTreeRedirected(juce::ValueTree &) {}