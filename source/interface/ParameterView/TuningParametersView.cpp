//
// Created by Dan Trueman on 11/5/24.
//

#include "TuningParametersView.h"

TuningParametersView::TuningParametersView(chowdsp::PluginState& pluginState, TuningParams& param, juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
{
    setName("tuning");
    setLookAndFeel(DefaultLookAndFeel::instance());
    setComponentID(name);

    offsetKnobBorder.setName("offsetknobborder");
    offsetKnobBorder.setText("Offset");
    offsetKnobBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(offsetKnobBorder);

    auto& listeners = pluginState.getParameterListeners();
    for ( auto &param_ : *params.getFloatParams())
    {
        if (param_->getParameterID() == "lastNote") continue; // handle this separately
        auto slider = std::make_unique<SynthSlider>(param_->paramID);
        auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
        addSlider(slider.get());
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setShowPopupOnHover(true);
        floatAttachments.emplace_back(std::move(attachment));
        _sliders.emplace_back(std::move(slider));
    }
    /**
     * todo: there is only one knob, so reduce the above, no need for the loop
     */
    _sliders[0]->setName("cents");

    keyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState);
    addStateModulatedComponent(keyboard.get());
    keyboard->setName("absolute");

    circular_keyboard = std::make_unique<OpenGLCircularKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState);
    addStateModulatedComponent(circular_keyboard.get());
    circular_keyboard->setName("circular");

    semitoneSection = std::make_unique<SemitoneWidthSection>(name, params.tuningState.semitoneWidthParams, listeners, *this);
    addSubSection(semitoneSection.get());

    adaptiveSection = std::make_unique<AdaptiveTuningSection>(name, params.tuningState.adaptiveParams, listeners, *this);
    addSubSection(adaptiveSection.get());

    springTuningSection = std::make_unique<SpringTuningSection>(name, params.tuningState.springTuningParams, listeners, *this);
    addSubSection(springTuningSection.get());

    if (auto* tuningParams = dynamic_cast<TuningParams*>(&params)) {
        ///tuning systems
        auto index = tuningParams->tuningState.tuningSystem->getIndex();
        tuning_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.tuningSystem->paramID.toStdString());
        tuning_attachment= std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningSystem.get(), listeners,*tuning_combo_box, nullptr);
        addAndMakeVisible(tuning_combo_box.get());
        addOpenGlComponent(tuning_combo_box->getImageComponent());
        setupTuningSystemMenu(tuning_combo_box);
        tuning_combo_box->setSelectedItemIndex(index,juce::sendNotificationSync);

        fundamental_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.fundamental->paramID.toStdString());
        fundamental_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.fundamental.get(), listeners,*fundamental_combo_box, nullptr);
        addAndMakeVisible(fundamental_combo_box.get());
        addOpenGlComponent(fundamental_combo_box->getImageComponent());

        tuningtype_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.tuningType->paramID.toStdString());
        tuningtype_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningType.get(), listeners,*tuningtype_combo_box, nullptr);
        addAndMakeVisible(tuningtype_combo_box.get());
        addOpenGlComponent(tuningtype_combo_box->getImageComponent());
    }

    /*
     * display relevant subsets of the UI depending on selected TuningType
     */
    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.tuningType,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            if(params.tuningState.tuningType->get() == TuningType::Static)
                showStaticTuning(true);
            else
                showStaticTuning(false);

            if(params.tuningState.tuningType->get() == TuningType::Adaptive || params.tuningState.tuningType->get() == TuningType::Adaptive_Anchored)
                showAdaptiveTuning(true);
            else
                showAdaptiveTuning(false);

            if(params.tuningState.tuningType->get() == TuningType::Spring_Tuning)
                showSpringTuning(true);
            else
                showSpringTuning(false);

            /*
             * important call to get the backgrounds to redraw properly!
             */
            auto interface = findParentComponentOfClass<SynthGuiInterface>();
            interface->getGui()->prep_popup->repaintPrepBackground();

        })
    };

    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.tuningSystem,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]()
        {
            setOffsetsFromTuningSystem(
                params.tuningState.tuningSystem->get(),
                params.tuningState.fundamental->getIndex(),
                this->params.tuningState.circularTuningOffset,
                this->params.tuningState.circularTuningOffset_custom);

            //params.tuningState.setFromAudioThread = false;
            circular_keyboard->redoImage();
        })
    };

    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.fundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            params.tuningState.setFundamental(params.tuningState.fundamental->getIndex());
            circular_keyboard->redoImage();
        })
    };

    // to catch presses of the reset button
    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.adaptiveParams.tReset,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            if (params.tuningState.adaptiveParams.tReset.get()->get()) {
                params.tuningState.adaptiveReset();
                params.tuningState.adaptiveParams.tReset->setParameterValue(false);
            }
        })
    };

    lastNoteDisplay = std::make_shared<PlainTextComponent>("lastpitch", "Last Pitch = 69.00");
    addOpenGlComponent(lastNoteDisplay);
    lastNoteDisplay->setTextSize (12.0f);
    lastNoteDisplay->setJustification(juce::Justification::centred);

    lastFrequencyDisplay = std::make_shared<PlainTextComponent>("lastfrequency", "Last Frequency = 440");
    addOpenGlComponent(lastFrequencyDisplay);
    lastFrequencyDisplay->setTextSize (12.0f);
    lastFrequencyDisplay->setJustification(juce::Justification::centred);

    lastIntervalDisplay = std::make_shared<PlainTextComponent>("lastinterval", "Last Interval = 0.00");
    addOpenGlComponent(lastIntervalDisplay);
    lastIntervalDisplay->setTextSize (12.0f);
    lastIntervalDisplay->setJustification(juce::Justification::centred);

    /*
     * this will hear changes to tuningState.lastNote, which are triggered by the last tuned note in the synth (through synthState, called in DirectProcessor, etc...)
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.lastNote,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            lastNoteDisplay->setText ("Last Pitch = " + this->params.tuningState.lastNote->getCurrentValueAsText());
            lastFrequencyDisplay->setText ("Last Frequency = " + juce::String(mtof(
                                   this->params.tuningState.lastNote->getCurrentValue())
                                   * this->params.tuningState.getGlobalTuningReference() / 440.,
                                               2));
            lastIntervalDisplay->setText ("Last Interval = " + juce::String(this->params.tuningState.lastIntervalCents, 2));
        })
    };

    /*
     * similar, listening for changes to current adaptive fundamental, for display
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.adaptiveParams.tCurrentAdaptiveFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            adaptiveSection->currentFundamental->setText ("Current Fundamental = " + this->params.tuningState.adaptiveParams.tCurrentAdaptiveFundamental_string);
        })
    };

    /*
     * similar, listening for changes to current spring tuning fundamental, for display
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.tCurrentSpringTuningFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            springTuningSection->currentFundamental->setText ("Current Fundamental = " + this->params.tuningState.springTuningParams.tCurrentSpringTuningFundamental->getCurrentValueAsText());
        })
    };

    /*
     * callbacks for all the spring tuning params
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.scaleId,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->intervalScaleChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.intervalFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->intervalFundamentalChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.scaleId_tether,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->tetherScaleChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.tetherFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->tetherFundamentalChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.rate,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->rateChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.drag,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->dragChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.tetherStiffness,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->tetherStiffnessChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.intervalStiffness,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            params.tuningState.springTuner->intervalStiffnessChanged();
        })
    };

    circular_keyboard->addMyListener(this);
}


void TuningParametersView::showStaticTuning(bool show)
{
    keyboard->setVisible(show);
    circular_keyboard->setVisible(show);
    semitoneSection->setVisible(show);

    // this will show/hide the offset knob, which is the only thing in _sliders
    for (auto &ts_ : _sliders)
    {
        ts_->setVisible(show);
    }
}

void TuningParametersView::showAdaptiveTuning(bool show)
{
    adaptiveSection->setVisible(show);
}

void TuningParametersView::showSpringTuning(bool show)
{
    springTuningSection->setVisible(show);
}

void TuningParametersView::resized()
{
    juce::Rectangle<int> area (getLocalBounds());

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int labelsectionheight = findValue(Skin::kLabelHeight);
    int title_width = getTitleWidth();
    int circularKeyboardTargetHeight = 120;
    int circularKeyboardTargetWidth = 350;
    int semitoneBoxTargetHeight = 81; // scale these...
    int semitoneBoxTargetWidth = 176;

    area.removeFromLeft(title_width);
    juce::Rectangle leftHalf = area.removeFromLeft(area.getWidth() * 0.5);
    leftHalf.reduce(largepadding, largepadding);
    juce::Rectangle areaSpring = leftHalf;
    juce::Rectangle areaAdaptive = leftHalf;

    juce::Rectangle lastValsDisplay = leftHalf.removeFromBottom(labelsectionheight);
    lastNoteDisplay->setBounds(lastValsDisplay.removeFromLeft(lastValsDisplay.getWidth() / 3.));
    lastFrequencyDisplay->setBounds(lastValsDisplay.removeFromLeft(lastValsDisplay.getWidth() / 2.));
    lastIntervalDisplay->setBounds(lastValsDisplay);

    juce::Rectangle topMenuBox = leftHalf.removeFromTop(comboboxheight);
    int menuWidthFactor = leftHalf.getWidth() / 7.;
    tuningtype_combo_box->setBounds(topMenuBox.removeFromLeft(menuWidthFactor * 2.));
    tuning_combo_box->setBounds(topMenuBox.removeFromLeft(menuWidthFactor * 3.));
    fundamental_combo_box->setBounds((topMenuBox));

    leftHalf.removeFromTop(largepadding);

    juce::Rectangle circularKeyboardBox = leftHalf.removeFromTop(circularKeyboardTargetHeight);
    if (circularKeyboardBox.getWidth() > circularKeyboardTargetWidth)
        circularKeyboardBox.reduce((circularKeyboardBox.getWidth() - circularKeyboardTargetWidth) / 2., 0);
    circular_keyboard->setBounds(circularKeyboardBox);

    leftHalf.removeFromTop(largepadding);

    juce::Rectangle absoluteKeyboardBox = leftHalf.removeFromTop(circularKeyboardTargetHeight);
    keyboard->setBounds(absoluteKeyboardBox);

    leftHalf.removeFromTop(largepadding);

    juce::Rectangle offsetAndSemitoneBox = leftHalf.removeFromTop(semitoneBoxTargetHeight);
    semitoneSection->setBounds(offsetAndSemitoneBox.removeFromRight(semitoneBoxTargetWidth));

    juce::Rectangle offsetKnobBox = offsetAndSemitoneBox.removeFromLeft(knobsectionheight + 30);
    offsetKnobBorder.setBounds(offsetKnobBox);
    offsetKnobBox.reduce(largepadding, largepadding);
    _sliders[0]->setBounds(offsetKnobBox);

    areaAdaptive.removeFromTop(comboboxheight + largepadding * 2);
    areaSpring.removeFromTop(comboboxheight + largepadding * 2);
    adaptiveSection->setBounds(areaAdaptive.removeFromTop(200));
    springTuningSection->setBounds(areaSpring.removeFromTop(300));



    int knob_section_height = getKnobSectionHeight();
    int knob_y = getHeight() - knob_section_height;

    int widget_margin = findValue(Skin::kWidgetMargin);

    int area_width = getWidth() - 2 * title_width;
    int envelope_height = knob_y - widget_margin;
//    tuningtype_combo_box->setBounds(10,10, 100, 25);
//    tuning_combo_box->setBounds(115,10,100, 25);
//    fundamental_combo_box->setBounds(225, 10, 50,25);
//    keyboard->setBounds(50, 200, 500, 100);
//    circular_keyboard->setBounds(50, 50, 500, 100);

    //semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 50, getKnobSectionHeight() + 50 + getTextComponentHeight() * 2);
//    semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 105, getKnobSectionHeight() + 10);
//    DBG("semintone section h/w " + juce::String(getKnobSectionHeight() + 105) + " " + juce::String(getKnobSectionHeight() + 10));
//    adaptiveSection->setBounds(circular_keyboard->getRight() + 20, circular_keyboard->getY(), 500, 200);

//    juce::Rectangle<int> outputKnobsArea = {50, semitoneSection->getBottom() + knob_section_height, 100, 100};
//        //bounds.removeFromTop(knob_section_height);
//    placeKnobsInArea(outputKnobsArea, _sliders, true);

    juce::Rectangle<int> springTuningBox = {adaptiveSection->getX(), adaptiveSection->getBottom() + 20, 500, 300};
//    springTuningSection->setBounds(springTuningBox);

//    lastNoteDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY(), 100, 30);
//    lastFrequencyDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY() + 30, 100, 30);
//    lastIntervalDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY() + 60, 100, 30);

    SynthSection::resized();
}

void TuningParametersView::keyboardSliderChanged(juce::String name) {

    if (name == "circular")
        params.tuningState.tuningSystem->setParameterValue(TuningSystem::Custom);
}
