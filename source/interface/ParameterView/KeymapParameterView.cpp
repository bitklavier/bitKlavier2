//
// Created by Davis Polito on 5/27/25.
//
#include "KeymapParameterView.h"
#include "FullInterface.h"

KeymapParameterView::KeymapParameterView (
    KeymapProcessor& _proc,
    KeymapParams& kparams,
    juce::String name,
    OpenGlWrapper *open_gl) : proc(_proc), params(kparams), SynthSection("")
 {
     setName("keymap");
     setLookAndFeel(DefaultLookAndFeel::instance());
     setComponentID(name);

     auto& listeners = proc.getState().getParameterListeners();

     keyboard_component_ = std::make_unique<OpenGLKeymapKeyboardComponent>(params);
     addStateModulatedComponent(keyboard_component_.get());
     addAndMakeVisible(keyboard_component_.get());

     // knobs
     asymmetricalWarp_knob = std::make_unique<SynthSlider>(params.velocityCurve_asymWarp->paramID);
     addSlider(asymmetricalWarp_knob.get());
     asymmetricalWarp_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     asymmetricalWarp_knob->setPopupPlacement(juce::BubbleComponent::below);
     asymmetricalWarp_knob->setShowPopupOnHover(true);
     asymmetricalWarp_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_asymWarp, listeners, *asymmetricalWarp_knob, nullptr);
     asymmetricalWarp_knob->addAttachment(asymmetricalWarp_knob_attach.get());

     symmetricalWarp_knob = std::make_unique<SynthSlider>(params.velocityCurve_symWarp->paramID);
     addSlider(symmetricalWarp_knob.get());
     symmetricalWarp_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     symmetricalWarp_knob->setPopupPlacement(juce::BubbleComponent::below);
     symmetricalWarp_knob->setShowPopupOnHover(true);
     symmetricalWarp_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_symWarp, listeners, *symmetricalWarp_knob, nullptr);
     symmetricalWarp_knob->addAttachment(symmetricalWarp_knob_attach.get());

     scale_knob = std::make_unique<SynthSlider>(params.velocityCurve_scale->paramID);
     addSlider(scale_knob.get());
     scale_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     scale_knob->setPopupPlacement(juce::BubbleComponent::below);
     scale_knob->setShowPopupOnHover(true);
     scale_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_scale, listeners, *scale_knob, nullptr);
     scale_knob->addAttachment(scale_knob_attach.get());

     offset_knob = std::make_unique<SynthSlider>(params.velocityCurve_offset->paramID);
     addSlider(offset_knob.get());
     offset_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
     offset_knob->setPopupPlacement(juce::BubbleComponent::below);
     offset_knob->setShowPopupOnHover(true);
     offset_knob_attach = std::make_unique<chowdsp::SliderAttachment>(params.velocityCurve_offset, listeners, *offset_knob, nullptr);
     offset_knob->addAttachment(offset_knob_attach.get());

     invert = std::make_unique<SynthButton>(params.velocityCurve_invert->paramID);
     invert_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.velocityCurve_invert, listeners, *invert, nullptr);
     invert->setComponentID(params.velocityCurve_invert->paramID);
     addSynthButton(invert.get(), true, true);
     invert->setText("invert?");

     velocityCurveGraph = std::make_unique<VelocityCurveGraph>(kparams);
     addAndMakeVisible(velocityCurveGraph.get());

     startTimer(50);
     //velocityCurveGraph.updateVelocityList(km->getVelocities());
//     velocityCurveGraph.setAsym_k(1.);
//     velocityCurveGraph.setSym_k(1.);
//     velocityCurveGraph.setScale(1.);
//     velocityCurveGraph.setOffset(0.);
//     velocityCurveGraph.setVelocityInvert(false);
//     addAndMakeVisible(velocityCurveGraph);
//
}

KeymapParameterView::~KeymapParameterView(){}

void KeymapParameterView::resized()
{
    const auto kTitleWidth = findValue(Skin::kTitleWidth);
    juce::Rectangle<int> area (getLocalBounds());

    int smallpadding = findValue(Skin::kPadding);
    int largepadding = findValue(Skin::kLargePadding);
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int labelsectionheight = findValue(Skin::kLabelHeight);

    area.removeFromLeft(kTitleWidth);
    area.removeFromRight(kTitleWidth);

    juce::Rectangle keySelectorRect = area.removeFromBottom(150);
    keyboard_component_->setBounds(keySelectorRect);

    area.removeFromBottom(smallpadding);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent && midi_selector_ == nullptr) {
        juce::AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
        if (device_manager) {
            midi_selector_ = std::make_unique<OpenGlMidiSelector>
                    (proc.v);
            addAndMakeVisible(midi_selector_.get());
            addOpenGlComponent(midi_selector_->getImageComponent());
            parent->getGui()->open_gl_.context.executeOnGLThread([this](juce::OpenGLContext& context)
                                               {
                SynthGuiInterface* parent = this->findParentComponentOfClass<SynthGuiInterface>();
                midi_selector_->getImageComponent()->init(parent->getGui()->open_gl_); },false);
        }
    }

    if (midi_selector_) {
        juce::Rectangle midiSelectorRect = area.removeFromLeft(200);
        midiSelectorRect.removeFromTop(20);
        midi_selector_->setBounds(midiSelectorRect);
        //midi_selector_->setBounds(kTitleWidth, 10, 200, 200);
        midi_selector_->redoImage();
        midi_selector_->setRowHeight(22);

        juce::Colour background = findColour(Skin::kPopupBackground, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::backgroundColourId, background);

        juce::Colour text = findColour(Skin::kBodyText, true);
        setColorRecursively(midi_selector_.get(), juce::ListBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ComboBox::textColourId, text);
        setColorRecursively(midi_selector_.get(), juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    }

    // velocity knobs and invert button
    juce::Rectangle velocityKnobsRect = area.removeFromLeft(area.getWidth() * 0.5);
    velocityKnobsRect.reduce(velocityKnobsRect.getWidth() * 0.25, velocityKnobsRect.getHeight() * 0.25);
    juce::Rectangle invertButtonRect = velocityKnobsRect.removeFromBottom(comboboxheight);
    invertButtonRect.reduce(40, 0);
    invert->setBounds(invertButtonRect);

    velocityKnobsRect.removeFromBottom(20);
    juce::Rectangle velocityKnobsRect_top = velocityKnobsRect.removeFromTop(velocityKnobsRect.getHeight() * 0.5);
    asymmetricalWarp_knob->setBounds(velocityKnobsRect_top.removeFromLeft(velocityKnobsRect_top.getWidth() * 0.5));
    symmetricalWarp_knob->setBounds(velocityKnobsRect_top);
    scale_knob->setBounds(velocityKnobsRect.removeFromLeft(velocityKnobsRect.getWidth() * 0.5));
    offset_knob->setBounds(velocityKnobsRect);

    area.reduce(0, 40);
    velocityCurveBox = area;
    //velocityCurveGraph->setBounds(area);
}

void KeymapParameterView::redoImage()
{
    int mult = getPixelMultiple();
    int image_width = getWidth() * mult;
    int image_height = getHeight() * mult;
    //juce::Image background_image(juce::Image::ARGB,image_width, image_height, true);
    //juce::Graphics g (background_image);

    //paintKnobShadows(g);
    //sliderShadows.setOwnImage(background_image);

}

void KeymapParameterView::timerCallback(void)
{
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    interface->getGui()->prep_popup->repaintPrepBackground();
}

void KeymapParameterView::drawVelocityCurve(juce::Graphics &g)
{
    float asym_k            = params.velocityCurve_asymWarp->getCurrentValue();
    float sym_k             = params.velocityCurve_symWarp->getCurrentValue();
    float scale             = params.velocityCurve_scale->getCurrentValue();
    float offset            = params.velocityCurve_offset->getCurrentValue();
    bool velocityInvert     = params.velocityCurve_invert->get();

    int leftPadding = 40; // space for "velocity out" label
    int rightPadding = 4; // space for the graph's right edge to fully display
    int topPadding = 6; // space for the dot and graph's top edge to fully display
    int bottomPadding = 40; // space for "velocity in" label
    int leftAdditional = 40; // additional space for number labels on left

    // wrapper rectangles for labels
    juce::Rectangle<int> graphArea(
        velocityCurveBox.getX() + leftPadding,
        velocityCurveBox.getY() + topPadding,
        velocityCurveBox.getWidth() - leftPadding - rightPadding,
        velocityCurveBox.getHeight() - bottomPadding - topPadding);

    // graph background setup
    g.setColour (juce::Colours::grey);
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
    g.drawText("Velocity In", bottomLabel, juce::Justification::centredBottom);

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
    g.drawFittedText("Velocity Out", leftLabel, juce::Justification::centred, 2);

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
        if(!velocityInvert)
        {
            toAdd = juce::Point<float> (
                graphX + i,
                (graphY + graphHeight) - graphHeight * bitklavier::utils::dt_warpscale((float) i / graphWidth, asym_k, sym_k, scale, offset));
        }
        else
        {
            toAdd = juce::Point<float> (
                graphX + i,
                graphY + graphHeight * bitklavier::utils::dt_warpscale((float) i / graphWidth, asym_k, sym_k, scale, offset));
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
//    for (std::pair<int, float> element : velocities)
//    {
//        float velocity = element.second;
//        float warpscale = bitklavier::utils::dt_warpscale(velocity, asym_k, sym_k, scale, offset);
//        if (warpscale > 1) warpscale = 1;
//        if (warpscale < 0) warpscale = 0;
//
//        if (velocityInvert) {
//            g.fillEllipse(graphArea.getX() + velocity * graphWidth - radius / 2,
//                topPadding + graphHeight * warpscale - radius / 2,
//                radius, radius);
//        }
//        else {
//            g.fillEllipse(graphArea.getX() + velocity * graphWidth - radius / 2,
//                topPadding + graphHeight - graphHeight * warpscale - radius / 2,
//                radius, radius);
//        }
//    }
}