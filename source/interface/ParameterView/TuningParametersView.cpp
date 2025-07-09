//
// Created by Dan Trueman on 11/5/24.
//

#include "TuningParametersView.h"

TuningParametersView::TuningParametersView(chowdsp::PluginState& pluginState, TuningParams& param, juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
{
    setName("tuning");
    setLookAndFeel(DefaultLookAndFeel::instance());
    setComponentID(name);

    auto& listeners = pluginState.getParameterListeners();
    for ( auto &param_ : *params.getFloatParams())
    {
        /**
         * todo: i think there is only one float param handled here, so might be good to simplify and remove this loop
         */
        if (param_->getParameterID() == "lastNote") continue; // handle this separately
        auto slider = std::make_unique<SynthSlider>(param_->paramID);
        auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
        addSlider(slider.get());
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        floatAttachments.emplace_back(std::move(attachment));
        _sliders.emplace_back(std::move(slider));
    }

    keyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState);
    addStateModulatedComponent(keyboard.get());

    circular_keyboard = std::make_unique<OpenGLCircularKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState);
    addStateModulatedComponent(circular_keyboard.get());

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

    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.tuningSystem,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]()
        {
            setOffsetsFromTuningSystem(
                params.tuningState.tuningSystem->get(),
                params.tuningState.fundamental->getIndex(),
                this->params.tuningState.circularTuningOffset);

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
    lastNoteDisplay->setJustification(juce::Justification::left);

    lastFrequencyDisplay = std::make_shared<PlainTextComponent>("lastfrequency", "Last Frequency = 440");
    addOpenGlComponent(lastFrequencyDisplay);
    lastFrequencyDisplay->setTextSize (12.0f);
    lastFrequencyDisplay->setJustification(juce::Justification::left);

    lastIntervalDisplay = std::make_shared<PlainTextComponent>("lastinterval", "Last Interval = 0.00");
    addOpenGlComponent(lastIntervalDisplay);
    lastIntervalDisplay->setTextSize (12.0f);
    lastIntervalDisplay->setJustification(juce::Justification::left);

    /*
     * this will hear changes to tuningState.lastNote, which are triggered by the last tuned note in the synth (through synthState, called in DirectProcessor, etc...)
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.lastNote,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            lastNoteDisplay->setText ("Last Pitch = " + this->params.tuningState.lastNote->getCurrentValueAsText());
            lastFrequencyDisplay->setText ("Last Frequency = " + juce::String(mtof(this->params.tuningState.lastNote->getCurrentValue()),2));
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
            DBG("updating spring tuning current fundamental display");
            springTuningSection->currentFundamental->setText ("Current Fundamental = " + this->params.tuningState.springTuningParams.tCurrentSpringTuningFundamental->getCurrentValueAsText());
        })
    };

    /*
     * callbacks for all the spring tuning params
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.scaleId,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("intervalScale (scaleId) changed by user");
            params.tuningState.springTuner->intervalScaleChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.intervalFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("intervalFundamental changed by user");
            params.tuningState.springTuner->intervalFundamentalChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.scaleId_tether,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("scaleId_tether (scaleId) changed by user");
            params.tuningState.springTuner->tetherScaleChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.tetherFundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("tetherFundamental changed by user");
            params.tuningState.springTuner->tetherFundamentalChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.rate,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("rate changed by user");
            params.tuningState.springTuner->rateChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.tetherStiffness,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("tetherStiffness changed by user");
            params.tuningState.springTuner->tetherStiffnessChanged();
        })
    };

    tuningCallbacks += { listeners.addParameterListener (param.tuningState.springTuningParams.intervalStiffness,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            DBG("intervalStiffness changed by user");
            params.tuningState.springTuner->intervalStiffnessChanged();
        })
    };

    circular_keyboard->addMyListener(this);
}

void TuningParametersView::resized()
{
    int knob_section_height = getKnobSectionHeight();
    int knob_y = getHeight() - knob_section_height;

    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int area_width = getWidth() - 2 * title_width;
    int envelope_height = knob_y - widget_margin;
    tuningtype_combo_box->setBounds(10,10, 100, 25);
    tuning_combo_box->setBounds(115,10,100, 25);
    fundamental_combo_box->setBounds(225, 10, 50,25);
    keyboard->setBounds(50, 200, 500, 100);
    circular_keyboard->setBounds(50, 50, 500, 100);

    //semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 50, getKnobSectionHeight() + 50 + getTextComponentHeight() * 2);
    semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 105, getKnobSectionHeight() + 10);
    adaptiveSection->setBounds(circular_keyboard->getRight() + 20, circular_keyboard->getY(), 500, 200);

    juce::Rectangle<int> outputKnobsArea = {50, semitoneSection->getBottom() + knob_section_height, 100, 100};
        //bounds.removeFromTop(knob_section_height);
    placeKnobsInArea(outputKnobsArea, _sliders, true);

    juce::Rectangle<int> springTuningBox = {adaptiveSection->getX(), semitoneSection->getY(), 500, 200};
    springTuningSection->setBounds(springTuningBox);

    lastNoteDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY(), 100, 30);
    lastFrequencyDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY() + 30, 100, 30);
    lastIntervalDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY() + 60, 100, 30);

    SynthSection::resized();
}

void TuningParametersView::keyboardSliderChanged(juce::String name) {

    if (name == "circular")
        params.tuningState.tuningSystem->setParameterValue(TuningSystem::Custom);
}
