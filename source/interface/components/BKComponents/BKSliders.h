//
// Created by Dan Trueman on 10/22/24.
//

#pragma once

#include "BKLookAndFeel.h"
#include "FullInterface.h"
#include "PreparationSection.h"
#include "../StateModulatedComponent.h"
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>

// ******************************************************************************************************************** //
// **************************************************  BKSubSlider **************************************************** //
// ******************************************************************************************************************** //

class BKSubSlider : public juce::Slider
{
public:

    BKSubSlider (SliderStyle sstyle, double min, double max, double def, double increment, int width, int height);
    ~BKSubSlider();

    double getValueFromText    (const juce::String & text ) override;
    bool isActive() { return active; }
    void isActive(bool newactive) {active = newactive; }
    void setMinMaxDefaultInc(std::vector<float> newvals);
    void setSkewFromMidpoint(bool sfm);

private:

    double sliderMin, sliderMax;
    double sliderDefault;
    double sliderIncrement;

    int sliderWidth, sliderHeight;

    bool sliderIsVertical;
    bool sliderIsBar;
    bool skewFromMidpoint;

    bool active;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKSubSlider)
};




// ******************************************************************************************************************** //
// **************************************************  BKMultiSlider ************************************************** //
// ******************************************************************************************************************** //

/*

 BKMultiSlider is a horizontal array of sliders.

 Each "slider" can have multiple sliders, facilitating, for instance, multiple transposition values

 The user can drag across the sliders to quickly set values

 The user can also enter slider values directly via a text editor:
    "1 [-1, -2, -3] 2 3" will create three sliders, and the second will have three overlaid sliders

 By default it has 12 sliders, but that can be increased by adding additional values via text entry

 Gaps in the slider array are possible, and indicated by a forward slash '/' in the text editor

 The API includes functions for setting the slider, either all at once (via text) or by individual slider (via mouse)
 and for deactivating individual sliders or sections of sliders.

*/

class BKMultiSlider :
    public StateModulatedComponent,
    public juce::Slider::Listener,
    public juce::TextEditor::Listener,
    public juce::ImageButton::Listener
{

public:

    // *** Public Instance Methods *** //

    // constructor
    BKMultiSlider(const juce::ValueTree& defaultState={});
    ~BKMultiSlider();

    // when the client sends an array of only the active slider values, this will construct the complete array
    // of slider values, including inactive sliders, and then call setTo. So
    //      newActiveVals = {1, 2, 3}
    //      newactives = {true, false, false, true, true, false....... false}
    // will result in [1 / / 2 3 / / / / / / /], where / represents an inactive slider (gap)
    void setToOnlyActive(juce::Array<juce::Array<float>> newActiveVals, juce::Array<bool> newactives, juce::NotificationType newnotify);

    // as above, but takes a 1d array, for sliders that don't use subSliders
    void setToOnlyActive(juce::Array<float> newActiveVals, juce::Array<bool> newactives, juce::NotificationType newnotify);

    // identifiers
    void setName(juce::String newName){
        sliderName = newName;
        showName.setText(sliderName, juce::dontSendNotification);
    }
    juce::String getName() { return sliderName; }
    void setToolTipString(juce::String newTip) { showName.setTooltip(newTip); bigInvisibleSlider->setTooltip(newTip); }

    // slider range and skew
    void setMinMaxDefaultInc(std::vector<float> newvals);
    void setSkewFromMidpoint(bool sfm);

    // highlight whichever slider is currently active in bK
    void setCurrentSlider(int activeSliderNum);
    void deHighlightCurrentSlider(void);
    inline int getNumVisible(void) const noexcept { return allSliderVals.size(); } //return numVisibleSliders;}

    // should this slider allow subsliders? (multiple transpositions, for instance)
    void setAllowSubSlider(bool ss) { allowSubSliders = ss; }

    // name of the subslider (add transposition, for instance)
    void setSubSliderName(juce::String ssname) { subSliderName = ssname; }

    // listeners overrride these functions so they can get what they need from MultiSlider
    // Multislider will call these when values change, so the clients can update
    class Listener
    {
    public:

        virtual ~Listener() {};
        virtual void multiSliderValueChanged(juce::String name, int whichSlider, juce::Array<float> values) = 0;
        virtual void multiSliderAllValuesChanged(juce::String name, juce::Array<juce::Array<float>> values, juce::Array<bool> states) = 0;
    };

    // listeners
    juce::ListenerList<Listener> listeners;
    void addMyListener(Listener* listener)     { listeners.add(listener);      }
    void removeMyListener(Listener* listener)  { listeners.remove(listener);   }


//private:

    // *** Private Instance Methods *** //

    // this is the main function for setting the multislider
    // the array of arrays is the slider values, with the inner array for subsliders (multiple sliders at one position
    // the array of bools sets which of the sliders is actually active
    void setTo(juce::Array<juce::Array<float>> newvals, juce::Array<bool> newactives, juce::NotificationType newnotify);

    // initialize the slider; it should have no less than numDefaultSliders, all set to sliderDefault value
    void initializeSliderVals(int howmany);

    // update and activate a particular slider
    void updateSliderVal(int which, int whichSub, float val);

    // print out the current vals, for debugging
    void printSliderVals();

    // draw the actual sliders
    void drawSliders(juce::NotificationType newnotify);

    // return all values, including inactive sliders
    juce::Array<juce::Array<float>> getAllValues() { return allSliderVals; }

    // return only those sliders that are active
    juce::Array<juce::Array<float>> getAllActiveValues();

    // return the values for one slider
    juce::Array<float> getOneSliderBank(int which);

    // inserts a slider into the multislider
    void addSlider(int where, bool active, juce::NotificationType newnotify);

    // adds an additional slider at a particular index; it's possible to have multiple
    // sliders at one location, facilitating, for instance, multiple transpositions at one time
    void addSubSlider(int where, bool active, juce::NotificationType newnotify);

    // as above, but updates the allSliderVals and whichSlidersActive first
    void addActiveSubSlider(int where, juce::NotificationType newnotify);

    // these methods clear currently active sliders
    // exception: you can't clear the FIRST slider, as we need at least one active slider
    void deactivateSlider(int where, juce::NotificationType notify);
    void deactivateAll(juce::NotificationType notify);
    void deactivateAllAfter(int where, juce::NotificationType notify);
    void deactivateAllBefore(int where, juce::NotificationType notify);

    // slider values can be edited directly via a text editor
    // values in brackets will be collected at one index
    // for example: "0 [1 2 3] 4" will have three active sliders, and the second one will have three subsliders
    inline void setText(juce::String text) { editValsTextField->setText(text, juce::dontSendNotification); }
    inline juce::TextEditor* getTextEditor(void) { return editValsTextField.get();}
    inline void dismissTextEditor(bool setValue = false);

    // identify which slider is mouseDowned
    int whichSlider (const juce::MouseEvent &e);

    // identify which subSlider is closest to cursor after mouseDown on a particular slider
    int whichSubSlider (int which);

    // identify which subSlider is closest on mouseOver
    int whichSubSlider (int which, const juce::MouseEvent &e);

    // update which slider is active after mouseDrag and mouseUp; to support dragging across multiple sliders
    int whichActiveSlider (int which);

    // for highlighting text related to sliders
    void highlight(int activeSliderNum);
    void deHighlight(int sliderNum);
    int getActiveSlider(int sliderNum);

    // rescale/normalize slider ranges
    void resetRanges();

    // mouse/text events
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent &e) override;
    void mouseDown (const juce::MouseEvent &event) override;
    void mouseUp (const juce::MouseEvent &event) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& textEditor) override;
    void sliderValueChanged (juce::Slider *slider) override;
    void textEditorReturnKeyPressed(juce::TextEditor& textEditor) override;
    void textEditorFocusLost(juce::TextEditor& textEditor) override;
    void textEditorTextChanged(juce::TextEditor&) override;
    void buttonClicked(juce::Button* button) override;
    void showModifyPopupMenu(int which);
    static void sliderModifyMenuCallback (const int result, BKMultiSlider* slider, int which);

    void resized() override;


    // *** Instance Variables *** //

    // holds all the current values, including for sub sliders (hence the double array)
    juce::Array<juce::Array<float>> allSliderVals;

    // which of these sliders is active; the first is always true
    juce::Array<bool> whichSlidersActive;

    // the invisible slider lays on top of all the sliders, its value
    // is used to set the values of the individual slider that the mouse is nearest
    double currentInvisibleSliderValue;

    // allow sliders to have multiple values
    bool allowSubSliders;

    // if allowSubSliders, what to call it "add [subSliderName]"
    juce::String subSliderName;

    // to keep track of stuff
    int currentSubSlider;
    int lastHighlightedSlider;
    bool focusLostByEscapeKey;
    bool skewFromMidpoint;
    float clickedHeight;

    // default values
    double sliderMin, sliderMax, sliderMinDefault, sliderMaxDefault;
    double sliderDefault;
    double sliderIncrement;

    // dimensions
    int totalWidth;
    int sliderHeight;
    float sliderWidth;
    int displaySliderWidth;

    // by default, how many sliders to show (12)
    int numDefaultSliders;

    // UI elements
    BKMultiSliderLookAndFeel activeSliderLookAndFeel;
    BKMultiSliderLookAndFeel passiveSliderLookAndFeel;
    BKMultiSliderLookAndFeel highlightedSliderLookAndFeel;
    BKMultiSliderLookAndFeel displaySliderLookAndFeel;
    juce::String sliderName;
    juce::Label showName;
    juce::Slider::SliderStyle subsliderStyle;
    juce::OwnedArray<juce::OwnedArray<BKSubSlider>> sliders;
    std::unique_ptr<BKSubSlider> displaySlider;
    std::unique_ptr<BKSubSlider> bigInvisibleSlider;
    std::unique_ptr<juce::TextEditor> editValsTextField;
//    std::unique_ptr<juce::ImageButton> rotateButton;
    std::unique_ptr<OpenGlShapeButton> rotateButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKMultiSlider)
};


// ******************************************************************************************************************** //
// *************************************************  BKStackedSlider ************************************************* //
// ******************************************************************************************************************** //

class BKStackedSlider : public StateModulatedComponent, // needed for this to be param-state modulatable
                        public juce::Slider::Listener,
                        public juce::TextEditor::Listener
{
public:
    BKStackedSlider (
        juce::String sliderName,
        double min,
        double max,
        double defmin,
        double defmax,
        double def,
        double increment,
        int numActiveSliders,
        const juce::ValueTree& defaultState={});

    ~BKStackedSlider()
    {
        topSlider->setLookAndFeel (nullptr);

        for (int i = 0; i < numSliders; i++)
        {
            juce::Slider* newSlider = dataSliders.operator[] (i);
            if (newSlider != nullptr)

                newSlider->setLookAndFeel (nullptr);
        }
    }

    void sliderValueChanged (juce::Slider* slider) override;
    void textEditorReturnKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorFocusLost (juce::TextEditor& textEditor) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorTextChanged (juce::TextEditor& textEditor) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    inline juce::TextEditor* getTextEditor (void)
    {
        return editValsTextField.get();
    }

    inline void dismissTextEditor (bool setValue = false)
    {
        if (setValue)
            textEditorReturnKeyPressed (*editValsTextField);
        else
            textEditorEscapeKeyPressed (*editValsTextField);
    }

    void setTo (juce::Array<float> newvals, juce::NotificationType newnotify);
    void setValue (juce::Array<float> newvals, juce::NotificationType newnotify) { setTo (newvals, newnotify); }
    void resetRanges();
    int whichSlider();
    int whichSlider (const juce::MouseEvent& e);
    virtual void addSlider (juce::NotificationType newnotify);
    inline juce::String getText (void) { return editValsTextField->getText(); }
    inline void setText (juce::String text) { editValsTextField->setText (text, juce::dontSendNotification); }

    void setName (juce::String newName)
    {
        sliderName = newName;
        //showName.setText (sliderName, juce::dontSendNotification);
    }

    juce::String getName() { return sliderName; }

    void setTooltip (juce::String newTip)
    {
        topSlider->setTooltip (newTip);
        //showName.setTooltip (newTip);
    }

    void resized() override;
    void setDim (float newAlpha);
    void setBright();

    class Listener
    {
    public:
        virtual ~Listener() {
        };

        virtual void BKStackedSliderValueChanged (juce::String name, juce::Array<float> val) = 0;

        //rewrite all this to pass "this" and check by slider ref instead of name?
    };

    juce::ListenerList<Listener> listeners;
    void addMyListener (Listener* listener) { listeners.add (listener); }
    void removeMyListener (Listener* listener) { listeners.remove (listener); }

    juce::OwnedArray<juce::Slider> dataSliders; //displays data, user controls with topSlider

    /*
     * needed for the state modulation system
     */
    BKStackedSlider* clone()
    {
        int i = 0;
        for (auto slider : activeSliders)
        {
            if (slider)
            {
                i++;
            }
        }
        return new BKStackedSlider (sliderName, sliderMin, sliderMax, sliderMinDefault, sliderMaxDefault, sliderDefault, sliderIncrement, i);
    }


    void syncToValueTree()
    {
    }

    juce::Array<float> getAllActiveValues();
    bool isEditing;

private
    :
    chowdsp::SliderAttachment attachment;
    std::unique_ptr<juce::Slider> topSlider; //user interacts with this
    juce::Array<bool> activeSliders;
    std::unique_ptr<juce::TextEditor> editValsTextField;

    int numSliders;
    int numActiveSliders;
    int clickedSlider;
    float clickedPosition;

    juce::String sliderName;
    //juce::Label showName;
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

    juce::GroupComponent sliderBorder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKStackedSlider)
};

// ******************************************************************************************************************** //
// **************************************************  BKRangeSlider ************************************************** //
// ******************************************************************************************************************** //

typedef enum BKRangeSliderType {
    BKRangeSliderMin = 0,
    BKRangeSliderMax,
    BKRangeSliderNil

} BKRangeSliderType;

class BKRangeSlider : public StateModulatedComponent,
                      public juce::Slider::Listener,
                      public juce::TextEditor::Listener
#if JUCE_IOS
    ,
                      public WantsBigOne
#endif
{
public:
    BKRangeSlider (juce::String sliderName, double min, double max, double defmin, double defmax, double increment, const juce::ValueTree& stateDefault);
    ~BKRangeSlider()
    {
        minSlider.setLookAndFeel (nullptr);
        maxSlider.setLookAndFeel (nullptr);
        invisibleSlider.setLookAndFeel (nullptr);
        displaySlider->setLookAndFeel (nullptr);
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

    void setName (juce::String newName)
    {
        sliderName = newName;
        showName.setText (sliderName, juce::dontSendNotification);
    }
    juce::String getName() { return sliderName; }

    void setToolTipString (juce::String newTip)
    {
        showName.setTooltip (newTip);
        invisibleSlider.setTooltip (newTip);
        minValueTF.setTooltip (newTip);
        maxValueTF.setTooltip (newTip);
    }

    void setMinValue (double newval, juce::NotificationType notify);
    void setMaxValue (double newval, juce::NotificationType notify);
    void setIsMinAlwaysLessThanMax (bool im) { isMinAlwaysLessThanMax = im; }

    double getMinValue() { return sliderMin; }
    double getMaxValue() { return sliderMax; }

    void setDisplayValue (double newval) { displaySlider->setValue (newval); }
    void displaySliderVisible (bool vis) { displaySlider->setVisible (vis); }

    void setJustifyRight (bool jr)
    {
        justifyRight = jr;
        if (justifyRight)
            showName.setJustificationType (juce::Justification::bottomRight);
        else
            showName.setJustificationType (juce::Justification::bottomLeft);
    }

    inline void setText (BKRangeSliderType which, juce::String text)
    {
        if (which == BKRangeSliderMin)
            minValueTF.setText (text, false);
        else if (which == BKRangeSliderMax)
            maxValueTF.setText (text, false);
    }

    inline juce::TextEditor* getTextEditor (BKRangeSliderType which)
    {
        if (which == BKRangeSliderMin)
            return &minValueTF;
        if (which == BKRangeSliderMax)
            return &maxValueTF;

        return nullptr;
    }

    inline void dismissTextEditor (bool setValue = false)
    {
        if (setValue)
        {
            textEditorReturnKeyPressed (minValueTF);
            textEditorReturnKeyPressed (maxValueTF);
        }
        else
        {
            textEditorEscapeKeyPressed (minValueTF);
            textEditorEscapeKeyPressed (maxValueTF);
        }
    }

    void checkValue (double newval);
    void rescaleMinSlider();
    void rescaleMaxSlider();

    void sliderValueChanged (juce::Slider* slider) override;
    void textEditorReturnKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorFocusLost (juce::TextEditor& textEditor) override;
    void textEditorEscapeKeyPressed (juce::TextEditor& textEditor) override;
    void textEditorTextChanged (juce::TextEditor& textEditor) override;
    void resized() override;
    void sliderDragEnded (juce::Slider* slider) override;
    void mouseDown (const juce::MouseEvent& event) override;

    void setDim (float newAlpha);
    void setBright();

    class Listener
    {
    public:
        virtual ~Listener() {};

        virtual void BKRangeSliderValueChanged (juce::String name, double min, double max) = 0;
    };

    juce::ListenerList<Listener> listeners;
    void addMyListener (Listener* listener) { listeners.add (listener); }
    void removeMyListener (Listener* listener) { listeners.remove (listener); }

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

    juce::GroupComponent rangeSliderBorder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKRangeSlider)
};