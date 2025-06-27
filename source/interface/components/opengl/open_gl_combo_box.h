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
    }
    ~OpenGLComboBox() {
        setLookAndFeel(nullptr);
    }
    virtual void resized() override
    {
        OpenGlAutoImageComponent<juce::ComboBox>::resized();
        redoImage();
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
};



#endif //OPEN_GL_COMBOBOX_H
