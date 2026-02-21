//
// Created by Davis Polito on 6/28/24.
//

#include "CableView.h"

#include "ConstructionSite.h"
#include "sound_engine.h"


CableView::CableView (ConstructionSite &site, juce::UndoManager& um, SynthGuiData* data)
    : site(site),
    /*pathTask (*this),*/ SynthSection("cableView"),
    undoManager (um),
connection_list(data->synth->getActiveConnectionList()),
synth(data->synth)
{
    setInterceptsMouseClicks (false,true);
    //startTimerHz (36);
    setAlwaysOnTop(true);
    connection_list->addListener(this);
}

void CableView::requestDeleteConnection (juce::ValueTree connectionVT)
{
    if (connection_list != nullptr && connectionVT.isValid() && connectionVT.getParent().isValid())
    {
        connection_list->removeChild (connectionVT, &undoManager);
    }
}

CableView::~CableView() {
    for (auto object: objects)
        delete object;
  objects.clear();
}


bool CableView::mouseOverClickablePort()
{
    if (! mousePosition.has_value())
        return false;

#if ! JUCE_IOS
    if (isDraggingCable)
        return false;
#endif

    return false;
}

bool CableView::mouseDraggingOverOutputPort()
{
    if (! mousePosition.has_value() || ! isDraggingCable)
        return false;

    return false;
}

void CableView::beginConnectorDrag (juce::AudioProcessorGraph::NodeAndChannel source,
                         juce::AudioProcessorGraph::NodeAndChannel dest,
                         const juce::MouseEvent& e)
{
    auto c = dynamic_cast<Cable*> (e.originalComponent);
    int index = objects.indexOf(c);

    if(index >= 0)
        objects.remove(index);
    draggingConnector.reset (c);

    if (draggingConnector == nullptr)
        draggingConnector.reset (new Cable (&site, *this));

    addChildComponent(draggingConnector.get(), 0);

    addOpenGlComponent(draggingConnector->getImageComponent(), true, false);
    site.open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context) {
        draggingConnector->getImageComponent()->init(site.open_gl);
        juce::MessageManager::callAsync(
            [safeComp = juce::Component::SafePointer<Cable>(draggingConnector.get())] {
                if (auto *comp = safeComp.getComponent()) {
                comp->setVisible(true);
               comp->getImageComponent()->setVisible(true);
                }
            });
    },false);
    draggingConnector->setInput (source);
    draggingConnector->setOutput (dest);

    //draggingConnector->toFront (false);
    addAndMakeVisible (draggingConnector.get());

    dragConnector (e);
}

void CableView::paint (juce::Graphics& g)
{

}

void CableView::reset()
{
    DBG("At line " << __LINE__ << " in function " << __PRETTY_FUNCTION__);
    setActivePiano();
}
void CableView::resized()
{
    for (auto* cable : objects)
        cable->setBounds (getLocalBounds());
}

void CableView::mouseMove (const juce::MouseEvent& e)
{
    mousePosition = e.getEventRelativeTo (this).getPosition();
}

void CableView::mouseExit (const juce::MouseEvent&)
{
    mousePosition = std::nullopt;
}

void CableView::mouseDown (const juce::MouseEvent& e)
{

    if (!e.mods.isCommandDown() || e.mods.isPopupMenu() || e.eventComponent == nullptr)
        return; // not a valid mouse event

}

void CableView::mouseDrag (const juce::MouseEvent& e)
{

    if (e.eventComponent == nullptr)
        return;
    mousePosition = e.getEventRelativeTo (this).getPosition();
    const auto eventCompName = e.eventComponent->getName();

}

bool CableView::cableBeingDragged() const
{
    return isDraggingCable;
}

juce::Point<float> CableView::getCableMousePosition() const
{
    return mousePosition.value_or (juce::Point<int> {}).toFloat();
}

void CableView::mouseUp (const juce::MouseEvent& e)
{
    //DBG("mousei[can");
    if (isDraggingCable)
    {
        auto relMouse = e.getEventRelativeTo (this);
        auto mousePos = relMouse.getPosition();

        auto* cable = objects.getLast();
        // not being connected... trash the latest cable
        {
        }
        isDraggingCable = false;
    }
}


void CableView::endDraggingConnector (const juce::MouseEvent& e)
{
    if (draggingConnector == nullptr)
        return;

    //draggingConnector->setTooltip ({});

    auto e2 = e.getEventRelativeTo (this);
    auto connection = draggingConnector->connection;
    objectToDelete = draggingConnector->getImageComponent().get();

    draggingConnector = nullptr;
    objectToDelete->setVisible(false);
    site.open_gl.context.executeOnGLThread([this](juce::OpenGLContext &openGLContext) {destroyOpenGlComponent(*objectToDelete, site.open_gl);
        objectToDelete = nullptr;},true);

    if (auto* pin = site.findPinAt (e2.position))
    {
        if (connection.source.nodeID == juce::AudioProcessorGraph::NodeID())
        {
            if (pin->isInput)
                return;

            connection.source = pin->pin;
        }
        else
        {
            if (! pin->isInput)
                return;

            connection.destination = pin->pin;
        }
        if (!connection.source.isMIDI() && connection.destination.isMIDI()) {
            return;
        }
        if (connection.source.isMIDI() && !connection.destination.isMIDI()) {
            return;
        }

        if (connection.source.nodeID == connection.destination.nodeID) {
            return;
        }


        juce::ValueTree my_connection(IDs::CONNECTION);
        my_connection.setProperty(IDs::isMod, 0, nullptr);
        my_connection.setProperty(IDs::src,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.source.nodeID), nullptr);
        my_connection.setProperty(IDs::srcIdx, connection.source.channelIndex, nullptr);
        my_connection.setProperty(IDs::dest,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.destination.nodeID), nullptr);
        my_connection.setProperty(IDs::destIdx, connection.destination.channelIndex, nullptr);
        my_connection.setProperty(IDs::isSelected, 0, nullptr);

        connection_list->appendChild(my_connection, &undoManager);
    }
}





void CableView::connectionListChanged() {
    updateComponents();
    resized();
}

void CableView::removeConnection(bitklavier::Connection *c) {
    // DBG("remove cable");
    Cable* at = nullptr;
    int index = 0;
    if (objects.isEmpty())
        return;
    for (auto obj : objects)
    {

        if (obj->state == c->state) {
            at = obj;
            break;
        }
        index++;
    }
    if(at == nullptr)
        return;


    at->setVisible(false);

    // If context is already active, use GL thread for cleanup.
    // Else let standard destruction handle it (OpenGlImage has safety guards).
    if (site.open_gl.context.isActive())
    {
        site.open_gl.context.executeOnGLThread([this, at](juce::OpenGLContext &openGLContext) {
            this->destroyOpenGlComponent(*(at->getImageComponent()), site.open_gl);
        }, true);
    }
    else
    {
        // Still remove from the list so we don't try to render it.
        juce::ScopedLock lock(open_gl_critical_section_);
        auto new_logical_end = std::partition(open_gl_components_.begin(), open_gl_components_.end(),
                                              [&](std::shared_ptr<OpenGlComponent> const &p) {
                                                  return p.get() != static_cast<OpenGlComponent*>(at->image_component_.get());
                                              });
        if (new_logical_end != open_gl_components_.end())
            open_gl_components_.erase(new_logical_end, open_gl_components_.end());
    }

    {
        juce::ScopedLock lock(open_gl_critical_section_);
        objects.remove(index);
    }

    delete at;
    // DBG("cable remove");

}



void CableView::updateCablePositions()
{
    for (auto* cable : objects)
    {
        cable->updateStartPoint();
        cable->updateEndPoint();
    }
}
void CableView::connectionAdded(bitklavier::Connection *c) {

        // DBG("add cable");
        auto* comp = new Cable(&site, *this);
        addChildComponent(comp, 0);
    {
        juce::ScopedLock lock(open_gl_critical_section_);
            addOpenGlComponent(comp->getImageComponent(), true, false);
    }
    if (site.open_gl.context.isAttached() && site.open_gl.context.isActive())
    {
        site.open_gl.context.executeOnGLThread([this,comp](juce::OpenGLContext& context) {
            comp->getImageComponent()->init(site.open_gl);
            juce::MessageManager::callAsync(
                    [safeComp = juce::Component::SafePointer<Cable>(comp)] {
                        if (auto *_comp = safeComp.getComponent()) {
                            _comp->setVisible(true);
                            _comp->getImageComponent()->setVisible(true);
                            _comp->resized();
                        }
                    });
        },true);
    }
    else
    {
        // Fallback: if GL context not active, mark for init but don't block
        juce::MessageManager::callAsync(
                [safeComp = juce::Component::SafePointer<Cable>(comp)] {
                    if (auto *_comp = safeComp.getComponent()) {
                        _comp->setVisible(true);
                        _comp->getImageComponent()->setVisible(true);
                        _comp->resized();
                    }
                });
    }
        // addAndMakeVisible (comp);
        comp->setValueTree(c->state);
        objects.add(comp);
        // DBG("cable added");
}








void CableView::updateComponents()
{
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    for (int i = objects.size(); --i >= 0;)
    {
        if (! _parent->isConnected (objects.getUnchecked (i)->connection))
        {
            objects.remove (i);
        }
    }

    for (auto* cc : objects)
        cc->update();

}

void CableView::dragConnector(const juce::MouseEvent& e)
{

        auto e2 = e.getEventRelativeTo (this);

        if (draggingConnector != nullptr)
        {
            //draggingConnector->setTooltip ({});

            auto pos = e2.position;

            if (auto* pin = site.findPinAt (pos))
            {
                auto connection = draggingConnector->connection;

                if (connection.source.nodeID == juce::AudioProcessorGraph::NodeID() && ! pin->isInput)
                {
                    connection.source = pin->pin;
                }
                else if (connection.destination.nodeID == juce::AudioProcessorGraph::NodeID() && pin->isInput)
                {
                    connection.destination = pin->pin;
                }

//                if (graph.graph.canConnect (connection))
//                {
//                    pos = (pin->getParentComponent()->getPosition() + pin->getBounds().getCentre()).toFloat();
//                    draggingConnector->setTooltip (pin->getTooltip());
//                }
            }

            if (draggingConnector->connection.source.nodeID == juce::AudioProcessorGraph::NodeID())
                draggingConnector->dragStart (pos);
            else
                draggingConnector->dragEnd (pos);
        }
}
void CableView::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
    juce::ScopedLock lock(open_gl_critical_section_);
    SynthSection::renderOpenGlComponents(open_gl, animate);
}

void CableView::destroyOpenGlComponents(OpenGlWrapper &open_gl) {
    SynthSection::destroyOpenGlComponents(open_gl);
}



void CableView::_update()
{
    updateComponents();
}

void CableView::removeAllGuiListeners() {
    if (connection_list)
    {
        // Ensure any existing GUI elements are removed before detaching
        connection_list->deleteAllGui();
        connection_list->removeListener(this);
    }
    connection_list = nullptr;
}

void CableView::hitTestCables (juce::Point<float> pos)
{
    for (auto* cable : objects)
    {
        auto relativePos = pos - cable->getPosition().toFloat();
        if (cable->hitTest ((int) relativePos.x, (int) relativePos.y))
        {
            cable->state.setProperty (IDs::isSelected, 1, nullptr);
        }
        else
        {
            cable->state.setProperty (IDs::isSelected, 0, nullptr);
        }
        cable->resized();
    }
}

