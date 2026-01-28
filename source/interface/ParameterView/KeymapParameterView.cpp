//
// Created by Davis Polito on 5/27/25.
//
#include "KeymapParameterView.h"
#include "FullInterface.h"

KeymapParameterView::KeymapParameterView(
    KeymapProcessor &_proc,
    KeymapParams &kparams,
    juce::String name,
    OpenGlWrapper *open_gl) : proc(_proc), params(kparams), SynthSection("") {
    setName("keymap");
    setLookAndFeel(DefaultLookAndFeel::instance());
    setComponentID(name);

    setSkinOverride(Skin::kDirect);

    prepTitle = std::make_shared<PlainTextComponent>(getName(), getName());
    addOpenGlComponent(prepTitle);
    prepTitle->setJustification(juce::Justification::centredLeft);
    prepTitle->setFontType(PlainTextComponent::kTitle);
    prepTitle->setRotation(-90);

    velocityInLabel = std::make_shared<PlainTextComponent>("velocityIn", "VELOCITY  IN");
    addOpenGlComponent(velocityInLabel);
    velocityInLabel->setJustification(juce::Justification::centredBottom);
    //velocityInLabel->setFontType (PlainTextComponent::kTitle);

    velocityOutLabel = std::make_shared<PlainTextComponent>("velocityOut", "VELOCITY  OUT");
    addOpenGlComponent(velocityOutLabel);
    velocityOutLabel->setJustification(juce::Justification::centredLeft);
    velocityOutLabel->setRotation(-90);

    auto &listeners = proc.getState().getParameterListeners();

    keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(params.keyboard_state);
    keyboard_component_->postUInotesToEngine_ = false;
    addStateModulatedComponent(keyboard_component_.get());
    addAndMakeVisible(keyboard_component_.get());

    velocityMinMaxSlider = std::make_unique<OpenGL_VelocityMinMaxSlider>(&params.velocityMinMax, listeners, _proc.getState());
    velocityMinMaxSlider->setComponentID("keymap_velocity_min_max");
    addStateModulatedComponent(velocityMinMaxSlider.get());

    // knobs
    asymmetricalWarp_knob = std::make_unique<SynthSlider>(params.velocityCurve_asymWarp->paramID,
                                                          params.velocityCurve_asymWarp->getModParam());
    addSlider(asymmetricalWarp_knob.get());
    asymmetricalWarp_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    asymmetricalWarp_knob->setPopupPlacement(juce::BubbleComponent::below);
    asymmetricalWarp_knob->setShowPopupOnHover(true);
    asymmetricalWarp_knob_attach = std::make_unique<chowdsp::SliderAttachment>(
        params.velocityCurve_asymWarp, listeners, *asymmetricalWarp_knob, _proc.getState().undoManager);
    asymmetricalWarp_knob->addAttachment(asymmetricalWarp_knob_attach.get());

    symmetricalWarp_knob = std::make_unique<SynthSlider>(params.velocityCurve_symWarp->paramID,
                                                         params.velocityCurve_symWarp->getModParam());
    addSlider(symmetricalWarp_knob.get());
    symmetricalWarp_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    symmetricalWarp_knob->setPopupPlacement(juce::BubbleComponent::below);
    symmetricalWarp_knob->setShowPopupOnHover(true);
    symmetricalWarp_knob_attach = std::make_unique<chowdsp::SliderAttachment>(
        params.velocityCurve_symWarp, listeners, *symmetricalWarp_knob, _proc.getState().undoManager);
    symmetricalWarp_knob->addAttachment(symmetricalWarp_knob_attach.get());

    scale_knob = std::make_unique<SynthSlider>(params.velocityCurve_scale->paramID,
                                               params.velocityCurve_scale->getModParam());
    addSlider(scale_knob.get());
    scale_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    scale_knob->setPopupPlacement(juce::BubbleComponent::below);
    scale_knob->setShowPopupOnHover(true);
    scale_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_scale, listeners, *scale_knob,
                                                                    _proc.getState().undoManager);
    scale_knob->addAttachment(scale_knob_attach.get());

    offset_knob = std::make_unique<SynthSlider>(params.velocityCurve_offset->paramID,
                                                params.velocityCurve_offset->getModParam());
    addSlider(offset_knob.get());
    offset_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    offset_knob->setPopupPlacement(juce::BubbleComponent::below);
    offset_knob->setShowPopupOnHover(true);
    offset_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_offset, listeners,
                                                                     *offset_knob, _proc.getState().undoManager);
    offset_knob->addAttachment(offset_knob_attach.get());

    invert = std::make_unique<SynthButton>(params.velocityCurve_invert->paramID);
    invert_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.velocityCurve_invert, listeners, *invert,
                                                                    _proc.getState().undoManager);
    invert->setComponentID(params.velocityCurve_invert->paramID);
    addSynthButton(invert.get(), true, true);
    invert->setText("invert?");

    asymmetricalWarp_knob_label = std::make_shared<PlainTextComponent>(asymmetricalWarp_knob->getName(),
                                                                       params.velocityCurve_asymWarp->getName(20));
    addOpenGlComponent(asymmetricalWarp_knob_label);
    asymmetricalWarp_knob_label->setJustification(juce::Justification::centred);

    symmetricalWarp_knob_label = std::make_shared<PlainTextComponent>(symmetricalWarp_knob->getName(),
                                                                      params.velocityCurve_symWarp->getName(20));
    addOpenGlComponent(symmetricalWarp_knob_label);
    symmetricalWarp_knob_label->setJustification(juce::Justification::centred);

    scale_knob_label = std::make_shared<PlainTextComponent>(scale_knob->getName(),
                                                            params.velocityCurve_scale->getName(20));
    addOpenGlComponent(scale_knob_label);
    scale_knob_label->setJustification(juce::Justification::centred);

    offset_knob_label = std::make_shared<PlainTextComponent>(offset_knob->getName(),
                                                             params.velocityCurve_offset->getName(20));
    addOpenGlComponent(offset_knob_label);
    offset_knob_label->setJustification(juce::Justification::centred);

    velocityCurveControls = std::make_shared<OpenGL_LabeledBorder>("velocitycurveborder", "Velocity Curve Controls");
    addBorder(velocityCurveControls.get());
    drawCalls += {
        {
            listeners.addParameterListener(
                params.velocityCurve_invert,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    auto interface = findParentComponentOfClass<SynthGuiInterface>();
                    interface->getGui()->prep_popup->repaintPrepBackground();
                })
        },
        {
            listeners.addParameterListener(
                params.velocityCurve_offset,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    auto interface = findParentComponentOfClass<SynthGuiInterface>();
                    interface->getGui()->prep_popup->repaintPrepBackground();
                })
        },

        {
            listeners.addParameterListener(
                params.velocityCurve_scale,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    auto interface = findParentComponentOfClass<SynthGuiInterface>();
                    interface->getGui()->prep_popup->repaintPrepBackground();
                })
        },
        {
            listeners.addParameterListener(
                params.velocityCurve_asymWarp,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    auto interface = findParentComponentOfClass<SynthGuiInterface>();
                    interface->getGui()->prep_popup->repaintPrepBackground();
                })
        },
        {
            listeners.addParameterListener(
                params.velocityCurve_symWarp,
                chowdsp::ParameterListenerThread::MessageThread,
                [this] {
                    auto interface = findParentComponentOfClass<SynthGuiInterface>();
                    interface->getGui()->prep_popup->repaintPrepBackground();
                })
        },
    };

    startTimer(50);
}

KeymapParameterView::~KeymapParameterView() {
    stopTimer();
}

void KeymapParameterView::resized() {
    const auto kTitleWidth = findValue(Skin::kTitleWidth);
    juce::Rectangle<int> area(getLocalBounds());

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int labelsectionheight = findValue(Skin::kLabelHeight);
    auto knobLabelSize = findValue(Skin::kKnobLabelSizeSmall);

    asymmetricalWarp_knob_label->setTextSize(knobLabelSize);
    symmetricalWarp_knob_label->setTextSize(knobLabelSize);
    scale_knob_label->setTextSize(knobLabelSize);
    offset_knob_label->setTextSize(knobLabelSize);
    velocityInLabel->setTextSize(knobLabelSize);
    velocityOutLabel->setTextSize(knobLabelSize);

    juce::Rectangle<int> titleArea = getLocalBounds().removeFromLeft(getTitleWidth());
    prepTitle->setBounds(titleArea);
    prepTitle->setTextSize(findValue(Skin::kPrepTitleSize));

    area.removeFromLeft(kTitleWidth);
    area.removeFromRight(kTitleWidth);

    juce::Rectangle keySelectorRect = area.removeFromBottom(150);
    keyboard_component_->setBounds(keySelectorRect);

    area.removeFromBottom(smallpadding);

    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent && midi_selector_ == nullptr) {
        juce::AudioDeviceManager *device_manager = parent->getAudioDeviceManager();
        if (device_manager) {
            midi_selector_ = std::make_unique<OpenGlMidiSelector>(proc.v);
            addAndMakeVisible(midi_selector_.get());
            addOpenGlComponent(midi_selector_->getImageComponent());
            parent->getGui()->open_gl_.context.executeOnGLThread([this](juce::OpenGLContext &context) {
                SynthGuiInterface *parent = this->findParentComponentOfClass<SynthGuiInterface>();
                midi_selector_->getImageComponent()->init(parent->getGui()->open_gl_);
            }, false);
        }
    }

    if (midi_selector_) {
        juce::Rectangle midiSelectorRect = area.removeFromLeft(200);
        midiSelectorRect.removeFromTop(20);
        midiSelectorRect.removeFromLeft(largepadding);
        midi_selector_->setBounds(midiSelectorRect.removeFromTop(getHeight()));
        midi_selector_->redoImage();
        midi_selector_->setRowHeight(22);

        juce::Colour background = findColour(Skin::kPopupBackground, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::backgroundColourId, background);

        juce::Colour text = findColour(Skin::kBodyText, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ComboBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    }

    // velocity sliders, knobs and invert button
    juce::Rectangle velocityControlsRect = area.removeFromLeft(area.getWidth() * 0.5);
    velocityControlsRect.reduce(velocityControlsRect.getWidth() * 0.2, velocityControlsRect.getHeight() * 0.2);

    juce::Rectangle velocityKnobsRect = velocityControlsRect.removeFromTop(
        knobsectionheight * 2 + comboboxheight + largepadding * 5);
    velocityKnobsRect.removeFromBottom(largepadding);
    velocityCurveControls->setBounds(velocityKnobsRect);
    velocityKnobsRect.reduce(largepadding, largepadding);

    juce::Rectangle velocityKnobsRect_top = velocityKnobsRect.removeFromTop(knobsectionheight);
    asymmetricalWarp_knob->setBounds(velocityKnobsRect_top.removeFromLeft(velocityKnobsRect_top.getWidth() * 0.5));
    symmetricalWarp_knob->setBounds(velocityKnobsRect_top);
    velocityKnobsRect.removeFromTop(largepadding);
    juce::Rectangle velocityKnobsRect_bottom = velocityKnobsRect.removeFromTop(knobsectionheight);
    scale_knob->setBounds(velocityKnobsRect_bottom.removeFromLeft(velocityKnobsRect_bottom.getWidth() * 0.5));
    offset_knob->setBounds(velocityKnobsRect_bottom);

    velocityKnobsRect.removeFromTop(largepadding);

    juce::Rectangle invertButtonRect = velocityKnobsRect.removeFromTop(comboboxheight);
    invertButtonRect.reduce(invertButtonRect.getWidth() * 0.25, 0);
    invert->setBounds(invertButtonRect);

    velocityKnobsRect.removeFromTop(largepadding);

    velocityMinMaxSlider->setBounds(velocityControlsRect.removeFromTop(knobsectionheight));

    juce::Rectangle<int> label_rect1(asymmetricalWarp_knob->getX(), asymmetricalWarp_knob->getBottom() - 10,
                                     asymmetricalWarp_knob->getWidth(), labelsectionheight);
    asymmetricalWarp_knob_label->setBounds(label_rect1);
    juce::Rectangle<int> label_rect2(symmetricalWarp_knob->getX(), symmetricalWarp_knob->getBottom() - 10,
                                     symmetricalWarp_knob->getWidth(), labelsectionheight);
    symmetricalWarp_knob_label->setBounds(label_rect2);
    juce::Rectangle<int> label_rect3(scale_knob->getX(), scale_knob->getBottom() - 10, scale_knob->getWidth(),
                                     labelsectionheight);
    scale_knob_label->setBounds(label_rect3);
    juce::Rectangle<int> label_rect4(offset_knob->getX(), offset_knob->getBottom() - 10, offset_knob->getWidth(),
                                     labelsectionheight);
    offset_knob_label->setBounds(label_rect4);

    area.reduce(0, 40);
    juce::Rectangle<int> outLabelArea = area.removeFromLeft(labelsectionheight);
    outLabelArea.removeFromTop(bottomPadding + topPadding);
    //31 = bottomPadding + topPadding from drawVelocityCurve(); rewrite so not to hardwire, to fix label centering
    juce::Rectangle<int> inLabelArea = area.removeFromBottom(labelsectionheight);
    inLabelArea.removeFromLeft(leftPadding + rightPadding - labelsectionheight);
    //29 = leftPadding + rightPadding from drawVelocityCurve()
    velocityInLabel->setBounds(inLabelArea);
    velocityOutLabel->setBounds(outLabelArea);
    velocityCurveBox = area;
}

/**
 * called for drawing the velocity curve
 */
void KeymapParameterView::timerCallback(void) {
    velocityMinMaxSlider->setDisplayValue(params.velocityMinMax.lastVelocityParam);
    velocityMinMaxSlider->redoImage();

    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (interface != NULL)
        interface->getGui()->prep_popup->repaintPrepBackground();
}

void KeymapParameterView::drawVelocityCurve(juce::Graphics &g) {
    float asym_k = params.velocityCurve_asymWarp->getCurrentValue();
    float sym_k = params.velocityCurve_symWarp->getCurrentValue();
    float scale = params.velocityCurve_scale->getCurrentValue();
    float offset = params.velocityCurve_offset->getCurrentValue();
    bool velocityInvert = params.velocityCurve_invert->get();

    // int leftPadding = 25;        // space for "velocity out" label
    // int rightPadding = 4;       // space for the graph's right edge to fully display
    // int topPadding = 6;         // space for the dot and graph's top edge to fully display
    // int bottomPadding = 25;      // space for "velocity in" label
    // int leftAdditional = 40;    // additional space for number labels on left

    // wrapper rectangles for labels
    juce::Rectangle<int> graphArea(
        velocityCurveBox.getX() + leftPadding,
        velocityCurveBox.getY() + topPadding,
        velocityCurveBox.getWidth() - leftPadding - rightPadding,
        velocityCurveBox.getHeight() - bottomPadding - topPadding);

    // graph background setup
    g.setColour(juce::Colours::grey);
    g.drawRect(graphArea);
    int graphHeight = graphArea.getHeight();
    int graphWidth = graphArea.getWidth();
    int graphX = graphArea.getX();
    int graphY = graphArea.getY();

    // dashed midlines
    juce::Line<float> hMidLine(
        graphX,
        graphY + graphHeight / 2.,
        graphWidth + graphX,
        graphY + graphHeight / 2.);

    juce::Line<float> vMidLine(
        graphWidth / 2. + graphX,
        graphY,
        graphWidth / 2. + graphX,
        graphY + graphHeight);

    const float dashLengths[] = {5, 3};
    g.drawDashedLine(hMidLine, dashLengths, 2);
    g.drawDashedLine(vMidLine, dashLengths, 2);

    juce::Rectangle<int> bottomLabel(graphX, graphY + graphHeight, graphWidth, bottomPadding);
    juce::Rectangle<int> leftLabel(graphX - leftPadding - leftAdditional, graphY, leftPadding * 2, graphHeight);
    juce::Rectangle<int> leftNumbers(graphX - leftAdditional, graphY, leftAdditional, graphHeight);

    // graph label setup
    g.drawText("0", bottomLabel, juce::Justification::topLeft);
    g.drawText("0.5", bottomLabel, juce::Justification::centredTop);
    g.drawText("1", bottomLabel, juce::Justification::topRight);
    g.drawText("0.5", leftNumbers, juce::Justification::centredRight);
    g.drawText("1", leftNumbers, juce::Justification::topRight);
    g.setColour(juce::Colours::white);
    // g.drawText("Velocity In", bottomLabel, juce::Justification::centredBottom);

    // === START ROTATED BLOCK ===
    // Save the current, un-rotated graphics context state.
    g.saveState();

    // Calculate the center point of the rectangle where the text will be drawn.
    auto center = leftLabel.getCentre();

    // Apply the rotation (-90 degrees, or -pi/2 radians) using the center
    // of the target rectangle as the rotation pivot point.
    g.addTransform(juce::AffineTransform::rotation(-bitklavier::kPi / 2.0f, center.getX(), center.getY()));

    // Draw the text. Because we rotated the drawing context, the text will appear
    // rotated around its center point.
    // g.drawFittedText("Velocity Out", leftLabel, juce::Justification::centred, 2);

    // Restore the graphics context. This removes the rotation transform entirely,
    // ensuring that any code following this block draws horizontally again.
    g.restoreState();
    // === END ROTATED BLOCK ===

    // plotter
    g.setColour(juce::Colours::red);
    juce::Path plot;

    // go pixel by pixel, adding each point to plot
    for (int i = 0; i <= graphWidth; i++) {
        juce::Point<float> toAdd;
        if (!velocityInvert) {
            toAdd = juce::Point<float>(
                graphX + i,
                (graphY + graphHeight) - graphHeight * bitklavier::utils::dt_warpscale(
                    (float) i / graphWidth, asym_k, sym_k, scale, offset));
        } else {
            toAdd = juce::Point<float>(
                graphX + i,
                graphY + graphHeight * bitklavier::utils::dt_warpscale((float) i / graphWidth, asym_k, sym_k, scale,
                                                                       offset));
        }

        if (toAdd.getY() < graphY) toAdd.setY(graphY);
        if (toAdd.getY() > graphY + graphHeight) toAdd.setY(graphY + graphHeight);

        // the first point starts the subpath, whereas all subsequent points are merely
        // added to the subpath.
        if (i == 0) plot.startNewSubPath(toAdd);
        else plot.lineTo(toAdd);
    }

    g.strokePath(plot, juce::PathStrokeType(2.0));

    // add a dot to represent input velocity
    g.setColour(juce::Colours::goldenrod);
    int radius = 12;
    g.fillEllipse(
        graphX + params.invelocity * graphWidth - radius / 2,
        (graphY + graphHeight) - graphHeight * params.warpedvelocity - radius / 2,
        radius,
        radius);
}
