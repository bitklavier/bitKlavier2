//
// Created by Dan Trueman on 10/22/24.
//

#include "BKSliders.h"
#include "BKGraphicsConstants.h"
#include "BinaryData.h"

/**
 * move this stuff somewhere else
 */

juce::String BKfloatArrayToString(juce::Array<float> arr)
{
    juce::String s = "";
    for (auto key : arr)
    {
        s.append(juce::String(key), 6);
        s.append(" ", 1);
    }
    return s;
}

juce::Array<float> BKstringToFloatArray(juce::String s)
{
    juce::Array<float> arr = juce::Array<float>();

    juce::String temp = "";
    bool inNumber = false;

    juce::String::CharPointerType c = s.getCharPointer();

    juce::juce_wchar prd = '.';
    juce::juce_wchar dash = '-';
    juce::juce_wchar slash = '/'; // blank: put a zero in

    int prdCnt = 0;

    // DEBUG
    for (int i = 0; i < (s.length()+1); i++)
    {
        juce::juce_wchar c1 = c.getAndAdvance();

        bool isPrd = !juce::CharacterFunctions::compare(c1, prd);
        bool isDash = !juce::CharacterFunctions::compare(c1, dash);
        bool isSlash = !juce::CharacterFunctions::compare(c1, slash);

        if (isPrd) prdCnt += 1;

        bool isNumChar = juce::CharacterFunctions::isDigit(c1) || isPrd || isDash;

        if (!isNumChar)
        {
            if (inNumber)
            {
                arr.add(temp.getFloatValue());
                temp = "";
            }

            // slash indicates a zero slot
            if (isSlash) {
                arr.add(0.);
                temp = "";
            }

            inNumber = false;
            continue;
        }
        else
        {
            inNumber = true;

            temp += c1;
        }
    }

    return arr;
}

juce::String arrayActiveFloatArrayToString(juce::Array<juce::Array<float>> afarr, juce::Array<bool> act)
{
    juce::String s = "";

    int boolCtr = 0;

    for (auto arr : afarr)
    {
        if (arr.size() > 1)
        {
            s += "[";
            for (auto f : arr)
            {
                s += juce::String(f) + " ";
            }
            s += "] ";
        }
        else
        {
            if (act[boolCtr]) {
                s += juce::String(arr[0]) + " ";
            }
            else
            {
                s +=  "/ ";
            }
        }

        boolCtr++;
    }
    return s;
}

juce::Array<float> stringToFloatArray(juce::String s)
{
    juce::Array<float> arr = juce::Array<float>();

    juce::String temp = "";
    bool inNumber = false;

    juce::String::CharPointerType c = s.getCharPointer();

    juce::juce_wchar prd = '.';
    juce::juce_wchar dash = '-';
    juce::juce_wchar slash = '/'; // blank: put a zero in

    int prdCnt = 0;

    // DEBUG
    for (int i = 0; i < (s.length()+1); i++)
    {
        juce::juce_wchar c1 = c.getAndAdvance();

        bool isPrd = !juce::CharacterFunctions::compare(c1, prd);
        bool isDash = !juce::CharacterFunctions::compare(c1, dash);
        bool isSlash = !juce::CharacterFunctions::compare(c1, slash);

        if (isPrd) prdCnt += 1;

        bool isNumChar = juce::CharacterFunctions::isDigit(c1) || isPrd || isDash;

        if (!isNumChar)
        {
            if (inNumber)
            {
                arr.add(temp.getFloatValue());
                temp = "";
            }

            // slash indicates a zero slot
            if (isSlash) {
                arr.add(0.);
                temp = "";
            }

            inNumber = false;
            continue;
        }
        else
        {
            inNumber = true;

            temp += c1;
        }
    }

    return arr;
}

juce::Array<juce::Array<float>> stringToArrayFloatArray(juce::String s)
{
    juce::Array<juce::Array<float>> afarr;

    juce::String rest = s;

    // "4"
    while (rest.length())
    {
        juce::String sub = rest.upToFirstOccurrenceOf("[", false, true);

        juce::Array<float> ind = stringToFloatArray(sub);

        for (auto f : ind)
        {
            juce::Array<float> arr; arr.add(f);
            afarr.add(arr);
        }

        if (sub == rest) break; // no [ in s

        rest = rest.substring(sub.length()+1);

        sub = rest.upToFirstOccurrenceOf("]", false, true);

        if (sub != rest)
        {
            juce::Array<float> group = stringToFloatArray(sub);

            afarr.add(group);
        }

        rest = rest.substring(sub.length()+1);
    }

    return afarr;
}

// reads through string: single whitespaces = true, slashes = false
// assume whitespaces follow values that we want, so all single whitespaces
// set to true, and otherwise set slashes to false
// need to ignore []
juce::Array<bool> slashToFalse(juce::String s)
{
    juce::Array<bool> arr = juce::Array<bool>();
    s.append(" ", 1); // to get the last value

    juce::String::CharPointerType c = s.getCharPointer();
    juce::juce_wchar slash = '/'; // blank: put a zero in
    juce::juce_wchar leftbracket = '[';
    juce::juce_wchar rightbracket = ']';

    bool precedingIsSpace = true;
    bool precedingIsSlash = false;
    bool inBracket = false;

    for (int i = 0; i < (s.length() + 1); i++)
    {
        juce::juce_wchar c1 = c.getAndAdvance();

        if (!juce::CharacterFunctions::compare(c1, leftbracket))
            inBracket = true;

        if (!inBracket) {
            if (!juce::CharacterFunctions::compare(c1, slash)) {
                arr.add(false);
                precedingIsSlash = true;
            }
            else if(juce::CharacterFunctions::isWhitespace(c1)) {
                if (!precedingIsSlash && !precedingIsSpace) arr.add(true);
                precedingIsSpace = true;
            }
            else {
                precedingIsSpace = false;
                precedingIsSlash = false;
            }
        }
        else if (!juce::CharacterFunctions::compare(c1, rightbracket))
        {
            inBracket = false;
            precedingIsSpace = false;
            precedingIsSlash = false;
        }
    }

    return arr;
}

// ******************************************************************************************************************** //
// **************************************************  BKSubSlider **************************************************** //
// ******************************************************************************************************************** //

//used in BKMultSlider

BKSubSlider::BKSubSlider (SliderStyle sstyle, double min, double max, double def, double increment, int width, int height):
                                                                                                                             sliderMin(min),
                                                                                                                             sliderMax(max),
                                                                                                                             sliderDefault(def),
                                                                                                                             sliderIncrement(increment),
                                                                                                                             sliderWidth(width),
                                                                                                                             sliderHeight(height)
{

    setSliderStyle(sstyle);
    active = true;

    if(sstyle == LinearVertical || sstyle == LinearBarVertical) sliderIsVertical = true;
    else sliderIsVertical = false;

    if(sstyle == LinearBarVertical || sstyle == LinearBar) sliderIsBar = true;
    else sliderIsBar = false;

    if(!sliderIsBar)
    {
        if(sliderIsVertical) setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, false, 50, 20);
        else setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, false, 50, 20);
    }
    else
    {
        if(sliderIsVertical) setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    }

    setRange(sliderMin, sliderMax, sliderIncrement);
    setValue(sliderDefault, juce::dontSendNotification);
    setSkewFromMidpoint(true);
}

BKSubSlider::~BKSubSlider()
{

}

void BKSubSlider::setMinMaxDefaultInc(std::vector<float> newvals)
{
    sliderMin = newvals[0];
    sliderMax = newvals[1];
    sliderDefault = newvals[2];
    sliderIncrement = newvals[3];
    setRange(sliderMin, sliderMax, sliderIncrement);
    if(skewFromMidpoint) setSkewFactorFromMidPoint(sliderDefault);
    else setSkewFactor (1., false);
    setValue(sliderDefault, juce::dontSendNotification);
}

void BKSubSlider::setSkewFromMidpoint(bool sfm)
{
    skewFromMidpoint = sfm;

    if(skewFromMidpoint) setSkewFactorFromMidPoint(sliderDefault);
    else setSkewFactor (1., false);
}

double BKSubSlider::getValueFromText(const juce::String & text )
{
    double newval = text.getDoubleValue();

    if(newval > getMaximum()) {
        sliderMax = newval;
        setRange(getMinimum(), newval, sliderIncrement);
    }

    if(newval < getMinimum()) {
        sliderMin = newval;
        setRange(newval, getMaximum(), sliderIncrement);
    }

    return newval;
}



// ******************************************************************************************************************** //
// **************************************************  BKMultiSlider ************************************************** //
// ******************************************************************************************************************** //

BKMultiSlider::BKMultiSlider(const juce::ValueTree& stateDefault) : StateModulatedComponent(stateDefault)
{
    // initialize stuff
    passiveSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colour::greyLevel (0.8f).contrasting().withAlpha (0.13f));
    highlightedSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::red.withSaturation(1.));
    activeSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.75));
    displaySliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::red.withMultipliedAlpha(0.5));
    lastHighlightedSlider = 0;

    sliderMin = sliderMinDefault = -1.;
    sliderMax = sliderMaxDefault = 1.;
    sliderIncrement = 0.01;
    sliderDefault = 0.;
    numDefaultSliders = 12;
    allowSubSliders = false;
    subSliderName = "add subslider";
    sliderWidth = 20;
    sliderHeight = 60;
    displaySliderWidth = 80;
    clickedHeight = 0.;
    subsliderStyle = juce::Slider::LinearBarVertical;

    // the bigInvisibleSlider sits on top of the individual sliders
    // it's used to set the values of the slider that the mouse is nearest
    bigInvisibleSlider = std::make_unique<BKSubSlider>(juce::Slider::LinearBarVertical,
        sliderMin,
        sliderMax,
        sliderDefault,
        sliderIncrement,
        20,
        sliderHeight);

    // this displays the current value of whatever slider the mouse is nearest
    // is placed to the left of the array of sliders
    displaySlider = std::make_unique<BKSubSlider>(juce::Slider::LinearBarVertical,
        sliderMin,
        sliderMax,
        sliderDefault,
        sliderIncrement,
        displaySliderWidth,
        sliderHeight);

    bigInvisibleSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0,0);
    bigInvisibleSlider->setAlpha(0.0);
    bigInvisibleSlider->addMouseListener(this, true);
    bigInvisibleSlider->setName("BIG");
    bigInvisibleSlider->addListener(this);
    bigInvisibleSlider->setLookAndFeel(&activeSliderLookAndFeel);
    addAndMakeVisible(*bigInvisibleSlider);

    displaySlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, true, 0,0);
    displaySlider->addMouseListener(this, true);
    displaySlider->setName("DISPLAY");
    displaySlider->addListener(this);
    displaySlider->setInterceptsMouseClicks(false, false);
    displaySlider->setLookAndFeel(&displaySliderLookAndFeel);
    addAndMakeVisible(*displaySlider);

    showName.setInterceptsMouseClicks(false, true);
    addAndMakeVisible(showName);

    // for editing the slider values by text
    editValsTextField = std::make_unique<juce::TextEditor>();
    editValsTextField->setMultiLine(true);
    editValsTextField->setName("PARAMTXTEDIT");
    editValsTextField->setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey);
    editValsTextField->addListener(this);
    addAndMakeVisible(*editValsTextField);

    // for rotating the slider values
    rotateButton = std::make_unique<juce::ImageButton>("ROTATE");
    rotateButton->setImages(false, true, true,
        juce::ImageCache::getFromMemory(BinaryData::rotate_arrow_png, BinaryData::rotate_arrow_pngSize), 0.8f, juce::Colours::transparentBlack,
        juce::Image(), 1.0f, juce::Colours::transparentBlack,
        juce::Image(), 0.6, juce::Colours::transparentBlack);
    rotateButton->setVisible(true);
    rotateButton->setTooltip("Rotate values");
    rotateButton->addListener(this);
    addAndMakeVisible(*rotateButton);

    //create the default sliders, with one active
    initializeSliderVals(numDefaultSliders);

    // draw them! ready to go....
    drawSliders(juce::dontSendNotification);

    addAndMakeVisible(sliderBorder);

}

BKMultiSlider::~BKMultiSlider() {}

// initialize the slider; it should have no less than numDefaultSliders, all set to sliderDefault value
void BKMultiSlider::initializeSliderVals(int howmany)
{
    if (howmany < numDefaultSliders) howmany = numDefaultSliders;

    // slider values
    allSliderVals.clear();
    allSliderVals.ensureStorageAllocated(howmany);
    for (int i = 0; i < howmany; i++)
        allSliderVals.insert(i, {sliderDefault});

    // slider states (whether they are active or not (passive))
    whichSlidersActive.clearQuick();
    whichSlidersActive.ensureStorageAllocated(howmany);
    whichSlidersActive.set(0, true);
    for (int i = 1; i < howmany; i++)
        whichSlidersActive.set(i, false);

}


void BKMultiSlider::updateSliderVal(int which, int whichSub, float val)
{
    if (which < allSliderVals.size() && which >= 0)
    {
        juce::Array<float> stemp = allSliderVals.getUnchecked(which);
        stemp.set(whichSub, val);
        allSliderVals.set(which, stemp);
        whichSlidersActive.set(which, true);
    }
}


void BKMultiSlider::printSliderVals()
{
    for (int i = 0; i < allSliderVals.size(); i++)
    {
        for (int j = 0; j < allSliderVals[i].size(); j++)
        {
            DBG("slider # " + juce::String(i) + " subslider # " + juce::String(j) +  " = " + juce::String(allSliderVals[i][j]) + " isActive = " + juce::String((int)whichSlidersActive[i]));
        }
    }
}


inline void BKMultiSlider::dismissTextEditor(bool setValue)
{
    if (setValue)   textEditorReturnKeyPressed(*editValsTextField);
    else            textEditorEscapeKeyPressed(*editValsTextField);
}


void BKMultiSlider::drawSliders(juce::NotificationType newnotify)
{
    sliders.clearQuick(true);

    // rebuild display slider array
    for(int i = 0; i < allSliderVals.size(); i++)
    {
        if(i >= sliders.size()) addSlider(-1, false, newnotify);

        for(int j = 0; j < allSliderVals[i].size(); j++)
        {

            if(j >= sliders[i]->size()) addSubSlider(i, false, newnotify);

            BKSubSlider* refSlider = sliders[i]->operator[](j);
            if(refSlider != nullptr)
            {
                if(refSlider->getMaximum() < allSliderVals[i][j]) refSlider->setRange(sliderMin, allSliderVals[i][j], sliderIncrement);
                if(refSlider->getMinimum() > allSliderVals[i][j]) refSlider->setRange(allSliderVals[i][j], sliderMax, sliderIncrement);

                if (whichSlidersActive[i]) {
                    refSlider->setValue(allSliderVals[i][j], newnotify);
                    refSlider->setLookAndFeel(&activeSliderLookAndFeel);
                }
                else {
                    refSlider->setLookAndFeel(&passiveSliderLookAndFeel);
                }

                refSlider->setSkewFromMidpoint(skewFromMidpoint);

            }
        }
    }

    resetRanges();
    resized();
    displaySlider->setValue(allSliderVals[0][0]);
}


void  BKMultiSlider::setTo(juce::Array<juce::Array<float>> newvals, juce::Array<bool> newactives, juce::NotificationType newnotify)
{
    initializeSliderVals(newvals.size());

    for (int i = 0; i < newvals.size() && i < newactives.size(); i++)
    {
        allSliderVals.set(i, newvals[i]);

        if(newactives[i]) whichSlidersActive.set(i, true);
        else whichSlidersActive.set(i, false);
    }

    drawSliders(newnotify);
}

// when the client sends an array of only the active slider values, this will construct the complete array
// of slider values, including inactive sliders, and then call setTo
void BKMultiSlider::setToOnlyActive(juce::Array<juce::Array<float>> newActiveVals, juce::Array<bool> newactives, juce::NotificationType newnotify)
{
    juce::Array<juce::Array<float>> allvals;
    int inc = 0;

    for (int i = 0; i < newactives.size() && inc < newActiveVals.size(); i++)
    {
        if (newactives[i]) allvals.set(i, newActiveVals[inc++]);
        else allvals.set(i, {0});
    }

    setTo(allvals, newactives, newnotify);
}


void BKMultiSlider::setToOnlyActive(juce::Array<float> newActiveVals, juce::Array<bool> newactives, juce::NotificationType newnotify)
{
    juce::Array<juce::Array<float>> allvals;
    for (int i = 0; i < newActiveVals.size(); i++)
    {
        allvals.set(i, {newActiveVals.getUnchecked(i)});
    }

    setToOnlyActive(allvals, newactives, newnotify);
}

void BKMultiSlider::setMinMaxDefaultInc(std::vector<float> newvals)
{
    sliderMin = sliderMinDefault = newvals[0];
    sliderMax = sliderMaxDefault = newvals[1];
    sliderDefault = newvals[2];
    sliderIncrement = newvals[3];

    for(int i = 0; i < sliders.size(); i++)
    {
        for(int j = 0; j < sliders[i]->size(); j++)
        {
            BKSubSlider* refSlider = sliders[i]->operator[](j);
            if(refSlider != nullptr)
            {
                refSlider->setMinMaxDefaultInc(newvals);
                refSlider->setSkewFromMidpoint(skewFromMidpoint);
            }
        }
    }

    displaySlider->setMinMaxDefaultInc(newvals);
    bigInvisibleSlider->setMinMaxDefaultInc(newvals);
    displaySlider->setSkewFromMidpoint(skewFromMidpoint);
    bigInvisibleSlider->setSkewFromMidpoint(skewFromMidpoint);
}

std::vector<float> BKMultiSlider::getMinMaxDefaultInc()
{
    std::vector<float> rangeVals = {static_cast<float>(sliderMin), static_cast<float>(sliderMax), static_cast<float>(sliderDefault), static_cast<float>(sliderIncrement)};
    return rangeVals;
}

void BKMultiSlider::setSkewFromMidpoint(bool sfm)
{
    skewFromMidpoint = sfm;

    for(int i = 0; i < sliders.size(); i++)
    {
        for(int j = 0; j < sliders[i]->size(); j++)
        {
            BKSubSlider* refSlider = sliders[i]->operator[](j);
            if(refSlider != nullptr)
            {
                refSlider->setSkewFromMidpoint(skewFromMidpoint);
            }
        }
    }

    displaySlider->setSkewFromMidpoint(skewFromMidpoint);
    bigInvisibleSlider->setSkewFromMidpoint(skewFromMidpoint);
}


void BKMultiSlider::addSlider(int where, bool active, juce::NotificationType newnotify)
{
    BKSubSlider* newslider;

    newslider      = new BKSubSlider(subsliderStyle,
        sliderMin,
        sliderMax,
        sliderDefault,
        sliderIncrement,
        sliderWidth,
        sliderHeight);

    newslider->setRange(sliderMin, sliderMax, sliderIncrement);
    newslider->setValue(sliderDefault, juce::dontSendNotification);
    newslider->addListener(this);

    if(where < 0)
    {
        sliders.add(new juce::OwnedArray<BKSubSlider>);
        sliders.getLast()->add(newslider);
    }
    else
    {
        sliders.insert(where, new juce::OwnedArray<BKSubSlider>);
        sliders[where]->add(newslider);
    }

    addAndMakeVisible(newslider);

    if(active)
        newslider->setLookAndFeel(&activeSliderLookAndFeel);
    else
        newslider->setLookAndFeel(&passiveSliderLookAndFeel);

    if(newnotify == juce::sendNotification)
    {
        listeners.call(&BKMultiSlider::Listener::multiSliderAllValuesChanged,
            getName(),
            getAllActiveValues(),
            whichSlidersActive);
    }

}

void  BKMultiSlider::addActiveSubSlider(int which, juce::NotificationType newnotify)
{

    if (which < allSliderVals.size() && which >= 0)
    {
        // get current array of values for this slider
        juce::Array<float> stemp = allSliderVals.getUnchecked(which);

        // find slot for next one
        int whichSub = stemp.size();

        // calculate the value to set it at, based on where clicked
        float newval = bigInvisibleSlider->proportionOfLengthToValue( 1. - (clickedHeight / this->getHeight()));

        // update the state arrays
        stemp.set(whichSub, newval);
        allSliderVals.set(which, stemp);
        whichSlidersActive.set(which, true);

        // make the subSlider
        addSubSlider(which, true, newnotify);
    }

}

void BKMultiSlider::addSubSlider(int where, bool active, juce::NotificationType newnotify)
{
    BKSubSlider* newslider;

    newslider = new BKSubSlider(subsliderStyle,
        sliderMin,
        sliderMax,
        sliderDefault,
        sliderIncrement,
        sliderWidth,
        sliderHeight);

    newslider->setRange(sliderMin, sliderMax, sliderIncrement);
    if(this->getHeight() > 0.)
        newslider->setValue(newslider->proportionOfLengthToValue( 1. - (clickedHeight / this->getHeight())), juce::dontSendNotification);
    newslider->addListener(this);

    juce::OwnedArray<BKSubSlider> *newsliderArray = sliders[where];
    newsliderArray->add(newslider);
    sliders.set(where, newsliderArray);

    addAndMakeVisible(newslider);

    if(active)
        newslider->setLookAndFeel(&activeSliderLookAndFeel);
    else
        newslider->setLookAndFeel(&passiveSliderLookAndFeel);

    if(newnotify == juce::sendNotification)
    {
        listeners.call(&BKMultiSlider::Listener::multiSliderAllValuesChanged,
            getName(),
            getAllActiveValues(),
            whichSlidersActive);
    }
}

void BKMultiSlider::deactivateSlider(int where, juce::NotificationType notify)
{
    if (where > 0 && where < whichSlidersActive.size())
    {
        whichSlidersActive.set(where, false);
        drawSliders(notify);
    }
}


void BKMultiSlider::deactivateAll(juce::NotificationType notify)
{
    for(int i = 0; i < sliders.size(); i++ )
    {
        deactivateSlider(i, notify);
    }
}


void BKMultiSlider::deactivateAllAfter(int where, juce::NotificationType notify)
{
    for(int i = where+1; i < sliders.size(); i++ )
    {
        deactivateSlider(i, notify);
    }
}


void BKMultiSlider::deactivateAllBefore(int where, juce::NotificationType notify)
{
    if (where > sliders.size()) where = sliders.size();
    for(int i = 0; i < where; i++ )
    {
        deactivateSlider(i, notify);
    }
}


// mouseDrag: updates the values of all sliders that the user drags over
void BKMultiSlider::mouseDrag(const juce::MouseEvent& e)
{
    if(e.eventComponent == bigInvisibleSlider.get())
    {
        int which = whichSlider(e);
        if(e.mods.isShiftDown()) updateSliderVal(which, currentSubSlider, round(currentInvisibleSliderValue));
        else updateSliderVal(which, currentSubSlider, currentInvisibleSliderValue);

        if(which >= 0) {

            BKSubSlider* currentSlider = sliders[which]->operator[](currentSubSlider);
            if (currentSlider != nullptr)
            {
                if(e.mods.isShiftDown())
                {
                    currentSlider->setValue(round(currentInvisibleSliderValue));
                    displaySlider->setValue(round(currentInvisibleSliderValue));
                }
                else
                {
                    currentSlider->setValue(currentInvisibleSliderValue);
                    displaySlider->setValue(currentInvisibleSliderValue);
                }

                if(whichSlidersActive[which]){
                    currentSlider->setLookAndFeel(&activeSliderLookAndFeel);
                    listeners.call(&BKMultiSlider::Listener::multiSliderAllValuesChanged,
                        getName(),
                        getAllActiveValues(),
                        whichSlidersActive);
                }
                else
                {
                    listeners.call(&BKMultiSlider::Listener::multiSliderValueChanged,
                        getName(),
                        whichActiveSlider(which),
                        getOneSliderBank(which));
                }
            }
        }
    }
}


// mouseMove: updates the displaySlider to show the value of the slider that the pointer is nearest
void BKMultiSlider::mouseMove(const juce::MouseEvent& e)
{
    if(e.eventComponent == bigInvisibleSlider.get())
    {
        int which = whichSlider(e);
        int whichSub = whichSubSlider(which, e);

        if (which < allSliderVals.size() && which >= 0)
        {
            if (whichSub < allSliderVals[which].size() && whichSub >= 0)
            {
                if (whichSlidersActive[which]) displaySlider->setValue(allSliderVals[which][whichSub]);
            }
        }
    }
}


// mouseDoubleClick: opens text window for editing slider values directly
void BKMultiSlider::mouseDoubleClick (const juce::MouseEvent &e)
{
    int which = whichSlider(e);
    int whichSave = which;

    //account for subsliders
    which += whichSubSlider(which, e);
    for (int i=0; i<whichSave; i++)
    {
        if(sliders[i]->size() > 0)
        {
            which += (sliders[i]->size() - 1);
        }
    }

    //highlight number for current slider
    juce::StringArray tokens;
    tokens.addTokens(arrayActiveFloatArrayToString(allSliderVals, whichSlidersActive), false); //arrayFloatArrayToString

    int startPoint = 0;
    int endPoint;

    //need to skip brackets
    int numBrackets = 0;
    for(int i=0; i<=which + numBrackets; i++)
    {
        if(tokens[i] == "[" || tokens[i] == "]") numBrackets++;
    }

    for(int i=0; i < which + numBrackets; i++) {
        if(tokens[i] == "[") startPoint += 1;
        else if(tokens[i] == "]") startPoint += 2;
        else startPoint += tokens[i].length() + 1;
    }
    endPoint = startPoint + tokens[which + numBrackets].length();

    editValsTextField->setVisible(true);
    editValsTextField->toFront(true);
    editValsTextField->setText(arrayActiveFloatArrayToString(allSliderVals, whichSlidersActive));
    editValsTextField->setWantsKeyboardFocus(true);
    editValsTextField->grabKeyboardFocus();

    juce::Range<int> highlightRange(startPoint, endPoint);
    editValsTextField->setHighlightedRegion(highlightRange);

    focusLostByEscapeKey = false;
}


// mouseDown: determines which subslider the mouseDown is nearest
//            checks to see if ctrl is down, for contextual menu
void BKMultiSlider::mouseDown (const juce::MouseEvent &event)
{
    if (event.mouseWasClicked())
    {
        currentSubSlider = whichSubSlider(whichSlider(event));
        clickedHeight = event.y;

        if(event.mods.isCtrlDown())
        {
            showModifyPopupMenu(whichSlider(event));
        }
    }
}


// mouseUp: on shift-click, slider will be set to default value
void BKMultiSlider::mouseUp (const juce::MouseEvent &event)
{
    if(!event.mouseWasClicked())
    {
        if(event.mods.isShiftDown())
        {
            int which = whichSlider(event);

            if(which >= 0) {

                if (sliders[which]->size() == 0) addSubSlider(which, false, juce::sendNotification);

                updateSliderVal(which, currentSubSlider, sliderDefault);

                BKSubSlider* currentSlider = sliders[which]->operator[](0);
                if (currentSlider != nullptr)
                {
                    currentSlider->setValue(sliderDefault); //again, need to identify which subslider to get
                }

                displaySlider->setValue(sliderDefault);

                listeners.call(&BKMultiSlider::Listener::multiSliderValueChanged,
                    getName(),
                    whichActiveSlider(which),
                    getOneSliderBank(which));
            }
        }
    }
}


int BKMultiSlider::whichSlider (const juce::MouseEvent &e)
{
    int x = e.x;

    BKSubSlider* refSlider = sliders[0]->operator[](0);
    if (refSlider != nullptr)
    {
        int which = (x / refSlider->getWidth());
        if (which >= 0 && which < sliders.size()) return which;
    }

    return -1;
}


int BKMultiSlider::whichSubSlider (int which)
{
    if(which < 0) return 0;

    int whichSub = 0;
    float refDistance;

    BKSubSlider* refSlider = sliders[which]->operator[](0);
    if(refSlider != nullptr)
    {
        refDistance = fabs(refSlider->getValue() - currentInvisibleSliderValue);
    }

    for(int i = 0; i < sliders[which]->size(); i++)
    {
        BKSubSlider* currentSlider = sliders[which]->operator[](i);
        if(currentSlider != nullptr) {
            float tempDistance = fabs(currentSlider->getValue() - currentInvisibleSliderValue);
            if(tempDistance < refDistance)
            {
                whichSub = i;
                refDistance = tempDistance;
            }
        }
    }

    return whichSub;
}


int BKMultiSlider::whichSubSlider (int which, const juce::MouseEvent &e)
{
    if(which < 0) return 0;

    int whichSub = 0;
    float refDistance;

    BKSubSlider* refSlider = sliders[which]->operator[](0);
    if(refSlider != nullptr)
    {
        refDistance = fabs(refSlider->getPositionOfValue(refSlider->getValue()) - e.y);
    }

    for(int i = 0; i < sliders[which]->size(); i++)
    {
        BKSubSlider* currentSlider = sliders[which]->operator[](i);
        if(currentSlider != nullptr) {
            float tempDistance = fabs(currentSlider->getPositionOfValue(currentSlider->getValue()) - e.y);
            if(tempDistance < refDistance)
            {
                whichSub = i;
                refDistance = tempDistance;
            }
        }
    }

    return whichSub;
}

// given a slider location 'which', how many active sliders
// are there up to and including 'which'
int BKMultiSlider::whichActiveSlider (int which)
{
    int counter = 0;
    if(which > whichSlidersActive.size()) which = whichSlidersActive.size();

    for(int i = 0; i < which; i++)
    {
        if(whichSlidersActive[which]) counter++;
    }

    return counter;
}


void BKMultiSlider::resetRanges()
{
    double sliderMinTemp = sliderMinDefault;
    double sliderMaxTemp = sliderMaxDefault;

    for (int i = 0; i < sliders.size(); i++)
    {
        for (int j = 0; j < sliders[i]->size(); j++)
        {
            BKSubSlider* currentSlider = sliders[i]->operator[](j);
            if (currentSlider != nullptr)
            {
                if (currentSlider->getValue() > sliderMaxTemp) sliderMaxTemp = currentSlider->getValue();
                if (currentSlider->getValue() < sliderMinTemp) sliderMinTemp = currentSlider->getValue();
            }
        }
    }

    if ((sliderMax != sliderMaxTemp) || sliderMin != sliderMinTemp)
    {
        sliderMax = sliderMaxTemp;
        sliderMin = sliderMinTemp;

        for (int i = 0; i < sliders.size(); i++)
        {
            for (int j = 0; j < sliders[i]->size(); j++)
            {
                BKSubSlider* currentSlider = sliders[i]->operator[](j);
                if (currentSlider != nullptr)
                {
                    currentSlider->setRange(sliderMin, sliderMax, sliderIncrement);
                }
            }
        }

        bigInvisibleSlider->setRange(sliderMin, sliderMax, sliderIncrement);
        displaySlider->setRange(sliderMin, sliderMax, sliderIncrement);
    }
}


void BKMultiSlider::resized()
{

    juce::Rectangle<float> area (getLocalBounds().toFloat());
    juce::Rectangle<float> bounds = area;

    sliderBorder.setBounds(getLocalBounds());
    area.removeFromTop(5);
    area.reduce(10, 10);

    juce::Rectangle<float> rotateButtonBounds (area.getBottomLeft(), area.getBottomLeft().translated(displaySliderWidth*0.2, -displaySliderWidth*0.2));
    rotateButton->setBounds(rotateButtonBounds.toNearestInt());

    displaySlider->setBounds(area.removeFromLeft(displaySliderWidth).toNearestInt());
    editValsTextField->setBounds(area.toNearestInt());
    editValsTextField->setVisible(false);

    juce::Rectangle<float> nameSlab (area);
    nameSlab.removeFromTop(gYSpacing / 2.).removeFromRight(gXSpacing);
    //showName.setBounds(nameSlab.toNearestInt());
    showName.setJustificationType(juce::Justification::topRight);

    bigInvisibleSlider->setBounds(area.toNearestInt());

    sliderWidth = (float)area.getWidth() / sliders.size();

    for (int i = 0; i < sliders.size(); i++)
    {
        juce::Rectangle<float> sliderArea (area.removeFromLeft(sliderWidth));
        for(int j = 0; j < sliders[i]->size(); j++)
        {
            BKSubSlider* currentSlider = sliders[i]->operator[](j);
            if(currentSlider != nullptr)
            {
                currentSlider->setBounds(sliderArea.toNearestInt());
            }
        }
    }

    bigInvisibleSlider->toFront(false);
}


void BKMultiSlider::sliderValueChanged (juce::Slider *slider)
{
    if (slider->getName() == "BIG")
    {
        currentInvisibleSliderValue = slider->getValue();
    }
}


void BKMultiSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        editValsTextField->setVisible(false);
        editValsTextField->toBack();

        juce::String ins = textEditor.getText();
        setTo(stringToArrayFloatArray(ins), slashToFalse(ins), juce::sendNotification);

        listeners.call(&BKMultiSlider::Listener::multiSliderAllValuesChanged,
            getName(),
            getAllActiveValues(),
            whichSlidersActive);
    }
}


void BKMultiSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        focusLostByEscapeKey = true;
        editValsTextField->setVisible(false);
        editValsTextField->toBack();
        unfocusAllComponents();
    }
}


void BKMultiSlider::textEditorTextChanged(juce::TextEditor& tf)
{
}


void BKMultiSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        editValsTextField->setVisible(false);
        editValsTextField->toBack();

        if(!focusLostByEscapeKey)
        {
            juce::String ins = textEditor.getText();
            setTo(stringToArrayFloatArray(ins), slashToFalse(ins), juce::sendNotification);

            listeners.call(&BKMultiSlider::Listener::multiSliderAllValuesChanged,
                getName(),
                getAllActiveValues(),
                whichSlidersActive);
        }
    }
}


void BKMultiSlider::buttonClicked(juce::Button* button)
{
    if (button->getName() == "ROTATE")
    {
        juce::Array<juce::Array<float>> values = getAllActiveValues();
        juce::Array<float> swap = values.getLast();
        values.insert(0, swap);
        values.removeLast();
        setToOnlyActive(values, whichSlidersActive, juce::sendNotification);
    }
}


juce::Array<juce::Array<float>> BKMultiSlider::getAllActiveValues()
{
    juce::Array<juce::Array<float>> allvals;

    for (int i = 0; (i < whichSlidersActive.size()) && (i < allSliderVals.size()); i++)
    {
        if (whichSlidersActive[i]) allvals.add(allSliderVals.getUnchecked(i));
    }

    return allvals;
}


juce::Array<float> BKMultiSlider::getOneSliderBank(int which)
{
    if (which < allSliderVals.size() && which >= 0)
    {
        return allSliderVals[which];
    }

    return {0};
}


void BKMultiSlider::showModifyPopupMenu(int which)
{
    juce::PopupMenu m;
    m.addItem (1, juce::translate ("deactivate slider"), true, false);
    m.addItem (2, juce::translate ("deactivate all after this"), true, false);
    m.addItem (3, juce::translate ("deactivate all before this"), true, false);
    if(allowSubSliders) m.addItem (4, translate (subSliderName), true, false);
    m.addSeparator();

    m.showMenuAsync (juce::PopupMenu::Options(),
        juce::ModalCallbackFunction::forComponent (sliderModifyMenuCallback, this, which));
}

/**
 * todo: need for OpenGL wrapper to know that this is from a mouseInteraction
 *      none of these are registering for real unless something is changed after this, before closing/opening the prepview
 */
void BKMultiSlider::sliderModifyMenuCallback (const int result, BKMultiSlider* ms, int which)
{
    if (ms == nullptr)
    {
        juce::PopupMenu::dismissAllActiveMenus();
        return;
    }

    switch (result)
    {
        case 1: ms->deactivateSlider(which, juce::sendNotification);
            break;

        case 2: ms->deactivateAllAfter(which, juce::sendNotification);
            break;

        case 3: ms->deactivateAllBefore(which, juce::sendNotification);
            break;

        case 4: ms->addActiveSubSlider(which, juce::sendNotification);
            //ms->addSubSlider(which, true, sendNotification);
            ms->resized();
            break;

        default:  break;
    }
}


// find activeSliderNum, highlight it as current, dehighlight all the others...
void BKMultiSlider::setCurrentSlider(int activeSliderNum)
{
    // dim all inactive sliders
    for (int i = 1; i < allSliderVals.size(); i++)
    {
        for(int j = 0; j < sliders[i]->size(); j++)
        {
            if (!whichSlidersActive[i]) sliders[i]->operator[](j)->setLookAndFeel(&passiveSliderLookAndFeel);
        }
    }

    if (lastHighlightedSlider < sliders.size())
    {
        deHighlight(lastHighlightedSlider);
    }

    int sliderNum = getActiveSlider(activeSliderNum);
    highlight(sliderNum);
    displaySlider->setValue(sliders[sliderNum]->operator[](0)->getValue());

    lastHighlightedSlider = sliderNum;
}


int BKMultiSlider::getActiveSlider(int sliderNum)
{
    int sliderCount = 0;

    for (int i = 0; i < whichSlidersActive.size(); i++)
    {
        if (whichSlidersActive[i]) {
            if (sliderCount == sliderNum) return i;
            sliderCount++;
        }
    }
    return 0;
}


void BKMultiSlider::highlight(int activeSliderNum)
{
    //need to count through depth, but for now just the first one...
    for(int i = 0; i < sliders[activeSliderNum]->size(); i++)
    {
        sliders[activeSliderNum]->operator[](i)->setLookAndFeel(&highlightedSliderLookAndFeel);
    }
}


void BKMultiSlider::deHighlight(int sliderNum)
{
    for(int i = 0; i < sliders[sliderNum]->size(); i++)
    {
        sliders[sliderNum]->operator[](i)->setLookAndFeel(&activeSliderLookAndFeel);
    }

}

void BKMultiSlider::deHighlightCurrentSlider()
{
    for(int i = 0; i < sliders[lastHighlightedSlider]->size(); i++)
    {
        sliders[lastHighlightedSlider]->operator[](i)->setLookAndFeel(&activeSliderLookAndFeel);
    }

}

// ******************************************************************************************************************** //
// *******************************************  BKWaveDistanceUndertowSlider ****************************************** //
// ******************************************************************************************************************** //


BKWaveDistanceUndertowSlider::BKWaveDistanceUndertowSlider (juce::String name, double min, double max,
    double defmin, double defmax, double increment, const juce::ValueTree& stateDefault):
        StateModulatedComponent(stateDefault), sliderName(name), sliderMin(min), sliderMax(max),
        sliderDefaultMin(defmin), sliderDefaultMax(defmax), sliderIncrement(increment)
{
    maxSliders = 100;
    sliderMin = 0;
    sliderMax = 20000;
    sliderIncrement = 1;

    float skewFactor = 0.7;

    sliderBorder.setName("waveDistanceUndertowSlider");
    //sliderBorder.setText("Wave Distance <=> Undertow");
    sliderBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sliderBorder);

    sampleImageComponent.setImage(juce::ImageCache::getFromMemory(BinaryData::samplePic_png, BinaryData::samplePic_pngSize));
    sampleImageComponent.setImagePlacement(juce::RectanglePlacement(juce::RectanglePlacement::stretchToFit));
    sampleImageComponent.setTooltip("Provides real-time visualization of each independent Nostalgic wave");
    addAndMakeVisible(sampleImageComponent);

    // wavedistanceSlider = std::make_unique<juce::Slider>();
    wavedistanceSlider.addMouseListener(this, true);
    wavedistanceSlider.setRange(sliderMin, sliderMax, sliderIncrement);
    wavedistanceSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    wavedistanceSlider.setLookAndFeel(&displaySliderLookAndFeel);
    wavedistanceSlider.setTextBoxIsEditable(false);
    wavedistanceSlider.setColour(juce::Slider::trackColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.5));
    wavedistanceSlider.addListener(this);
    wavedistanceSlider.setSkewFactor(skewFactor);
    addAndMakeVisible(wavedistanceSlider);

    // undertowSlider = std::make_unique<juce::Slider>();
    undertowSlider.addMouseListener(this, true);
    undertowSlider.setRange(sliderMin, sliderMax, sliderIncrement);
    undertowSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    undertowSlider.setLookAndFeel(&displaySliderLookAndFeel);
    undertowSlider.setTextBoxIsEditable(false);
    undertowSlider.setColour(juce::Slider::trackColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.5));
    undertowSlider.addListener(this);
    undertowSlider.setSkewFactor(skewFactor);
    addAndMakeVisible(undertowSlider);

    displaySliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.5));
    for(int i=0; i<maxSliders; i++)
    {
        displaySliders.insert(0, new juce::Slider());
        juce::Slider* newSlider = displaySliders.getUnchecked(0);

        newSlider->setRange(sliderMin, sliderMax, sliderIncrement);
        newSlider->setLookAndFeel(&displaySliderLookAndFeel);
        newSlider->setSliderStyle(BKSubSlider::SliderStyle::LinearBar);
        newSlider->setColour(juce::Slider::trackColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.5));
        newSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        newSlider->setInterceptsMouseClicks(false, false);
        newSlider->setSkewFactor(skewFactor);
        addAndMakeVisible(newSlider);
    }

    undertowValueTF.setName("ut");
    undertowValueTF.addListener(this);
    addChildComponent(undertowValueTF);

    wavedistanceValueTF.setName("wd");
    wavedistanceValueTF.addListener(this);
    addChildComponent(wavedistanceValueTF);

    wavedistanceName.setText("wave distance (ms)", juce::dontSendNotification);
    wavedistanceName.setJustificationType(juce::Justification::topRight);
    addAndMakeVisible(wavedistanceName);

    undertowName.setText("undertow (ms)", juce::dontSendNotification);
    undertowName.setJustificationType(juce::Justification::bottomRight);
    addAndMakeVisible(undertowName);
}


void BKWaveDistanceUndertowSlider::sliderValueChanged (juce::Slider *slider)
{
    if(slider == &wavedistanceSlider)
    {
        setWaveDistance(wavedistanceSlider.getValue(), juce::dontSendNotification);
    }
}
void BKWaveDistanceUndertowSlider::setDim(float alphaVal)
{
    wavedistanceName.setAlpha(alphaVal);
    undertowName.setAlpha(alphaVal);
    wavedistanceSlider.setAlpha(alphaVal);
    undertowSlider.setAlpha(alphaVal);

    for(int i=0; i<maxSliders; i++)
    {
        juce::Slider* newSlider = displaySliders.getUnchecked(i);
        newSlider->setAlpha(0.);
    }

}

void BKWaveDistanceUndertowSlider::setBright()
{
    wavedistanceName.setAlpha(1.);
    undertowName.setAlpha(1.);
    wavedistanceSlider.setAlpha(1.);
    undertowSlider.setAlpha(1.);
}


void BKWaveDistanceUndertowSlider::updateSliderPositions(juce::Array<int> newpositions)
{

    if(newpositions.size() > maxSliders) newpositions.resize(maxSliders);

    for(int i=0; i<newpositions.size(); i++)
    {
        displaySliders.getUnchecked(i)->setValue(newpositions.getUnchecked(i) - wavedistanceSlider.getValue());
        displaySliders.getUnchecked(i)->setVisible(true);
    }

    for(int i=newpositions.size(); i<displaySliders.size(); i++)
    {
        displaySliders.getUnchecked(i)->setVisible(false);
    }
}


void BKWaveDistanceUndertowSlider::resized()
{
    juce::Rectangle<int> area (getLocalBounds());
    sliderBorder.setBounds(area);
    area.removeFromTop(10);
    area.removeFromBottom(2);
    area.reduce(8, 2);

    wavedistanceSlider.setBounds(area.removeFromTop(getHeight() * 0.1));
    undertowSlider.setBounds(area.removeFromBottom(getHeight() * 0.1));

    wavedistanceValueTF.setBounds(wavedistanceSlider.getBounds());
    undertowValueTF.setBounds(undertowSlider.getBounds());

    undertowName.setBounds(
                           undertowSlider.getRight() - undertowSlider.getWidth() / 4,
                           undertowSlider.getY() - undertowSlider.getHeight(),
                           undertowSlider.getWidth() / 4,
                           undertowSlider.getHeight());
    wavedistanceName.setBounds(
                               wavedistanceSlider.getRight() - wavedistanceSlider.getWidth() / 4,
                               wavedistanceSlider.getBottom(),
                               wavedistanceSlider.getWidth() / 4,
                               wavedistanceSlider.getHeight());

    sampleImageComponent.setBounds(area);

    for(int i=0; i<maxSliders; i++)
    {
        juce::Slider* newSlider = displaySliders.getUnchecked(i);
        newSlider->setBounds(area);
    }

    setWaveDistance(wavedistanceSlider.getValue(), juce::NotificationType::dontSendNotification);

}

void BKWaveDistanceUndertowSlider::mouseDoubleClick(const juce::MouseEvent& e)
{
    Component* c = e.eventComponent->getParentComponent();

    if (c == &wavedistanceSlider)
    {
        wavedistanceValueTF.setVisible(true);
        wavedistanceValueTF.grabKeyboardFocus();
    }
    else if (c == &undertowSlider)
    {
        undertowValueTF.setVisible(true);
        undertowValueTF.grabKeyboardFocus();
    }

}

void BKWaveDistanceUndertowSlider::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    double newval = editor.getText().getDoubleValue();

    //DBG("nostalgic wavedistance/undertow slider return key pressed");

    if (editor.getName() == "ut")
    {
        undertowSlider.setValue(newval, juce::sendNotification);
    }
    else if (editor.getName() == "wd")
    {
        wavedistanceSlider.setValue(newval, juce::sendNotification);
    }

    wavedistanceValueTF.setVisible(false);
    undertowValueTF.setVisible(false);

    setWaveDistance(wavedistanceSlider.getValue(), juce::dontSendNotification);

    listeners.call(&BKWaveDistanceUndertowSlider::Listener::BKWaveDistanceUndertowSliderValueChanged,
                   "nSlider",
                   wavedistanceSlider.getValue(),
                   undertowSlider.getValue());

    unfocusAllComponents();
}

void BKWaveDistanceUndertowSlider::textEditorTextChanged(juce::TextEditor& textEditor)
{
    focusLostByEscapeKey = false;
}

void BKWaveDistanceUndertowSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
{
    //DBG("Nostalgic textEditorEscapeKeyPressed");
    focusLostByEscapeKey = true;
    unfocusAllComponents();
}

void BKWaveDistanceUndertowSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
}

void BKWaveDistanceUndertowSlider::sliderDragEnded(juce::Slider *slider)
{
    if(slider == &wavedistanceSlider)
    {
        setWaveDistance(wavedistanceSlider.getValue(), juce::dontSendNotification);
    }

    listeners.call(&BKWaveDistanceUndertowSlider::Listener::BKWaveDistanceUndertowSliderValueChanged,
                   "nSlider",
                   wavedistanceSlider.getValue(),
                   undertowSlider.getValue());
}

void BKWaveDistanceUndertowSlider::setWaveDistance(int newwavedist, juce::NotificationType notify)
{
    wavedistanceSlider.setValue(newwavedist, notify);

    int xpos = wavedistanceSlider.getPositionOfValue(wavedistanceSlider.getValue()) + 6;
    undertowSlider.setBounds(xpos, undertowSlider.getY(), getWidth() - xpos, undertowSlider.getHeight());
    double max = sliderMax - wavedistanceSlider.getValue();
    if (max <= sliderMin) max = sliderMin + 0.0001;
    undertowSlider.setRange(sliderMin, max, sliderIncrement);

    for(int i=0; i<maxSliders; i++)
    {
        juce::Slider* newSlider = displaySliders.getUnchecked(i);

        newSlider->setBounds(xpos, newSlider->getY(), getWidth() - xpos, newSlider->getHeight());
        newSlider->setRange(sliderMin, max, sliderIncrement);
    }

}

void BKWaveDistanceUndertowSlider::setUndertow(int newundertow, juce::NotificationType notify)
{
    undertowSlider.setValue(newundertow, notify);
}



// ******************************************************************************************************************** //
// *************************************************  BKStackedSlider ************************************************* //
// ******************************************************************************************************************** //

BKStackedSlider::BKStackedSlider(
    juce::String sliderName,
    double min,
    double max,
    double defmin,
    double defmax,
    double def,
    double increment,
    int numActiveSliders,
    const juce::ValueTree& stateDefault) : StateModulatedComponent(stateDefault),
           sliderName(sliderName),
           sliderMin(min),
           sliderMax(max),
           sliderMinDefault(defmin),
           sliderMaxDefault(defmax),
           sliderDefault(def),
           sliderIncrement(increment),
           numActiveSliders(numActiveSliders)
{
    editValsTextField = std::make_unique<juce::TextEditor>();
    editValsTextField->setMultiLine(true);
    editValsTextField->setName("PARAMTXTEDIT");
    editValsTextField->addListener(this);
    editValsTextField->setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey);
    addAndMakeVisible(*editValsTextField);
    editValsTextField->setVisible(false);
    editValsTextField->setInterceptsMouseClicks(false, false);
    setInterceptsMouseClicks(true, true);

    numSliders = 12;

    for(int i=0; i<numSliders; i++)
    {
        dataSliders.add(new juce::Slider);
        juce::Slider* newSlider = dataSliders.getLast();

        newSlider->setSliderStyle(juce::Slider::LinearBar);
        newSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        newSlider->setRange(sliderMin, sliderMax, sliderIncrement);
        newSlider->setValue(sliderDefault, juce::dontSendNotification);
        newSlider->setLookAndFeel(&stackedSliderLookAndFeel);
        //newSlider->addListener(this);
        addAndMakeVisible(newSlider);
        if(i>(numActiveSliders -1)) {
            newSlider->setVisible(false);
            activeSliders.insert(i, false);
        }
        else activeSliders.insert(i, true);
    }

    topSlider = std::make_unique<juce::Slider>();
    topSlider->setSliderStyle(juce::Slider::LinearBar);
    topSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, true, 0,0);
    //topSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, true, 50,50);
    topSlider->setRange(sliderMin, sliderMax, sliderIncrement);
    topSlider->setValue(sliderDefault, juce::dontSendNotification);
    topSlider->addListener(this);
    topSlider->addMouseListener(this, true);
    topSlider->setLookAndFeel(&topSliderLookAndFeel);
    //topSlider->setAlpha(0.);
    addAndMakeVisible(*topSlider);

    topSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colour::greyLevel (0.8f).contrasting().withAlpha (0.0f));
    stackedSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.95));

    sliderBorder.setName("transpositionSlider");
    sliderBorder.setText("Transpositions");
    sliderBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(sliderBorder);

    //attachment = std::make_unique<chowdsp::SliderAttachment>(params.delayParam, listeners, *delay_, nullptr);

}

void BKStackedSlider::setDim(float alphaVal)
{
    //showName.setAlpha(alphaVal);
    //topSlider->setAlpha(alphaVal);

    for(int i=0; i<numSliders; i++)
    {
        juce::Slider* newSlider = dataSliders.operator[](i);
        if(newSlider != nullptr)
        {
            if(activeSliders.getUnchecked(i))
            {
                newSlider->setAlpha(alphaVal);
            }
        }
    }
}

void BKStackedSlider::setBright()
{
    //showName.setAlpha(1.);
    //topSlider->setAlpha(1.);

    for(int i=0; i<numSliders; i++)
    {
        juce::Slider* newSlider = dataSliders.operator[](i);
        if(newSlider != nullptr)
        {
            if(activeSliders.getUnchecked(i))
            {
                newSlider->setAlpha(1.);
            }
        }
    }
}

void BKStackedSlider::sliderValueChanged (juce::Slider *slider)
{

}

void BKStackedSlider::addSlider(juce::NotificationType newnotify)
{
    juce::Array<float> sliderVals = getAllActiveValues();
    //sliderVals.add(sliderDefault);
    sliderVals.add(topSlider->proportionOfLengthToValue((double)clickedPosition / getWidth()));
    setTo(sliderVals, newnotify);
    topSlider->setValue(topSlider->proportionOfLengthToValue((double)clickedPosition / getWidth()), juce::dontSendNotification);

    listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
        getName(),
        getAllActiveValues());
}

void BKStackedSlider::setTo(juce::Array<float> newvals, juce::NotificationType newnotify)
{

    int slidersToActivate = newvals.size();
    if(slidersToActivate > numSliders) slidersToActivate = numSliders;
    if(slidersToActivate <= 0)
    {
        slidersToActivate = 1;
        newvals.add(sliderDefault);
    }

    //activate sliders
    for(int i=0; i<slidersToActivate; i++)
    {
        juce::Slider* newSlider = dataSliders.operator[](i);
        if(newSlider != nullptr)
        {
            if(newvals.getUnchecked(i) > sliderMax)
                newSlider->setRange(sliderMin, newvals.getUnchecked(i), sliderIncrement);

            if(newvals.getUnchecked(i) < sliderMin)
                newSlider->setRange(newvals.getUnchecked(i), sliderMax, sliderIncrement);

            newSlider->setValue(newvals.getUnchecked(i));
            newSlider->setVisible(true);
        }

        activeSliders.set(i, true);
    }

    //deactivate unused sliders
    for(int i=slidersToActivate; i<numSliders; i++)
    {

        juce::Slider* newSlider = dataSliders.operator[](i);
        if(newSlider != nullptr)
        {
            //newSlider->setValue(sliderDefault);
            newSlider->setVisible(false);
        }

        activeSliders.set(i, false);
    }

    //make sure there is one!
    if(slidersToActivate <= 0)
    {
        dataSliders.getFirst()->setValue(sliderDefault);
        activeSliders.set(0, true);
    }

    resetRanges();

    topSlider->setValue(dataSliders.getFirst()->getValue(), juce::dontSendNotification);
}


void BKStackedSlider::mouseDown (const juce::MouseEvent &event)
{
    DBG("mouseup");
    if(event.mouseWasClicked())
    {
        clickedSlider = whichSlider();
        clickedPosition = event.x;

        mouseJustDown = true;

        if(event.mods.isCtrlDown())
        {
            addSlider(juce::sendNotification);
//            showModifyPopupMenu();
        }
    }
    if (editValsTextField->hasKeyboardFocus(false)) {
        editValsTextField->mouseDown(event);
    }
}


void BKStackedSlider::mouseDrag(const juce::MouseEvent& e)
{
    if(!mouseJustDown)
    {
        juce::Slider* currentSlider = dataSliders.operator[](clickedSlider);
        if(currentSlider != nullptr)
        {
            if(e.mods.isShiftDown())
            {
                currentSlider->setValue(round(topSlider->getValue()));
                topSlider->setValue(round(topSlider->getValue()));
            }
            else {
                currentSlider->setValue(topSlider->getValue(), juce::sendNotification);
            }
        }
    }
    else mouseJustDown = false;
    if (editValsTextField->hasKeyboardFocus(false)) {
        editValsTextField->mouseDrag(e);
    }
}

void BKStackedSlider::mouseUp(const juce::MouseEvent& e)
{

    listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
        getName(),
        getAllActiveValues());
    if (editValsTextField->hasKeyboardFocus(false)) {
        editValsTextField->mouseUp(e);
    }
}

void BKStackedSlider::mouseMove(const juce::MouseEvent& e)
{
    topSlider->setValue(dataSliders.getUnchecked(whichSlider(e))->getValue());
}


void BKStackedSlider::mouseDoubleClick (const juce::MouseEvent &e)
{
    juce::StringArray tokens;
    tokens.addTokens(BKfloatArrayToString(getAllActiveValues()), false); //arrayFloatArrayToString
    int startPoint = 0;
    int endPoint;

    int which = whichSlider();

    for(int i=0; i < which; i++) {
        startPoint += tokens[i].length() + 1;
    }
    endPoint = startPoint + tokens[which].length();

    editValsTextField->setVisible(true);
    editValsTextField->toFront(true);
    editValsTextField->grabKeyboardFocus();
    editValsTextField->setText(BKfloatArrayToString(getAllActiveValues()), juce::dontSendNotification); //arrayFloatArrayToString
    //editValsTextField->setInterceptsMouseClicks(true, true);
    juce::Range<int> highlightRange(startPoint, endPoint);
    editValsTextField->setHighlightedRegion(highlightRange);

    focusLostByEscapeKey = false;
}

void BKStackedSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        isEditing = true;
        editValsTextField->setVisible(false);
        editValsTextField->toBack();
       // editValsTextField->setInterceptsMouseClicks(false, false);
        setTo(BKstringToFloatArray(textEditor.getText()), juce::dontSendNotification);
        clickedSlider = 0;
        resized();

        listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
            getName(),
            getAllActiveValues());
        isEditing = false;
    }
}

void BKStackedSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
            textEditorEscapeKeyPressed(textEditor);
    }
}

void BKStackedSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        focusLostByEscapeKey = true;
        editValsTextField->setVisible(false);
        //editValsTextField->setInterceptsMouseClicks(false, false);

        editValsTextField->toBack();

        unfocusAllComponents();
    }
}

void BKStackedSlider::textEditorTextChanged(juce::TextEditor& textEditor)
{
}

void BKStackedSlider::resetRanges()
{

    double sliderMinTemp = sliderMinDefault;
    double sliderMaxTemp = sliderMaxDefault;

    for(int i = 0; i<dataSliders.size(); i++)
    {
        juce::Slider* currentSlider = dataSliders.operator[](i);
        if(currentSlider != nullptr)
        {
            if(currentSlider->getValue() > sliderMaxTemp) sliderMaxTemp = currentSlider->getValue();
            if(currentSlider->getValue() < sliderMinTemp) sliderMinTemp = currentSlider->getValue();
        }
    }

    if( (sliderMax != sliderMaxTemp) || sliderMin != sliderMinTemp)
    {
        sliderMax = sliderMaxTemp;
        sliderMin = sliderMinTemp;

        for(int i = 0; i<dataSliders.size(); i++)
        {
            juce::Slider* currentSlider = dataSliders.operator[](i);
            if(currentSlider != nullptr)
            {
                currentSlider->setRange(sliderMin, sliderMax, sliderIncrement);
            }
        }

        topSlider->setRange(sliderMin, sliderMax, sliderIncrement);
    }
}


juce::Array<float> BKStackedSlider::getAllActiveValues()
{
    juce::Array<float> currentVals;

    for(int i=0; i<dataSliders.size(); i++)
    {
        juce::Slider* currentSlider = dataSliders.operator[](i);
        if(currentSlider != nullptr)
        {
            if(activeSliders.getUnchecked(i))
                currentVals.add(currentSlider->getValue());
        }

    }

    return currentVals;
}

int BKStackedSlider::whichSlider()
{
    float refDistance;
    int whichSub = 0;

    juce::Slider* refSlider = dataSliders.getFirst();
    refDistance = fabs(refSlider->getValue() - topSlider->getValue());

    for(int i=1; i<dataSliders.size(); i++)
    {
        if(activeSliders.getUnchecked(i))
        {
            juce::Slider* currentSlider = dataSliders.operator[](i);
            if(currentSlider != nullptr) {
                float tempDistance = fabs(currentSlider->getValue() - topSlider->getValue());
                if(tempDistance < refDistance)
                {
                    whichSub = i;
                    refDistance = tempDistance;
                }
            }
        }
    }

    return whichSub;
}

int BKStackedSlider::whichSlider(const juce::MouseEvent& e)
{
    float refDistance;
    float topSliderVal = topSlider->proportionOfLengthToValue((double)e.x / getWidth());

    int whichSub = 0;

    juce::Slider* refSlider = dataSliders.getFirst();
    refDistance = fabs(refSlider->getValue() - topSliderVal);

    for(int i=1; i<dataSliders.size(); i++)
    {
        if(activeSliders.getUnchecked(i))
        {
            juce::Slider* currentSlider = dataSliders.operator[](i);
            if(currentSlider != nullptr) {
                float tempDistance = fabs(currentSlider->getValue() - topSliderVal);
                if(tempDistance < refDistance)
                {
                    whichSub = i;
                    refDistance = tempDistance;
                }
            }
        }
    }

    return whichSub;
}

void BKStackedSlider::showModifyPopupMenu()
{
    juce::PopupMenu m;
    m.addItem (1, juce::translate ("add transposition"), true, false);
    m.addSeparator();

    m.showMenuAsync (juce::PopupMenu::Options(),
        juce::ModalCallbackFunction::forComponent (sliderModifyMenuCallback, this));
}

void BKStackedSlider::sliderModifyMenuCallback (const int result, BKStackedSlider* ss)
{
    if (ss == nullptr)
    {
        juce::PopupMenu::dismissAllActiveMenus();
        return;
    }

    switch (result)
    {
        case 1:   ss->addSlider(juce::sendNotification); break;

        default:  break;
    }
}

void BKStackedSlider::resized ()
{
    juce::Rectangle<int> area (getLocalBounds());
    sliderBorder.setBounds(area);
    area.removeFromTop(10);
    area.removeFromBottom(2);
    area.reduce(4, 0);

    topSlider->setBounds(area);
    topSlider->setValue(dataSliders.getFirst()->getValue(), juce::dontSendNotification);

    editValsTextField->setBounds(area);
    editValsTextField->setVisible(false);

    for(int i=0; i<numSliders; i++)
    {
        juce::Slider* newSlider = dataSliders.getUnchecked(i);
        newSlider->setBounds(area);
    }
}




// ******************************************************************************************************************** //
// **************************************************  BKRangeSlider ************************************************** //
// ******************************************************************************************************************** //

BKRangeSlider::BKRangeSlider (juce::String name, double min, double max, double defmin, double defmax, double increment, const juce::ValueTree& stateDefault):
StateModulatedComponent(stateDefault),
                                                                                                                     sliderName(name),
                                                                                                                     sliderMin(min),
                                                                                                                     sliderMax(max),
                                                                                                                     sliderDefaultMin(defmin),
                                                                                                                     sliderDefaultMax(defmax),
                                                                                                                     sliderIncrement(increment)
{

    justifyRight = true;

    showName.setText(sliderName, juce::dontSendNotification);
    if(justifyRight) showName.setJustificationType(juce::Justification::bottomRight);
    else showName.setJustificationType(juce::Justification::bottomLeft);
    addAndMakeVisible(showName);

    minValueTF.setText(juce::String(sliderDefaultMin));
    minValueTF.setName("minvalue");
    minValueTF.addListener(this);
    minValueTF.setSelectAllWhenFocused(true);
    minValueTF.setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey);
    addAndMakeVisible(minValueTF);

    maxValueTF.setText(juce::String(sliderDefaultMax));
    maxValueTF.setName("maxvalue");
    maxValueTF.addListener(this);
    maxValueTF.setSelectAllWhenFocused(true);
    maxValueTF.setJustification(juce::Justification(2));
    maxValueTF.setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey);
    addAndMakeVisible(maxValueTF);

    minSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    minSlider.setRange(sliderMin, sliderMax, sliderIncrement);
    minSlider.setValue(sliderDefaultMin, juce::dontSendNotification);
    minSlider.addListener(this);
    minSlider.setLookAndFeel(&minSliderLookAndFeel);
    //minSlider.setInterceptsMouseClicks(false, true);
    //minSliderLookAndFeel.setColour(Slider::trackColourId, Colour::fromRGBA(55, 105, 250, 50));
    addAndMakeVisible(minSlider);

    maxSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    maxSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    maxSlider.setRange(sliderMin, sliderMax, sliderIncrement);
    maxSlider.setValue(sliderDefaultMax, juce::dontSendNotification);
    maxSlider.addListener(this);
    maxSlider.setLookAndFeel(&maxSliderLookAndFeel);
    //maxSlider.setInterceptsMouseClicks(false, true);
    //maxSliderLookAndFeel.setColour(Slider::trackColourId, Colour::greyLevel (0.8f).contrasting().withAlpha (0.13f));
    //maxSliderLookAndFeel.setColour(Slider::trackColourId, Colour::fromRGBA(55, 105, 250, 50));
    addAndMakeVisible(maxSlider);

    invisibleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    invisibleSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    invisibleSlider.setRange(sliderMin, sliderMax, sliderIncrement);
    invisibleSlider.setValue(sliderDefaultMin, juce::dontSendNotification);
    invisibleSlider.setAlpha(0.0);
    invisibleSlider.addListener(this);
    invisibleSlider.addMouseListener(this, true);
    //invisibleSlider.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(invisibleSlider);

    newDrag = false;
    isMinAlwaysLessThanMax = false;

    displaySlider = std::make_unique<juce::Slider>();
    displaySlider->setRange(min, max, increment);
    displaySlider->setSliderStyle(juce::Slider::SliderStyle::LinearBar);
    displaySlider->setLookAndFeel(&displaySliderLookAndFeel);
    displaySlider->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(*displaySlider);

    rangeSliderBorder.setName("rangeSlider");
    rangeSliderBorder.setText("Velocity Range");
    rangeSliderBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(rangeSliderBorder);

}

void BKRangeSlider::setSkew(float newskew)
{
    minSlider.setSkewFactor(newskew);
    maxSlider.setSkewFactor(newskew);
    displaySlider->setSkewFactor(newskew);
}

void BKRangeSlider::setDim(float alphaVal)
{
    minSlider.setAlpha(alphaVal);
    maxSlider.setAlpha(alphaVal);
    showName.setAlpha(alphaVal);
    minValueTF.setAlpha(alphaVal);
    maxValueTF.setAlpha(alphaVal);
}

void BKRangeSlider::setBright()
{
    minSlider.setAlpha(1.);
    maxSlider.setAlpha(1.);
    showName.setAlpha(1.);
    minValueTF.setAlpha(1.);
    maxValueTF.setAlpha(1.);
}


void BKRangeSlider::setMinValue(double newval, juce::NotificationType notify)
{
    checkValue(newval);
    minSlider.setValue(newval, notify);
    minValueTF.setText(juce::String(minSlider.getValue()), juce::dontSendNotification);
    rescaleMinSlider();
    // displaySlider->setRange(sliderMin, sliderMax, sliderIncrement);
}


void BKRangeSlider::setMaxValue(double newval, juce::NotificationType notify)
{
    checkValue(newval);
    maxSlider.setValue(newval, notify);
    maxValueTF.setText(juce::String(maxSlider.getValue()), juce::dontSendNotification);
    rescaleMaxSlider();

}


void BKRangeSlider::sliderValueChanged (juce::Slider *slider)
{

    if(slider == &invisibleSlider)
    {
        if(newDrag)
        {
            if(!clickedOnMinSlider)
            {
                maxSlider.setValue(invisibleSlider.getValue(), juce::sendNotification);
                maxValueTF.setText(juce::String(maxSlider.getValue()), juce::dontSendNotification);
                if(isMinAlwaysLessThanMax)
                    if(maxSlider.getValue() < minSlider.getValue())
                        setMinValue(maxSlider.getValue(), juce::dontSendNotification);
                //displaySlider->setRange(0,invisibleSlider.getValue(), sliderIncrement);
            }
            else
            {
                minSlider.setValue(invisibleSlider.getValue(), juce::sendNotification);
                minValueTF.setText(juce::String(minSlider.getValue()), juce::dontSendNotification);
                if(isMinAlwaysLessThanMax)
                    if(minSlider.getValue() > maxSlider.getValue())
                        setMaxValue(minSlider.getValue(), juce::dontSendNotification);
            }

            listeners.call(&BKRangeSlider::Listener::BKRangeSliderValueChanged,
                getName(),
                minSlider.getValue(),
                maxSlider.getValue());
        }
    }
}


void BKRangeSlider::mouseDown (const juce::MouseEvent &event)
{
    Component* ec = event.eventComponent;

    if (ec == &invisibleSlider)
    {
        if(event.mouseWasClicked())
        {
            if(event.y > invisibleSlider.getHeight() / 2.)
            {
                clickedOnMinSlider = true;
            }
            else
            {
                clickedOnMinSlider = false;
            }

            newDrag = true;
        }

        unfocusAllComponents();
    }
}


void BKRangeSlider::sliderDragEnded(juce::Slider *slider)
{
    newDrag = false;
    unfocusAllComponents();
}


void BKRangeSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    double newval = textEditor.getText().getDoubleValue();

    //adjusts min/max of sldiers as needed
    checkValue(newval);

    if(textEditor.getName() == "minvalue")
    {
        minSlider.setValue(newval, juce::sendNotification);
        if(isMinAlwaysLessThanMax)
            if(minSlider.getValue() > maxSlider.getValue())
                setMaxValue(minSlider.getValue(), juce::dontSendNotification);
        rescaleMinSlider();
    }
    else if(textEditor.getName() == "maxvalue")
    {
        maxSlider.setValue(newval, juce::sendNotification);
        if(isMinAlwaysLessThanMax)
            if(maxSlider.getValue() < minSlider.getValue())
                setMinValue(maxSlider.getValue(), juce::dontSendNotification);
        rescaleMaxSlider();
    }

    unfocusAllComponents();

    listeners.call(&BKRangeSlider::Listener::BKRangeSliderValueChanged,
        getName(),
        minSlider.getValue(),
        maxSlider.getValue());
}


void BKRangeSlider::textEditorTextChanged(juce::TextEditor& textEditor)
{
    focusLostByEscapeKey = false;

}


void BKRangeSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
{
    focusLostByEscapeKey = true;
    unfocusAllComponents();
}


void BKRangeSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
}


void BKRangeSlider::checkValue(double newval)
{
    if(newval > maxSlider.getMaximum()) {
        maxSlider.setRange(minSlider.getMinimum(), newval, sliderIncrement);
        minSlider.setRange(minSlider.getMinimum(), newval, sliderIncrement);
        invisibleSlider.setRange(minSlider.getMinimum(), newval, sliderIncrement);
    }

    if(newval < minSlider.getMinimum()) {
        maxSlider.setRange(newval, maxSlider.getMaximum(), sliderIncrement);
        minSlider.setRange(newval, maxSlider.getMaximum(), sliderIncrement);
        invisibleSlider.setRange(newval, maxSlider.getMaximum(), sliderIncrement);
    }
}


void BKRangeSlider::rescaleMinSlider()
{
    if(minSlider.getMinimum() < sliderMin &&
        minSlider.getValue() > sliderMin &&
        maxSlider.getValue() > sliderMin)
    {
        maxSlider.setRange(sliderMin, maxSlider.getMaximum(), sliderIncrement);
        minSlider.setRange(sliderMin, maxSlider.getMaximum(), sliderIncrement);
        invisibleSlider.setRange(sliderMin, maxSlider.getMaximum(), sliderIncrement);

    }
}

void BKRangeSlider::rescaleMaxSlider()
{

    if(maxSlider.getMaximum() > sliderMax &&
        maxSlider.getValue() < sliderMax &&
        minSlider.getValue() < sliderMax
    )
    {
        maxSlider.setRange(minSlider.getMinimum(), sliderMax, sliderIncrement);
        minSlider.setRange(minSlider.getMinimum(), sliderMax, sliderIncrement);
        invisibleSlider.setRange(minSlider.getMinimum(), sliderMax, sliderIncrement);

    }
}

void BKRangeSlider::resized()
{
    // draw the border
    juce::Rectangle<int> area (getLocalBounds());
    rangeSliderBorder.setBounds(area);
    area.removeFromTop(gComponentTextFieldHeight + 2);

    // figure out how much to remove from top and bottom to center slider
    int areaHeight = area.getHeight();
    int sliderHeight = gComponentRangeSliderHeight;
    int removeFromTopAndBottom = areaHeight - (sliderHeight/2);

    // place the sliders
    juce::Rectangle<int> sliderArea (area.removeFromTop(removeFromTopAndBottom).removeFromBottom(removeFromTopAndBottom));
    minSlider.setBounds(sliderArea);
    maxSlider.setBounds(sliderArea);
    invisibleSlider.setBounds(sliderArea);

//    maxValueTF.setBounds(maxSlider.getRight() - 72, maxSlider.getY() - 12, 52, 22);
//    minValueTF.setBounds(maxSlider.getX() + 20, maxSlider.getBottom() - 10, 52, 22);

    // place the text fields
    minValueTF.setBounds(maxSlider.getX() + 10, maxSlider.getY() - 14, 52, 22);
    maxValueTF.setBounds(maxSlider.getRight() - 62, maxSlider.getBottom() - 8, 52, 22);

    // blue slider to show most recently played velocity
    juce::Rectangle<int> displaySliderArea = maxSlider.getBounds();
    displaySliderArea.reduce(8, 0);
    displaySlider->setBounds(displaySliderArea.removeFromBottom(8));
}
