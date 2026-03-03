//
// Created by Dan Trueman on 11/5/24.
//

#include "TuningParametersView.h"
#include "SpringTuning/SpringTuning.h"
TuningParametersView::TuningParametersView(
    chowdsp::PluginState& pluginState,
    TuningParams& param,
    juce::String name,
    OpenGlWrapper *open_gl) : SynthSection(""), params(param)
{
    setName("tuning");
    setLookAndFeel(DefaultLookAndFeel::instance());
    setComponentID(name);

    setSkinOverride(Skin::kDirect);

    prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
    addOpenGlComponent(prepTitle);
    prepTitle->setTextSize (24.0f);
    prepTitle->setJustification(juce::Justification::centredLeft);
    prepTitle->setFontType (PlainTextComponent::kTitle);
    prepTitle->setRotation (-90);

    auto& listeners = pluginState.getParameterListeners();

    absolutekeyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState, true, true, "Individual Key Offsets");
    addStateModulatedComponent(absolutekeyboard.get());
    absolutekeyboard->setName("absolute");

    circular_keyboard = std::make_unique<OpenGLCircularKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState, true, "Octave Repeating");
    addStateModulatedComponent(circular_keyboard.get());
    circular_keyboard->setName("circular");

    semitoneSection = std::make_unique<SemitoneWidthSection>(name + "_semitone", params.tuningState.semitoneWidthParams, listeners, *this, pluginState);
    addSubSection(semitoneSection.get());

    adaptiveSection = std::make_unique<AdaptiveTuningSection>(name + "_adaptive", params.tuningState.adaptiveParams, listeners, *this, pluginState);
    addSubSection(adaptiveSection.get());

    springTuningSection = std::make_unique<SpringTuningSection>(name + "_spring", params.tuningState.springTuningParams, listeners, *this, pluginState);
    addSubSection(springTuningSection.get());

    offsetKnobSection = std::make_unique<OffsetKnobSection>(name + "_offset", params.tuningState.offsetKnobParam, listeners, *this, pluginState);
    addSubSection(offsetKnobSection.get());

    if (auto* tuningParams = dynamic_cast<TuningParams*>(&params)) {
        ///tuning systems
        tuning_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.tuningSystem->paramID.toStdString(), tuningParams->tuningState.tuningSystem->stateChanges.defaultState);
        setupTuningSystemMenu(tuning_combo_box);
        tuning_attachment= std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningSystem.get(), listeners, *tuning_combo_box, pluginState.undoManager);
        addComboBox(tuning_combo_box.get(), true, true);

        fundamental_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.fundamental->paramID.toStdString(), tuningParams->tuningState.fundamental->stateChanges.defaultState);
        fundamental_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.fundamental.get(), listeners, *fundamental_combo_box, pluginState.undoManager);
        addComboBox(fundamental_combo_box.get(), true, true);

        tuningtype_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningState.tuningType->paramID.toStdString(), tuningParams->tuningState.tuningType->stateChanges.defaultState);
        tuningtype_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningType.get(), listeners, *tuningtype_combo_box, pluginState.undoManager);
        addComboBox(tuningtype_combo_box.get(), true, true);
    }

    // Scala editing
    sclTextEditor = std::make_shared<OpenGlTextEditor>("SCLTXT");
    addOpenGlComponent(sclTextEditor->getImageComponent());
    addAndMakeVisible(sclTextEditor.get());
    sclTextEditor->setMultiLine(true);
    sclTextEditor->setReturnKeyStartsNewLine(true);
    sclTextEditor->setReadOnly(false);
    sclTextEditor->setScrollbarsShown(true);
    sclTextEditor->setCaretVisible(true);
    sclTextEditor->setPopupMenuEnabled(true);
    sclTextEditor->setText("SCALA STUFF");
    sclTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
    sclTextEditor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    sclTextEditor->setColour(juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha(0.5f));
    sclTextEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::white);

    kbmTextEditor = std::make_shared<OpenGlTextEditor>("KBMTXT");
    addOpenGlComponent(kbmTextEditor->getImageComponent());
    addAndMakeVisible(kbmTextEditor.get());
    kbmTextEditor->setMultiLine(true);
    kbmTextEditor->setReturnKeyStartsNewLine(true);
    kbmTextEditor->setReadOnly(false);
    kbmTextEditor->setScrollbarsShown(true);
    kbmTextEditor->setCaretVisible(true);
    kbmTextEditor->setPopupMenuEnabled(true);
    kbmTextEditor->setText("KBM STUFF");
    kbmTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
    kbmTextEditor->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    kbmTextEditor->setColour(juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha(0.5f));
    kbmTextEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::white);

    sclTextEditor->getImageComponent()->setInterceptsMouseClicks(false, false);
    kbmTextEditor->getImageComponent()->setInterceptsMouseClicks(false, false);

    sclTextEditor->addListener(this);
    kbmTextEditor->addListener(this);

    /*
     * todo: this is problematic, in that it overwrites modded values
     *          - if you trigger a mod that changes the circular tuning, and then open the Tuning prep, this will overwrite those
     *          - probably should use defaultState or modulationState if they exist?
     *
     * ok, so somewhat complicated:
     * - on initialization of TuningProcessor, defaultState and modulatedState should be the same, and should exist
     *      - when created from scratch, both should be set equal to the system and fundamental, like using setOffsetsFromTuningSystem
     *      - when a gallery is opened, both should be set from the gallery, where both should be saved
     * - if the tuning system is anything other than "custom", we ignore what the circularTuningOffset_custom values are
     */
    // setOffsetsFromTuningSystem(
    //             params.tuningState.tuningSystem->get(),
    //             params.tuningState.fundamental->getIndex(),
    //             this->params.tuningState.circularTuningOffset,
    //             this->params.tuningState.circularTuningOffset_custom);

    //circular_keyboard->updateValuesFromString (params.tuningState.stateChanges.defaultState.getProperty(IDs::circularTuning).toString(), true);

    /*
     * display relevant subsets of the UI depending on selected TuningType
     */
    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.tuningType,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]() {
             showCurrentTuningType();
        })
    };
    
    tuningCallbacks += {listeners.addParameterListener(
        params.tuningState.tuningSystem,
        chowdsp::ParameterListenerThread::MessageThread,
        [this]()
        {
            showCurrentTuningType();
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
            //doing this twice, because of the listener in TuningProcessor;
            //  ok, but might want to optimize at some point, having the processor notify here that the fundamental
            //  has been changed, so redoImage is called after.
            //  some with tuningSystem above
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

    // to listen for user changes to circular tuning, which will trigger change to 'custom' tuning system
    circular_keyboard->addMyListener(this);

    showCurrentTuningType();
    startTimer(50);
}

void TuningParametersView::showStaticTuning(bool show)
{
    tuning_combo_box->setVisible(show);
    fundamental_combo_box->setVisible(show);
    absolutekeyboard->setVisible(show);
    circular_keyboard->setVisible(show);
    semitoneSection->setVisible(show);
    offsetKnobSection->setVisible(show);
}

void TuningParametersView::showAdaptiveTuning(bool show)
{
    adaptiveSection->setVisible(show);
}

void TuningParametersView::showSpringTuning(bool show)
{
    springTuningSection->setVisible (show);
    params.tuningState.springTuner->setRate (params.tuningState.springTuningParams.rate->getCurrentValue(), show);
}

void TuningParametersView::showScalaKbm(bool show)
{
    DBG("TuningParametersView::showScalaKbm, show = " + juce::String((int)show));
    sclTextEditor->setVisible(show);
    kbmTextEditor->setVisible(show);
    if (show)
    {
        sclTextEditor->toFront(true);
        kbmTextEditor->toFront(true);
        sclTextEditor->getImageComponent()->toFront(true);
        kbmTextEditor->getImageComponent()->toFront(true);
        sclTextEditor->getImageComponent()->setInterceptsMouseClicks(false, false);
        kbmTextEditor->getImageComponent()->setInterceptsMouseClicks(false, false);
        sclTextEditor->redoImage();
        kbmTextEditor->redoImage();
    }

    // Ensure they are not covered by other components
    if (show)
    {
        for (auto* child : getChildren())
        {
            if (child != sclTextEditor.get() && child != kbmTextEditor.get() &&
                child != sclTextEditor->getImageComponent().get() && child != kbmTextEditor->getImageComponent().get())
            {
                if (child->getBounds().intersects(sclTextEditor->getBounds()) ||
                    child->getBounds().intersects(kbmTextEditor->getBounds()))
                {
                    child->toBack();
                }
            }
        }
    }
}

void TuningParametersView::showCurrentTuningType()
{
    // always show static tuning
    showStaticTuning(true);
    showAdaptiveTuning(params.tuningState.tuningType->get() == TuningType::Adaptive || params.tuningState.tuningType->get() == TuningType::Adaptive_Anchored);
    showSpringTuning(params.tuningState.tuningType->get() == TuningType::Spring_Tuning);
    showScalaKbm(params.tuningState.tuningType->get() == TuningType::Scala_KBM);

    resized();

    /*
     * important call to get the backgrounds to redraw properly!
     */
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (interface != nullptr)
    {
        if (auto* gui = interface->getGui())
        {
            if (gui->prep_popup != nullptr)
                gui->prep_popup->repaintPrepBackground();
        }
    }
}

/**
 * listen to old school bK circular keyboard slider for user changes to tuning
 *      - when the user changes a value on that keyboard, we automatically switch to 'custom' tuning
 */
void TuningParametersView::keyboardSliderChanged(juce::String name)
{
    if (name == "circular")
        params.tuningState.tuningSystem->setParameterValue(TuningSystem::Custom);
}

/**
 * todo: i'm not sure if this is the best way to get the spiralView to redraw, with drawSpiral() called in paintBackground()
 *          - but it does seem to work!
 */
void TuningParametersView::timerCallback(void)
{
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (interface != nullptr)
    {
        if (auto* gui = interface->getGui())
        {
            if (gui->prep_popup != nullptr)
                gui->prep_popup->repaintPrepBackground();
        }
    }

    // Refresh keyboards if tuning offsets were updated by the audio thread
    if (params.tuningState.absoluteTuningDirty.exchange(false, std::memory_order_acq_rel))
    {
        if (absolutekeyboard)
        {
            absolutekeyboard->redoImage();
            absolutekeyboard->repaint();
        }
    }

    if (params.tuningState.circularTuningDirty.exchange(false, std::memory_order_acq_rel))
    {
        if (circular_keyboard)
        {
            circular_keyboard->redoImage();
            circular_keyboard->repaint();
        }
    }
}

/**
 * function for drawing all the currently active notes, with intervals between them
 * @param spiralBox
 *
 * todo: rewrite this as an OpenGL shader?
 */
void TuningParametersView::drawSpiral(juce::Graphics& g)
{
    float midi, scalex, radians, cx, cy;
    float tmidi, tscalex, tradians, tcx, tcy;
    float centerx = spiralBox.getWidth() * 0.5f + spiralBox.getX();
    float centery = spiralBox.getCentreY();

    float radius_scale = 0.25;
    float radius = juce::jmin(spiralBox.getHeight() * radius_scale, spiralBox.getWidth() * radius_scale);
    float dimc_scale = 0.05;
    float dimc = juce::jmin(spiralBox.getHeight() * dimc_scale, spiralBox.getWidth() * dimc_scale);

    float midiScale, tmidiScale;

    std::vector<float> currentFreqs;

    /**
     * draw default/anchor/tether locations (ET by default)
     * todo: for spring tuning, update to draw locations of anchor springs
     *          - also possibly scale thickness of "springs" from spring settings
     */

    // for (int midi = 20; midi < 109; midi++)
    // {
    //     midiScale = midi / 60.;
    //     scalex = ((midi - 60.0f) / 12.0f);
    //     radians = scalex * Utilities::twopi - Utilities::pi * 0.5;
    //     cx = centerx + cosf(radians) * radius * midiScale - dimc * 0.5f;
    //     cy = centery + sinf(radians) * radius * midiScale - dimc * 0.5f;
    //     g.setColour (juce::Colours::dimgrey);
    //     g.setOpacity(0.25);
    //     g.fillEllipse(cx, cy, dimc, dimc);
    // }

    for (int tnote = 20; tnote < 109; tnote++)
    {
        double tetherFrequency = params.tuningState.springTuner->getTetherFrequency(tnote, params.tuningState.getGlobalTuningReference()) * intervalToRatio(params.tuningState.getOverallOffset());
        tmidi = ftom(tetherFrequency, params.tuningState.getGlobalTuningReference());
        tmidiScale = tmidi / 60.;
        tscalex = ((tmidi - 60.0f) / 12.0f);
        tradians = tscalex * Utilities::twopi - Utilities::pi * 0.5;
        tcx = centerx + cosf(tradians) * radius * tmidiScale - dimc * 0.5f;
        tcy = centery + sinf(tradians) * radius * tmidiScale - dimc * 0.5f;

        g.setColour (juce::Colours::dimgrey);
        g.setOpacity(0.25);
        g.fillEllipse(tcx, tcy, dimc, dimc);
    }

    /**
     * draw sounding notes
     */
    auto& allnotes = params.tuningState.spiralNotes;
    for (auto& p : allnotes)
    {
        const float f = p.load();
        if (!(f > 0.0f) || !std::isfinite(f))
            continue;

        const float ref = params.tuningState.getGlobalTuningReference();
        if (!(ref > 0.0f) || !std::isfinite(ref))
            continue;

        midi = ftom(f, ref);
        if (!std::isfinite(midi))
            continue;
        currentFreqs.emplace_back(midi); // keep track of all the current notes, for drawing lines between

        midiScale = midi / 60.;
        scalex = ((midi - 60.0f) / 12.0f);
        radians = scalex * Utilities::twopi - Utilities::pi * 0.5;
        cx = centerx + cosf(radians) * radius * midiScale - dimc * 0.5f;
        cy = centery + sinf(radians) * radius * midiScale - dimc * 0.5f;
        if (!std::isfinite(cx) || !std::isfinite(cy))
            continue;

        float hue = fmod(midi, 12.) / 12.;
        juce::Colour colour (hue, 0.5f, 0.5f, 0.9f);
        g.setColour (colour);
        g.setOpacity(0.75);
        g.fillEllipse(cx, cy, dimc, dimc);

        const float centsF = (midi - std::round(midi)) * 100.0f; // might need to update to show actual offset from anchor locations
        if (std::isfinite(centsF))
        {
            g.setColour(juce::Colours::white);
            g.setFont(14.0f);
            g.drawText(juce::String((int) std::round(centsF)), cx - dimc * 0.25f, cy + dimc * 0.25f, dimc * 1.5f, dimc * 0.5f, juce::Justification::centred);
        }
    }

    /**
     * draw lines between notes
     * todo: could optimize this a bit, since we are recalculating center points and so on....
     */
    if(currentFreqs.size() < 2) return;
    for (int startFreqIndex = 0; startFreqIndex < currentFreqs.size() - 1; startFreqIndex++)
    {
        for (int endFreqIndex = startFreqIndex + 1; endFreqIndex < currentFreqs.size(); endFreqIndex++)
        {
            //draw line between startFreq and endFreq

            // coordinates for first freq
            midi = currentFreqs[startFreqIndex];
            scalex = ((midi - 60.0f) / 12.0f);
            midiScale = midi / 60.;
            radians = scalex * Utilities::twopi - Utilities::pi * 0.5;
            float cxa = centerx + cosf(radians) * radius * midiScale;
            float cya = centery + sinf(radians) * radius * midiScale;

            // save for later
            float midiSave = midi;

            // coordinates for second freq
            midi = currentFreqs[endFreqIndex];
            scalex = ((midi - 60.0f) / 12.0f);
            midiScale = midi / 60.;
            radians = scalex * Utilities::twopi - Utilities::pi * 0.5;
            float cxb = centerx + cosf(radians) * radius * midiScale;
            float cyb = centery + sinf(radians) * radius * midiScale;

            if (!std::isfinite(cxa) || !std::isfinite(cya) || !std::isfinite(cxb) || !std::isfinite(cyb))
                continue;

            float hue = fmod((midi + midiSave)/2., 12.) / 12.;
            juce::Colour colour (hue, 0.5f, 0.5f, 0.25f);
            g.setColour(colour);
            g.drawLine(cxa, cya, cxb, cyb, 5);

            int h = 10, w = 35;
            float midX = (cxa + cxb) * 0.5f; //+ xoff;
            float midY = (cya + cyb) * 0.5f; //+ yoff;

            g.setColour(juce::Colours::ghostwhite);
            g.setFont(12.0f);
            g.setOpacity(0.7);
            const float centsDelta = 100.0f * (midi - midiSave);
            if (std::isfinite(centsDelta))
                g.drawText(juce::String((int) std::round(centsDelta)), midX - dimc * 0.25f, midY, w, h, juce::Justification::topLeft);

        }
    }
}

void TuningParametersView::textEditorReturnKeyPressed(juce::TextEditor& editor)
{
    if (&editor == sclTextEditor.get())
    {
        //params.tuningState.loadScalaFile(editor.getText().toStdString());
        DBG("Scala text editor return key: " << editor.getText());
    }
    else if (&editor == kbmTextEditor.get())
    {
        //params.tuningState.loadKBMFile(editor.getText().toStdString());
        DBG("KBM text editor return key: " << editor.getText());
    }
}

void TuningParametersView::textEditorFocusLost(juce::TextEditor& editor)
{
    textEditorReturnKeyPressed(editor);
}

void TuningParametersView::textEditorEscapePressed(juce::TextEditor& editor)
{
    editor.moveCaretToTop(false);
}

void TuningParametersView::resized()
{
    FullInterface *parent = findParentComponentOfClass<FullInterface>();
    if (parent)
        parent->hideSoundsetSelector();

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

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(title_width);
    prepTitle->setBounds(titleArea);

    area.removeFromLeft(title_width);
    juce::Rectangle leftHalf = area.removeFromLeft(area.getWidth() * 0.5);
    spiralBox = area;
    leftHalf.reduce(largepadding, largepadding);
//    juce::Rectangle areaSpring = leftHalf;
//    juce::Rectangle areaAdaptive = leftHalf;

    area.reduce(largepadding, largepadding);
    juce::Rectangle lastValsDisplay = area.removeFromBottom(labelsectionheight + smallpadding);
    lastNoteDisplay->setBounds(lastValsDisplay.removeFromLeft(lastValsDisplay.getWidth() / 3.));
    lastFrequencyDisplay->setBounds(lastValsDisplay.removeFromLeft(lastValsDisplay.getWidth() / 2.));
    lastIntervalDisplay->setBounds(lastValsDisplay);

    juce::Rectangle topMenuBox = leftHalf.removeFromTop(comboboxheight);
    int menuWidthFactor = leftHalf.getWidth() / 7.;
    tuningtype_combo_box->setBounds(topMenuBox.removeFromLeft(menuWidthFactor * 2.));
    tuning_combo_box->setBounds(topMenuBox.removeFromLeft(menuWidthFactor * 3.));
    fundamental_combo_box->setBounds((topMenuBox));

    leftHalf.removeFromTop(largepadding);

    juce::Rectangle absoluteKeyboardBox = leftHalf.removeFromTop(semitoneBoxTargetHeight * 1.5);
    absolutekeyboard->setBounds(absoluteKeyboardBox);

    leftHalf.removeFromTop(smallpadding);

    juce::Rectangle circularKeyboardBox = leftHalf.removeFromTop(semitoneBoxTargetHeight * 1.5);
    semitoneSection->setBounds(circularKeyboardBox.removeFromRight(semitoneBoxTargetWidth));
    offsetKnobSection->setBounds(circularKeyboardBox.removeFromLeft(knobsectionheight + 30));
    circular_keyboard->setBounds(circularKeyboardBox);

    // these two can be on top of one another, since only one or the other is visible at any particular time
    juce::Rectangle leftHalfSave = leftHalf;
    juce::Rectangle scalaBox = leftHalf;
    adaptiveSection->setBounds(leftHalf.removeFromBottom(165));
    springTuningSection->setBounds(leftHalfSave.removeFromBottom(275));

    sclTextEditor->setBounds(scalaBox.removeFromLeft(scalaBox.getWidth() / 2.));
    kbmTextEditor->setBounds(scalaBox);

    sclTextEditor->getImageComponent()->setBounds(sclTextEditor->getBounds());
    kbmTextEditor->getImageComponent()->setBounds(kbmTextEditor->getBounds());

    SynthSection::resized();
}
