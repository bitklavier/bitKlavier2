//
// Created by Dan Trueman on 10/22/24.
//

#ifndef BITKLAVIER2_BKSLIDERS_H
#define BITKLAVIER2_BKSLIDERS_H

#endif //BITKLAVIER2_BKSLIDERS_H

#include "FullInterface.h"
#include "PreparationSection.h"
#include "BKLookAndFeel.h"
#include "StateModulatedComponent.h"
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

/**
 * BKStacked Slider
 *
 * for having multiple simultaneous overlaid sliders
 * main application: transposition sliders in Direct and Nostalgic
 *
 */
class BKStackedSlider :
    public StateModulatedComponent,
    public juce::Slider::Listener,
    public juce::TextEditor::Listener
{
public:

    BKStackedSlider(juce::String sliderName, double min, double max, double defmin, double defmax, double def, double increment);

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
    void addSlider(juce::NotificationType newnotify);

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
        return new BKStackedSlider(sliderName, sliderMin, sliderMax, sliderMinDefault, sliderMaxDefault, sliderDefault, sliderIncrement);
    }
    void syncToValueTree()
    {}
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

    juce::Array<float> getAllActiveValues();

    void showModifyPopupMenu();
    static void sliderModifyMenuCallback (const int result, BKStackedSlider* ss);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKStackedSlider)
};