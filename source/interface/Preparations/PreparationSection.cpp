//
// Created by Davis Polito on 2/16/24.
//

#include "PreparationSection.h"
#include "synth_gui_interface.h"

#include "FullInterface.h"
#include "ConstructionSite.h"

PreparationSection::PreparationSection(juce::String name, juce::ValueTree v, OpenGlWrapper &open_gl,
                                       juce::AudioProcessor *proc) : tracktion::engine::ValueTreeObjectList<BKPort>(v),
                                                                     SynthSection(name), state(v), _open_gl(open_gl),
                                                                     _proc(proc) {
    //_parent = findParentComponentOfClass<SynthGuiInterface>();
    setInterceptsMouseClicks(true,false);
    x.referTo(v, IDs::x, nullptr);
    y.referTo(v, IDs::y, nullptr);
    width.referTo(v, IDs::width, nullptr);
    height.referTo(v, IDs::height, nullptr);
    numIns.referTo(v, IDs::numIns, nullptr);
    numOuts.referTo(v, IDs::numOuts, nullptr);
    //constrainer.setBoundsForComponent(this,getParentComponent()->getLocalBounds() );

    uuid.referTo(state, IDs::uuid, nullptr);

    pluginID = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty(IDs::nodeID));
    //pluginID.uid = static_cast<uint32>(int(state.getProperty(IDs::uuid)));
    constrainer.setMinimumOnscreenAmounts(0xffffff, 0xffffff, 0xffffff, 0xffffff);
    rebuildObjects();

    for (auto object: objects) {
        open_gl.context.executeOnGLThread([this,object,&open_gl](juce::OpenGLContext& context){
            object->getImageComponent()->init(open_gl);
        },true);
        this->addOpenGlComponent(object->getImageComponent(), false, true);
        object->addListener(this);
        addAndMakeVisible(object);
        object->redoImage();

    }


    //get xml for valuetree from audioprocessor
    juce::MemoryBlock data;
    proc->getStateInformation(data);
    auto xml = juce::parseXML(data.toString());

    if (!state.getChild(0).isValid() && xml != nullptr)
        state.addChild(juce::ValueTree::fromXml(*xml), 0, nullptr);

}

void PreparationSection::paintBackground(juce::Graphics &g) {
//    g.saveState();
//    juce::Rectangle<int> bounds = getLocalArea(item.get(), item->getLocalBounds());
//    g.reduceClipRegion(bounds);
//    g.setOrigin(bounds.getTopLeft());
//    //item->paintBackground(g);
    if (item) {
        //item->setColor(findColour (Skin::kWidgetPrimary1, true));
        item->redoImage();
    }
    for (auto object: objects) {
        object->redoImage();
    }


//    g.restoreState();
}

void PreparationSection::mouseDown(const juce::MouseEvent &e) {

//    if (e.getNumberOfClicks() == 2) {
//        showPrepPopup(this);
//    } else {
//  selectedSet->addToSelectionBasedOnModifiers(this, e.mods);
        pointBeforDrag = this->getPosition();

//    }

}
void PreparationSection::itemDropped(const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
    dynamic_cast< ConstructionSite *>(getParentComponent())->item_dropped_on_prep_ = true;
    auto dropped_tree = juce::ValueTree::fromXml(dragSourceDetails.description);
    //should switch to strings for type names
    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeModulation)
    {
        for(auto listener: listeners_)
            listener->modulationDropped(dropped_tree, state);
    }
    if (static_cast<int>(dropped_tree.getProperty(IDs::type)) == bitklavier::BKPreparationType::PreparationTypeTuning)
        for (auto listener: listeners_)
            listener->tuningDropped(dropped_tree, state);
}
void PreparationSection::resized() {
    juce::Rectangle<float> bounds = getLocalBounds().toFloat();
    int item_padding_y = kItemPaddingY * size_ratio_;
    int item_height = getHeight() - 2 * item_padding_y;
    int item_padding_x = kItemPaddingX * size_ratio_;
    int item_width = getWidth() - 2 * item_padding_x;
    auto newBounds = getBoundsInParent();

    item->setBounds(item_padding_x, item_padding_y, item_width, item_height);

    if (auto *processor = getProcessor()) {
        for (auto *port: objects) {
            const bool isInput = port->isInput;
            auto channelIndex = port->pin.channelIndex;
            int busIdx = 0;
            processor->getOffsetInBusBufferForAbsoluteChannelIndex(isInput, channelIndex, busIdx);

            int total = isInput ? numIns : numOuts;
            int index = port->pin.isMIDI() ? (total - 1) : channelIndex;
            auto totalSpaces = static_cast<float> (total) +
                               (static_cast<float> (juce::jmax(0, processor->getBusCount(isInput) - 1)) * 0.5f);
            auto indexPos = static_cast<float> (index) + (static_cast<float> (busIdx) * 0.5f);
            auto transformedBounds = item->layer_1_.getBounds();
            if (port->pin.isMIDI())
            {
                port->setBounds(transformedBounds.getX() + transformedBounds.getWidth() * ((1.0f + indexPos) / (totalSpaces + 1.0f)),
                                isInput ? transformedBounds.getBottom() - portSize / 2 + BKItem::kMeterPixel
                                        : transformedBounds.getY() - portSize / 2,
                                portSize, portSize);
            }
            else
            {
                port->setBounds(isInput ? transformedBounds.getX() - portSize / 2 - BKItem::kMeterPixel
                                        : transformedBounds.getRight() - portSize / 2 + BKItem::kMeterPixel,
                                transformedBounds.getY() + transformedBounds.getHeight() * ((1.0f + indexPos) / (totalSpaces + 1.0f)) - portSize / 2,
                                portSize, portSize);
            }
        }

    }

    SynthSection::resized();
}

PreparationSection::~PreparationSection() {
    freeObjects();
}

BKPort *PreparationSection::createNewObject(const juce::ValueTree &v) {

    return new BKPort(v);
}

void PreparationSection::deleteObject(BKPort *at) {
    if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {
        SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();

        //this might cause bugs when adding deletion of a prepartion and dynamic port adding and delting
        at->setVisible(false);
        _parent->getOpenGlWrapper()->context.executeOnGLThread([this, &at](juce::OpenGLContext &openGLContext) {
                                                                   //this->destroyOpenGlComponent()
                                                               },
                                                               false);
    } else
        delete at;

}

void PreparationSection::reset() {
    SynthSection::reset();
}

void PreparationSection::newObjectAdded(BKPort *object) {

    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    parent->getGui()->open_gl_.context.executeOnGLThread([this,object](juce::OpenGLContext& context) {
        SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
        object->getImageComponent()->init(_parent->getGui()->open_gl_);
        juce::MessageManagerLock mm;
        this->addOpenGlComponent(object->getImageComponent(), false, true);
        this->addAndMakeVisible(object);
        object->addListener(this);
        this->resized();
    },false);
}

void PreparationSection::valueTreeRedirected(juce::ValueTree &) {
}



//void PreparationSection::valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
//{
//    //if (v == state)
//    //        if (i == IDs::name || i == IDs::colour)
//    //            repaint();
//}

//PreparationSection* PreparationList::createNewObject (const juce::ValueTree& v)
//{
//
//        auto s = prepFactory.CreateObject((int)v.getProperty(IDs::type), v, undo);
//        addSubSection(s);
//        return s;
//
//
//}

//PreparationList::PreparationList(juce::ValueTree editTree, juce::UndoManager& um)
//    : SynthSection("preplist"),
//      tracktion::engine::ValueTreeObjectList<PreparationSection>(editTree),
//      undo(um)
//{
//
//
//}

//void PreparationSection::mouseDoubleClick(const juce::MouseEvent &event)
//{
//    showPrepPopup(this);
//}
void PreparationSection::mouseDrag(const juce::MouseEvent &e)
{
        for (auto listener: listeners_) {
            listener->_update();
            listener->preparationDragged(this, e);
        }
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        dynamic_cast< ConstructionSite *>(getParentComponent())->startDragging(state.toXmlString(), this,juce::ScaledImage(),true);
    //    if (e.mods.isLeftButtonDown()) {
//        auto newPos = this->getPosition() + e.getDistanceFromDragStart();
//        this->setPosition(newPos);
//    }
}


