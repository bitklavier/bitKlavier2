//
// Created bydacis polito
//

#ifndef OPEN_GL_COMBOBOX_H
#define OPEN_GL_COMBOBOX_H

#include "juce_data_structures/juce_data_structures.h"
#include "open_gl_image_component.h"
#include "default_look_and_feel.h"
#include "BKLookAndFeel.h"
class OpenGLComboBox : public OpenGlAutoImageComponent<juce::ComboBox>{

public:
    OpenGLComboBox(std::string str, const juce::ValueTree &defaultState_ = {}) : OpenGlAutoImageComponent<juce::ComboBox> (str)
    {
        this->defaultState = defaultState_;
        image_component_ = std::make_shared<OpenGlImageComponent> ();
        setLookAndFeel(&laf);
        image_component_->setComponent(this);
        setComponentID(str);
        paramID = str;
    }

    ~OpenGLComboBox() {
        setLookAndFeel(nullptr);
    }

    virtual void resized() override
    {
        OpenGlAutoImageComponent<juce::ComboBox>::resized();
        redoImage();
    }

    void mouseEnter(const juce::MouseEvent &event) override
    {
        OpenGlAutoImageComponent<juce::ComboBox>::mouseEnter(event);
        for (auto* listener : listeners_)
            listener->hoverStarted(this);
    }

    void mouseExit(const juce::MouseEvent &event) override
    {
        OpenGlAutoImageComponent<juce::ComboBox>::mouseExit(event);
        for (auto* listener : listeners_)
            listener->hoverEnded(this);
    }

    void mouseDown(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<juce::ComboBox>::mouseDown(event);
        redoImage();

        if (isPopupActive())
            mouseInteraction = true;
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<juce::ComboBox>::mouseUp(event);
        redoImage();

        if (!isPopupActive())
            mouseInteraction = false;
    }

    void valueChanged(juce::Value &value) override {
        OpenGlAutoImageComponent<juce::ComboBox>::valueChanged(value);
        if(mouseInteraction && defaultState.isValid()) {
            int newVal = static_cast<int>(value.getValue()) - 1;
            defaultState.setProperty(juce::String(paramID), newVal,nullptr);
        }

        mouseInteraction = false;
        redoImage();
    }

    BKButtonAndMenuLAF laf;
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void hoverStarted(OpenGLComboBox* button) { }
        virtual void hoverEnded(OpenGLComboBox* button) { }
    };

    std::vector<Listener*> listeners_;
    void addListener(OpenGLComboBox::Listener* listener)
    {
        listeners_.push_back(listener);
    }

    bool mouseInteraction = false;

    juce::PopupMenu* clone() {
        auto menu = this->getRootMenu();
        return menu;
    }

    std::string getParamID() {
        return paramID;
    }

    std::string paramID;
    juce::ValueTree defaultState;
};



#endif //OPEN_GL_COMBOBOX_H
