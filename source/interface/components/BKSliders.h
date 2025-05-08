//
// Created by Dan Trueman on 10/22/24.
//


#pragma once

#include "FullInterface.h"
#include "PreparationSection.h"
#include "BKLookAndFeel.h"
#include "StateModulatedComponent.h"
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

// ******************************************************************************************************************** //
// *************************************************  BKStackedSlider ************************************************* //
// ******************************************************************************************************************** //

class BKStackedSlider :
    public StateModulatedComponent,
    public juce::Slider::Listener,
    public juce::TextEditor::Listener
{
public:

    BKStackedSlider(juce::String sliderName, double min, double max, double defmin, double defmax, double def, double increment, int numActiveSliders);

    ~BKStackedSlider()
    {
        topSlider->setLookAndFeel(nullptr);

        for(int i=0; i<numSliders; i++)
        {
            juce::Slider* newSlider = dataSliders.operator[](i);
            if(newSlider != nullptr)
            {
                newSlider->setLookAndFeel(nullptr);
            }
        }
    };

    void sliderValueChanged (juce::Slider *slider) override;
    void textEditorReturnKeyPressed(juce::TextEditor& textEditor) override;
    void textEditorFocusLost(juce::TextEditor& textEditor) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorTextChanged(juce::TextEditor& textEditor) override;
    void mouseDown (const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent &e) override;

    inline juce::TextEditor* getTextEditor(void)
    {
        return editValsTextField.get();
    }

    inline void dismissTextEditor(bool setValue = false)
    {
        if (setValue)   textEditorReturnKeyPressed(*editValsTextField);
        else            textEditorEscapeKeyPressed(*editValsTextField);
    }

    void setTo(juce::Array<float> newvals, juce::NotificationType newnotify);
    void setValue(juce::Array<float> newvals, juce::NotificationType newnotify) { setTo(newvals, newnotify); }
    void resetRanges();
    int whichSlider();
    int whichSlider(const juce::MouseEvent& e);
    virtual void addSlider(juce::NotificationType newnotify);

    inline juce::String getText(void) { return editValsTextField->getText(); }
    inline void setText(juce::String text) { editValsTextField->setText(text, juce::dontSendNotification); }

    void setName(juce::String newName)    { sliderName = newName; showName.setText(sliderName, juce::dontSendNotification); }
    juce::String getName()                { return sliderName; }
    void setTooltip(juce::String newTip)  { topSlider->setTooltip(newTip); showName.setTooltip(newTip); }

    void resized() override;

    void setDim(float newAlpha);
    void setBright();

    class Listener
    {

    public:

        virtual ~Listener() {};

        virtual void BKStackedSliderValueChanged(juce::String name, juce::Array<float> val) = 0; //rewrite all this to pass "this" and check by slider ref instead of name?
    };

    juce::ListenerList<Listener> listeners;
    void addMyListener(Listener* listener)     { listeners.add(listener);      }
    void removeMyListener(Listener* listener)  { listeners.remove(listener);   }

    juce::OwnedArray<juce::Slider> dataSliders;  //displays data, user controls with topSlider
    BKStackedSlider* clone ()
    {
        int i = 0;
        for (auto slider : activeSliders) {
            if (slider) {i++;}
        }
        return new BKStackedSlider(sliderName, sliderMin, sliderMax, sliderMinDefault, sliderMaxDefault, sliderDefault, sliderIncrement,i);
    }
    void syncToValueTree() {

    }
    juce::Array<float> getAllActiveValues();


private:

    chowdsp::SliderAttachment attachment;

    std::unique_ptr<juce::Slider> topSlider; //user interacts with this

    juce::Array<bool> activeSliders;

    std::unique_ptr<juce::TextEditor> editValsTextField;

    int numSliders;
    int numActiveSliders;
    int clickedSlider;
    float clickedPosition;

    juce::String sliderName;
    juce::Label showName;
    bool justifyRight;

    BKMultiSliderLookAndFeel stackedSliderLookAndFeel;
    BKMultiSliderLookAndFeel topSliderLookAndFeel;

    double sliderMin, sliderMax, sliderMinDefault, sliderMaxDefault;
    double sliderDefault;
    double sliderIncrement;
    double currentDisplaySliderValue;

    bool focusLostByEscapeKey;
    bool focusLostByNumPad;
    bool mouseJustDown;

        void showModifyPopupMenu();
    static void sliderModifyMenuCallback (const int result, BKStackedSlider* ss);


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKStackedSlider)
};


// ******************************************************************************************************************** //
// **************************************************  BKRangeSlider ************************************************** //
// ******************************************************************************************************************** //

typedef enum BKRangeSliderType
{
    BKRangeSliderMin = 0,
    BKRangeSliderMax,
    BKRangeSliderNil

} BKRangeSliderType;

class BKRangeSlider :
    public StateModulatedComponent,
    public juce::Slider::Listener,
    public juce::TextEditor::Listener
#if JUCE_IOS
    , public WantsBigOne
#endif
{
public:
    BKRangeSlider(juce::String sliderName, double min, double max, double defmin, double defmax, double increment);
    ~BKRangeSlider()
    {
        minSlider.setLookAndFeel(nullptr);
        maxSlider.setLookAndFeel(nullptr);
        invisibleSlider.setLookAndFeel(nullptr);
        displaySlider->setLookAndFeel(nullptr);
    };

    juce::Slider minSlider;
    juce::Slider maxSlider;
    juce::String minSliderName;
    juce::String maxSliderName;

    juce::Slider invisibleSlider;

    std::unique_ptr<juce::Slider> displaySlider;

    juce::String sliderName;
    juce::Label showName;

    juce::TextEditor minValueTF;
    juce::TextEditor maxValueTF;

    void setName(juce::String newName)    { sliderName = newName; showName.setText(sliderName, juce::dontSendNotification); }
    juce::String getName()                { return sliderName; }
    void setToolTipString(juce::String newTip) {  showName.setTooltip(newTip);
        invisibleSlider.setTooltip(newTip);
        minValueTF.setTooltip(newTip);
        maxValueTF.setTooltip(newTip); }

    void setMinValue(double newval, juce::NotificationType notify);
    void setMaxValue(double newval, juce::NotificationType notify);
    void setIsMinAlwaysLessThanMax(bool im) { isMinAlwaysLessThanMax = im; }

    double getMinValue() { return sliderMin; }
    double getMaxValue() { return sliderMax; }

    void setDisplayValue(double newval) { displaySlider->setValue(newval); }
    void displaySliderVisible(bool vis) { displaySlider->setVisible(vis); }

    void setJustifyRight(bool jr)
    {
        justifyRight = jr;
        if (justifyRight) showName.setJustificationType(juce::Justification::bottomRight);
        else showName.setJustificationType(juce::Justification::bottomLeft);
    }

    inline void setText(BKRangeSliderType which, juce::String text)
    {
        if (which == BKRangeSliderMin)      minValueTF.setText(text, false);
        else if (which == BKRangeSliderMax) maxValueTF.setText(text, false);
    }

    inline juce::TextEditor* getTextEditor(BKRangeSliderType which)
    {
        if (which == BKRangeSliderMin) return &minValueTF;
        if (which == BKRangeSliderMax) return &maxValueTF;

        return nullptr;
    }

    inline void dismissTextEditor(bool setValue = false)
    {
        if (setValue)
        {
            textEditorReturnKeyPressed(minValueTF);
            textEditorReturnKeyPressed(maxValueTF);
        }
        else
        {
            textEditorEscapeKeyPressed(minValueTF);
            textEditorEscapeKeyPressed(maxValueTF);
        }
    }

    void checkValue(double newval);
    void rescaleMinSlider();
    void rescaleMaxSlider();

    void sliderValueChanged (juce::Slider *slider) override;
    void textEditorReturnKeyPressed(juce::TextEditor& textEditor) override;
    void textEditorFocusLost(juce::TextEditor& textEditor) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorTextChanged(juce::TextEditor& textEditor) override;
    void resized() override;
    void sliderDragEnded(juce::Slider *slider) override;
    void mouseDown (const juce::MouseEvent &event) override;

    void setDim(float newAlpha);
    void setBright();

    class Listener
    {

    public:
        virtual ~Listener() {};

        virtual void BKRangeSliderValueChanged(juce::String name, double min, double max) = 0;
    };

    juce::ListenerList<Listener> listeners;
    void addMyListener(Listener* listener)     { listeners.add(listener);      }
    void removeMyListener(Listener* listener)  { listeners.remove(listener);   }


private:

    double sliderMin, sliderMax;
    double sliderDefaultMin, sliderDefaultMax;
    double sliderIncrement;

    bool newDrag;
    bool clickedOnMinSlider;
    bool isMinAlwaysLessThanMax;
    bool focusLostByEscapeKey;
    bool justifyRight;

    BKRangeMinSliderLookAndFeel minSliderLookAndFeel;
    BKRangeMaxSliderLookAndFeel maxSliderLookAndFeel;
    BKDisplaySliderLookAndFeel displaySliderLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKRangeSlider)

};