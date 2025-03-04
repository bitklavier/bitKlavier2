//
// Created by Dan Trueman on 10/22/24.
//

#include "BKSliders.h"

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


BKStackedSlider::BKStackedSlider(
    juce::String sliderName,
    double min,
    double max,
    double defmin,
    double defmax,
    double def,
    double increment):
           sliderName(sliderName),
           sliderMin(min),
           sliderMax(max),
           sliderMinDefault(defmin),
           sliderMaxDefault(defmax),
           sliderDefault(def),
           sliderIncrement(increment)
{

    showName.setText(sliderName, juce::dontSendNotification);
    showName.setInterceptsMouseClicks(false, true);
    addAndMakeVisible(showName);

    editValsTextField = std::make_unique<juce::TextEditor>();
    editValsTextField->setMultiLine(true);
    editValsTextField->setName("PARAMTXTEDIT");
    editValsTextField->addListener(this);
    editValsTextField->setColour(juce::TextEditor::highlightColourId, juce::Colours::darkgrey);
#if JUCE_IOS
    editValsTextField->setReadOnly(true);
    editValsTextField->setCaretVisible(true);
#endif
    addAndMakeVisible(*editValsTextField);
    editValsTextField->setVisible(false);

    numSliders = 12;
    numActiveSliders = 1;

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
        if(i>0) {
            newSlider->setVisible(false);
            activeSliders.insert(i, false);
        }
        else activeSliders.insert(0, true);
    }

    topSlider = std::make_unique<juce::Slider>();
    topSlider->setSliderStyle(juce::Slider::LinearBar);
    topSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxLeft, true, 0,0);
    //topSlider->setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxLeft, true, 50,50);
    topSlider->setRange(sliderMin, sliderMax, sliderIncrement);
    topSlider->setValue(sliderDefault, juce::dontSendNotification);
    topSlider->addListener(this);
    topSlider->addMouseListener(this, true);
    topSlider->setLookAndFeel(&topSliderLookAndFeel);
    topSlider->setAlpha(0.);
    addAndMakeVisible(*topSlider);

    topSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colour::greyLevel (0.8f).contrasting().withAlpha (0.0f));
    stackedSliderLookAndFeel.setColour(juce::Slider::thumbColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.95));

    //attachment = std::make_unique<chowdsp::SliderAttachment>(params.delayParam, listeners, *delay_, nullptr);

}

void BKStackedSlider::setDim(float alphaVal)
{
    showName.setAlpha(alphaVal);
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
    showName.setAlpha(1.);
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
            newSlider->setValue(sliderDefault);
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
}

void BKStackedSlider::mouseUp(const juce::MouseEvent& e)
{

    listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
        getName(),
        getAllActiveValues());

}

void BKStackedSlider::mouseMove(const juce::MouseEvent& e)
{
    //topSlider->setValue(topSlider->proportionOfLengthToValue((double)e.x / getWidth()), dontSendNotification);
//    topSlider->setValue(dataSliders.getUnchecked(whichSlider(e))->getValue());
//
//    for(int i=0; i<dataSliders.size(); i++)
//    {
//        if(dataSliders.getUnchecked(whichSlider(e)) == dataSliders.getUnchecked(i))
//            dataSliders.getUnchecked(i)->setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 50);
//        else
//            dataSliders.getUnchecked(i)->setTextBoxStyle(juce::Slider::NoTextBox, false, 50, 50);
//    }
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

    juce::Range<int> highlightRange(startPoint, endPoint);
    editValsTextField->setHighlightedRegion(highlightRange);

    focusLostByEscapeKey = false;
}

void BKStackedSlider::textEditorReturnKeyPressed(juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        editValsTextField->setVisible(false);
        editValsTextField->toBack();

        setTo(BKstringToFloatArray(textEditor.getText()), juce::sendNotification);
        clickedSlider = 0;
        resized();

        listeners.call(&BKStackedSlider::Listener::BKStackedSliderValueChanged,
            getName(),
            getAllActiveValues());

    }
}

void BKStackedSlider::textEditorFocusLost(juce::TextEditor& textEditor)
{
#if !JUCE_IOS
    if(!focusLostByEscapeKey) {
        textEditorReturnKeyPressed(textEditor);
    }
#endif
}

void BKStackedSlider::textEditorEscapeKeyPressed (juce::TextEditor& textEditor)
{
    if(textEditor.getName() == editValsTextField->getName())
    {
        focusLostByEscapeKey = true;
        editValsTextField->setVisible(false);
        editValsTextField->toBack();
        unfocusAllComponents();
    }
}

void BKStackedSlider::textEditorTextChanged(juce::TextEditor& textEditor)
{
#if JUCE_IOS
    if (hasBigOne)
    {
        hasBigOne = false;
        textEditorReturnKeyPressed(textEditor);
    }
#endif
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

    showName.setBounds(area.toNearestInt());
    showName.setJustificationType(juce::Justification::topRight);
    showName.toFront(false);

    topSlider->setBounds(area);

    editValsTextField->setBounds(area);
    editValsTextField->setVisible(false);

    for(int i=0; i<numSliders; i++)
    {
        juce::Slider* newSlider = dataSliders.getUnchecked(i);
        newSlider->setBounds(area);
    }

}
