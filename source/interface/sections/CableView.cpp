//
// Created by Davis Polito on 6/28/24.
//

#include "CableView.h"

#include "ConstructionSite.h"
#include "sound_engine.h"


CableView::CableView (ConstructionSite &site) :site(site), tracktion::engine::ValueTreeObjectList<Cable>(site.getState()), /*pathTask (*this),*/ SynthSection("cableView")
{
    setInterceptsMouseClicks (false,false);
    //startTimerHz (36);
    setAlwaysOnTop(true);
}

CableView::~CableView() = default;


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
    site.open_gl.initOpenGlComp.try_enqueue([this] {
        draggingConnector->getImageComponent()->init(site.open_gl);
        juce::MessageManager::callAsync(
            [safeComp = juce::Component::SafePointer<Cable>(draggingConnector.get())] {
                if (auto *comp = safeComp.getComponent()) {
                comp->setVisible(true);
               comp->getImageComponent()->setVisible(true);
                }
            });
    });
    draggingConnector->setInput (source);
    draggingConnector->setOutput (dest);

    addAndMakeVisible (draggingConnector.get());
    //draggingConnector->toFront (false);

    dragConnector (e);
}

void CableView::paint (juce::Graphics& g)
{

}

void CableView::reset()
{
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    //safe to do on message thread because we have locked processing if this is called
    //_parent->getSynth()->getEngine()->resetEngine();
    parent = _parent->getSynth()->getValueTree().getChildWithName(IDs::PIANO);
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
    site.open_gl.initOpenGlComp.try_enqueue([this]{
        destroyOpenGlComponent(*objectToDelete, site.open_gl);
        objectToDelete = nullptr;
    });
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

        juce::ValueTree _connection(IDs::CONNECTION);
        _connection.setProperty(IDs::isMod, 0, nullptr);
        _connection.setProperty(IDs::src,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.source.nodeID), nullptr);
        _connection.setProperty(IDs::srcIdx, connection.source.channelIndex, nullptr);
        _connection.setProperty(IDs::dest,  juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection.destination.nodeID), nullptr);
        _connection.setProperty(IDs::destIdx, connection.destination.channelIndex, nullptr);
        parent.appendChild(_connection,nullptr);
    }
}


void CableView::valueTreeRedirected (juce::ValueTree&)
{
    SynthGuiInterface* interface = findParentComponentOfClass<SynthGuiInterface>();

//    for (auto object : objects)
//    {
//        DBG("bo");
//        object->destroyOpenGlComponents(interface->getGui()->open_gl_);
//        this->removeSubSection(object);
//    }
    deleteAllObjects();
    rebuildObjects();
    for(auto object : objects)
    {
        newObjectAdded(object);
    }
    updateComponents();
    resized();
} // may need to add handling if this is hit

void CableView::newObjectAdded(Cable *c) {
    SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getSynth()->processorInitQueue.try_enqueue([this,c]
                                                       {
                                                           SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
                                                           _parent->getSynth()->addConnection(c->connection);
                                                           //changelistener callback is causing timing errors here.

                                                           //last_proc.reset();
                                                       });

}
void CableView::deleteObject(Cable *at)  {
    if ((juce::OpenGLContext::getCurrentContext() == nullptr))
    {
        SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();

        //safe to do on message thread because we have locked processing if this is called
        at->setVisible(false);
        site.open_gl.context.executeOnGLThread([this,&at](juce::OpenGLContext &openGLContext) {
            this->destroyOpenGlComponent(*(at->getImageComponent()), this->site.open_gl);
        },true);
    }

}


void CableView::updateCablePositions()
{
    for (auto* cable : objects)
    {
        cable->updateStartPoint();
        cable->updateEndPoint();
    }
}

Cable* CableView::createNewObject(const juce::ValueTree &v) {
    auto* comp = new Cable(&site, *this);
    addChildComponent(comp, 0);

    addOpenGlComponent(comp->getImageComponent(), true, false);
    site.open_gl.initOpenGlComp.try_enqueue([this, comp] {
        comp->getImageComponent()->init(site.open_gl);
        juce::MessageManager::callAsync(
                [safeComp = juce::Component::SafePointer<Cable>(comp)] {
                    if (auto *_comp = safeComp.getComponent()) {
                        _comp->setVisible(true);
                        _comp->getImageComponent()->setVisible(true);
                        _comp->resized();
                    }
                });
    });
    addAndMakeVisible (comp);
    comp->setValueTree(v);

    return comp;

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


void CableView::_update()
{
    updateComponents();
}


