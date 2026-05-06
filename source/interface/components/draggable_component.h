// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Davis Polito on 1/17/24.
//

#ifndef BITKLAVIER2_DRAGGABLE_COMPONENT_H
#define BITKLAVIER2_DRAGGABLE_COMPONENT_H
#include "open_gl_image_component.h"
#include "open_gl_multi_image.h"
#include "common.h"

class DraggableComponent    : public juce::Component, public juce::DragAndDropTarget
{
public:

    DraggableComponent(bool isDrag, bool isDrop, bool isConn):
                                                                   isDraggable(isDrag),
                                                                   isDroppable(isDrop),
                                                                   isConnectable(isConn)
    {
        constrainer.setMinimumOnscreenAmounts(50,50,50,50);
        addMouseListener(this,true);
    }

    DraggableComponent(bool isDrag, bool isDrop, bool isConn,
        int top, int left, int bottom, int right):
                     isDraggable(isDrag),
                     isDroppable(isDrop),
                     isConnectable(isConn)
    {
        constrainer.setMinimumOnscreenAmounts(top,left,bottom,right);
        addMouseListener(this,true);
    }

    ~DraggableComponent()
    {
        removeMouseListener(this);
    }

    inline void setConstrainer(int top, int left, int bottom, int right)
    {

    }

    //virtual void itemWasDropped(BKPreparationType type, juce::Array<int>, int x, int y){};
    virtual void itemIsBeingDragged(const juce::MouseEvent& e){};
    virtual void itemWasDragged(const juce::MouseEvent& e){};
    virtual void keyPressedWhileSelected(const juce::KeyPress& e) {};

    bool isDragging;

    void prepareDrag(const juce::MouseEvent& e)
    {
        isDragging = true;

        dragger.startDraggingComponent (this, e);
    }

    void performDrag(const juce::MouseEvent& e)
    {
        dragger.dragComponent (this, e, &constrainer);

        itemIsBeingDragged(e);
    }

protected:
    bool itemIsHovering, isSelected;

    bool isDraggable, isDroppable, isConnectable;

private:
    // Drag interface
    bool isInterestedInDragSource (const SourceDetails& /*dragSourceDetails*/) override
    {
        // should check drag source to see if its what we want...
        return true;
    }

    void itemDragEnter (const SourceDetails& /*dragSourceDetails*/) override
    {
        itemIsHovering = true;
        if (isDroppable) repaint();
    }

    void itemDragMove (const SourceDetails& /*dragSourceDetails*/) override
    {

    }

    void itemDragExit (const SourceDetails& /*dragSourceDetails*/) override
    {
        itemIsHovering = false;
        if (isDroppable) repaint();
    }

    void itemDropped (const SourceDetails& dragSourceDetails) override
    {
        juce::String received = dragSourceDetails.description.toString();

        juce::Point<int> where = dragSourceDetails.localPosition;

        if (isDroppable)
        {
            //itemWasDropped(type, data, where.getX(), where.getY());

            itemIsHovering = false;

            repaint();
        }
    }

    juce::ComponentBoundsConstrainer constrainer;
    juce::ComponentDragger dragger;

    juce::Point<int> startPosition;
    juce::Point<int> vector;


    JUCE_LEAK_DETECTOR (DraggableComponent)
};



#endif //BITKLAVIER2_DRAGGABLE_COMPONENT_H
