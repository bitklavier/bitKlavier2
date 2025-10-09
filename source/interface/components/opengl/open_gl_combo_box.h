//
// Created bydacis polito
//

#ifndef OPEN_GL_COMBOBOX_H
#define OPEN_GL_COMBOBOX_H
#include "valuetree_utils/VariantConverters.h"

#include "juce_data_structures/juce_data_structures.h"
#include "open_gl_component.h"
#include "open_gl_image_component.h"
#include "default_look_and_feel.h"
#include "BKLookAndFeel.h"
class OpenGLComboBox : public OpenGlAutoImageComponent<juce::ComboBox>{

public:
    OpenGLComboBox(std::string str) : OpenGlAutoImageComponent<juce::ComboBox> (str)
    {
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
        for (auto* listener : listeners_)
            listener->hoverStarted(this);
        hovering_ = true;
    }

    void mouseExit(const juce::MouseEvent &event) override
    {
        for (auto* listener : listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }
    // void mouseUp(const juce::MouseEvent &event) override {
    //     OpenGlAutoImageComponent<juce::ComboBox>::mouseUp(event);
    //     redoImage();
    // }
    void valueChanged(juce::Value &value) override {
        OpenGlAutoImageComponent<juce::ComboBox>::valueChanged(value);
        redoImage();
    }
    // void mouseDown(const juce::MouseEvent &event) override {
    //     OpenGlAutoImageComponent<juce::ComboBox>::mouseDown(event);
    //     redoImage();
    // }
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
    bool hovering_ = false;
    // for components that are used to edit state modulation values
    bool isModulation_ = false;

    juce::PopupMenu* clone() {
       auto menu = this->getRootMenu();
        return menu;
    }
    std::string getParamID() {
        return paramID;
    }
    std::string paramID;
};



#endif //OPEN_GL_COMBOBOX_H
