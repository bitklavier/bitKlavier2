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

    if (auto* tuningParams = dynamic_cast<TuningParams*>(&params)) {
        ///tuning systems
        auto index = tuningParams->tuningState.tuningSystem->getIndex();
        tuning_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.tuningSystem->paramID.toStdString());
        tuning_attachment= std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningSystem.get(), listeners,*tuning_combo_box, nullptr);
        addAndMakeVisible(tuning_combo_box.get());
        addOpenGlComponent(tuning_combo_box->getImageComponent());

        // clear the default menu so we can make submenus
        tuning_combo_box->clear(juce::sendNotificationSync);
        juce::OwnedArray<juce::PopupMenu> submenus;
        submenus.add(new juce::PopupMenu());
        submenus.add(new juce::PopupMenu());
        int i = 0;
        for (auto choice : tuningParams->tuningState.tuningSystem->choices) {
            if (i <= 6)
                tuning_combo_box->addItem(choice,i+1);
            if (i>6 && i <33) {
                submenus.getUnchecked(0)->addItem(i+1,choice);
            }
            else if (i>=33) {
                submenus.getUnchecked(1)->addItem(i+1,choice);
            }
            i++;
        }

        tuning_combo_box->addSeparator();
        auto* pop_up = tuning_combo_box->getRootMenu();
        pop_up->addSubMenu("Historical",*submenus.getUnchecked(0));
        pop_up->addSubMenu("Various",*submenus.getUnchecked(1));
        tuning_combo_box->setSelectedItemIndex(index,juce::sendNotificationSync);

        fundamental_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.fundamental->paramID.toStdString());
        fundamental_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.fundamental.get(), listeners,*fundamental_combo_box, nullptr);
        addAndMakeVisible(fundamental_combo_box.get());
        addOpenGlComponent(fundamental_combo_box->getImageComponent());

        adaptive_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.tuningType->paramID.toStdString());
        adaptive_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningType.get(), listeners,*adaptive_combo_box, nullptr);
        addAndMakeVisible(adaptive_combo_box.get());
        addOpenGlComponent(adaptive_combo_box->getImageComponent());
    }

    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.tuningSystem,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            if (!params.tuningState.setFromAudioThread) {
                TuningSystem t = params.tuningState.tuningSystem->get();

                //this->params.tuningState.circularTuningOffset = tuningMap[t].second;
                auto it = std::find_if(tuningMap.begin(), tuningMap.end(),
                    [t](const auto& pair) {
                        return pair.first == t;
                    });
                if (it->first == TuningSystem::Custom) {

                }
                else if (it != tuningMap.end()) {
                    const auto& tuning = it->second;
                    const auto tuningArray = TuningState::rotateValuesByFundamental(tuning, params.tuningState.fundamental->getIndex());
                    int index  = 0;
                    for (const auto val :tuningArray) {
                        this->params.tuningState.circularTuningOffset[index] = val * 100;
                        DBG("new tuning " + juce::String(index) + " " + juce::String(this->params.tuningState.circularTuningOffset[index]));
                        index++;
                    }
                }
            }

            params.tuningState.setFromAudioThread = false;
            circular_keyboard->redoImage();
        }
        )
    };

    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.fundamental,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
            params.tuningState.setFundamental(params.tuningState.fundamental->getIndex());
            circular_keyboard->redoImage();
        }
        )};

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
    lastNoteDisplay->setJustification(juce::Justification::left);

    /*
     * this will hear changes to tuningState.lastNote, which are triggered by the last tuned note in the synth (through synthState, called in DirectProcessor, etc...)
     */
    tuningCallbacks += { listeners.addParameterListener (param.tuningState.lastNote,
        chowdsp::ParameterListenerThread::MessageThread,
        [this] {
            lastNoteDisplay->setText ("Last Pitch = " + this->params.tuningState.lastNote->getCurrentValueAsText());
            lastFrequencyDisplay->setText ("Last Frequency = " + juce::String(mtof(this->params.tuningState.lastNote->getCurrentValue()),2));
            lastIntervalDisplay->setText ("Last Interval = " + juce::String(this->params.tuningState.lastIntervalCents, 2));
        })};

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
    adaptive_combo_box->setBounds(10,10, 100, 25);
    tuning_combo_box->setBounds(115,10,100, 25);
    fundamental_combo_box->setBounds(225, 10, 50,25);
    keyboard->setBounds(50, 200, 500, 100);
    circular_keyboard->setBounds(50, 50, 500, 100);

    //semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 50, getKnobSectionHeight() + 50 + getTextComponentHeight() * 2);
    semitoneSection->setBounds(50, 350, getKnobSectionHeight() + 105, getKnobSectionHeight() + 10);

    juce::Rectangle<int> outputKnobsArea = {50, semitoneSection->getBottom() + knob_section_height, 100, 100};
        //bounds.removeFromTop(knob_section_height);
    placeKnobsInArea(outputKnobsArea, _sliders, true);

    lastNoteDisplay->setBounds(semitoneSection->getRight() + 50, semitoneSection->getY(), 200, 200);
    lastFrequencyDisplay->setBounds(lastNoteDisplay->getRight() + 20, semitoneSection->getY(), 200, 200);
    lastIntervalDisplay->setBounds(lastFrequencyDisplay->getRight() + 20, semitoneSection->getY(), 200, 200);

    SynthSection::resized();
}

void TuningParametersView::keyboardSliderChanged(juce::String name) {

    if (name == "circular")
        params.tuningState.tuningSystem->setParameterValue(TuningSystem::Custom);
}
