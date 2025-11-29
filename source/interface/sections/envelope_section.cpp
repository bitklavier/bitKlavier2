/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "envelope_section.h"

#include "skin.h"
#include "envelope_editor.h"
#include "fonts.h"
#include "paths.h"

#include "synth_slider.h"

DragMagnifyingGlass::DragMagnifyingGlass() : OpenGlShapeButton("Magnifying Glass") {
  setShape(Paths::magnifyingGlass());
}

void DragMagnifyingGlass::mouseDown(const juce::MouseEvent& e) {
  OpenGlShapeButton::mouseDown(e);
  last_position_ = e.position;

  juce::MouseInputSource source = e.source;
  if (source.isMouse() && source.canDoUnboundedMovement()) {
    source.hideCursor();
    source.enableUnboundedMouseMovement(true);
    mouse_down_position_ = e.getScreenPosition();
  }
}

void DragMagnifyingGlass::mouseUp(const juce::MouseEvent& e) {
  OpenGlShapeButton::mouseUp(e);

  juce::MouseInputSource source = e.source;
  if (source.isMouse() && source.canDoUnboundedMovement()) {
    source.showMouseCursor(juce::MouseCursor::NormalCursor);
    source.enableUnboundedMouseMovement(false);
    source.setScreenPosition(mouse_down_position_.toFloat());
  }
}

void DragMagnifyingGlass::mouseDrag(const juce::MouseEvent& e) {
  juce::Point<float> position = e.position;
  juce::Point<float> delta_position = position - last_position_;
  last_position_ = position;

  for (Listener* listener : listeners_)
    listener->magnifyDragged(delta_position);

  OpenGlShapeButton::mouseDrag(e);
}

void DragMagnifyingGlass::mouseDoubleClick(const juce::MouseEvent& e) {
  for (Listener* listener : listeners_)
    listener->magnifyDoubleClicked();
  
  OpenGlShapeButton::mouseDoubleClick(e);
}

EnvelopeSection::EnvelopeSection( EnvParams &params, chowdsp::ParameterListeners& listeners, SynthSection &parent) : SynthSection("envsection"), _params(params)
{

    setComponentID(parent.getComponentID());

    delay_ = std::make_unique<SynthSlider>("delay", params.delayParam->getModParam());
    addSlider(delay_.get());
    delay_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delay_->setPopupPlacement(juce::BubbleComponent::below);
    delay_->parentHierarchyChanged();
    delay_->setVisible(false);

    attack_ = std::make_unique<SynthSlider>(params.attackParam->getName(20), params.attackParam->getModParam());
    addSlider(attack_.get());
    attack_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attack_->setPopupPlacement(juce::BubbleComponent::below);
    attack_->parentHierarchyChanged();
    attack_->setShowPopupOnHover(true);
    attack_->setValue(params.attackParam->getDefaultValue());

    attack_power_ = std::make_unique<SynthSlider>( "attack_power", juce::ValueTree{});
    addSlider(attack_power_.get());
    attack_power_->setRange(-10., 10.); // don't need to do this in the original Vital version, not sure why we need this here
    attack_power_->setVisible(false);

    hold_ = std::make_unique<SynthSlider>("hold",juce::ValueTree{});
    addSlider(hold_.get());
    hold_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    hold_->setPopupPlacement(juce::BubbleComponent::below);
    hold_->parentHierarchyChanged();
    hold_->setVisible(false);

    decay_ = std::make_unique<SynthSlider>( params.decayParam->getName(20), params.decayParam->getModParam());
    addSlider(decay_.get());
    decay_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decay_->setPopupPlacement(juce::BubbleComponent::below);
    decay_->setShowPopupOnHover(true);
    decay_->setValue(params.decayParam->getDefaultValue());

    decay_power_ = std::make_unique<SynthSlider>( "decay_power",juce::ValueTree{});
    addSlider(decay_power_.get());
    decay_power_->setRange(-10., 10.);
    decay_power_->setVisible(false);

    release_ = std::make_unique<SynthSlider>(params.releaseParam->getName(20),params.releaseParam->getModParam());
    addSlider(release_.get());
    release_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    release_->setPopupPlacement(juce::BubbleComponent::below);
    release_->setShowPopupOnHover(true);
    release_->setValue(params.releaseParam->getDefaultValue());

    release_power_ = std::make_unique<SynthSlider>( "release_power",juce::ValueTree{});
    addSlider(release_power_.get());
    release_power_->setRange(-10., 10.);
    release_power_->setVisible(false);

    sustain_ = std::make_unique<SynthSlider>(params.sustainParam->getName(20), params.sustainParam->getModParam());
    addSlider(sustain_.get());
    sustain_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sustain_->setPopupPlacement(juce::BubbleComponent::below);
    sustain_->setShowPopupOnHover(true);
    sustain_->setValue(params.sustainParam->getDefaultValue());

    envelope_ = std::make_shared<EnvelopeEditor>("");
    addOpenGlComponent(envelope_);
    //envelope_->setName(value_prepend);
    envelope_->setDelaySlider(delay_.get());
    envelope_->setAttackSlider(attack_.get());
    envelope_->setAttackPowerSlider(attack_power_.get());
    envelope_->setHoldSlider(hold_.get());
    envelope_->setDecaySlider(decay_.get());
    envelope_->setDecayPowerSlider(decay_power_.get());
    envelope_->setSustainSlider(sustain_.get());
    envelope_->setReleaseSlider(release_.get());
    envelope_->setReleasePowerSlider(release_power_.get());
    envelope_->resetEnvelopeLine(-1);
    envelope_->addMouseListener(this, true);

    drag_magnifying_glass_ = std::make_unique<DragMagnifyingGlass>();
    drag_magnifying_glass_->addListener(this);
    //drag_magnifying_glass_->setName(value_prepend + "_magnify");
    addAndMakeVisible(drag_magnifying_glass_.get());
    addOpenGlComponent(drag_magnifying_glass_->getGlComponent());

    delay_attachment = std::make_unique<chowdsp::SliderAttachment>(params.delayParam, listeners, *delay_, nullptr);
    attack_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attackParam, listeners, *attack_, nullptr);
    attackPower_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attackPowerParam, listeners, *attack_power_, nullptr);
    hold_attachment = std::make_unique<chowdsp::SliderAttachment>(params.holdParam, listeners, *hold_, nullptr);
    decay_attachment = std::make_unique<chowdsp::SliderAttachment>(params.decayParam, listeners, *decay_, nullptr);
    decayPower_attachment = std::make_unique<chowdsp::SliderAttachment>(params.decayPowerParam, listeners, *decay_power_, nullptr);
    sustain_attachment = std::make_unique<chowdsp::SliderAttachment>(params.sustainParam, listeners, *sustain_, nullptr);
    release_attachment = std::make_unique<chowdsp::SliderAttachment>(params.releaseParam, listeners, *release_, nullptr);
    releasePower_attachment = std::make_unique<chowdsp::SliderAttachment>(params.releasePowerParam, listeners, *release_power_, nullptr);

    // for modulations
    attack_->addAttachment(attack_attachment.get());
    decay_->addAttachment(decay_attachment.get());
    sustain_->addAttachment(sustain_attachment.get());
    release_->addAttachment(release_attachment.get());

    attack_label = std::make_shared<PlainTextComponent>(attack_->getName(), attack_->getName());
    addOpenGlComponent(attack_label);
    attack_label->setTextSize (12.0f);
    attack_label->setJustification(juce::Justification::centred);

    decay_label = std::make_shared<PlainTextComponent>(decay_->getName(), decay_->getName());
    addOpenGlComponent(decay_label);
    decay_label->setTextSize (12.0f);
    decay_label->setJustification(juce::Justification::centred);

    sustain_label = std::make_shared<PlainTextComponent>(sustain_->getName(), sustain_->getName());
    addOpenGlComponent(sustain_label);
    sustain_label->setTextSize (12.0f);
    sustain_label->setJustification(juce::Justification::centred);

    release_label = std::make_shared<PlainTextComponent>(release_->getName(), release_->getName());
    addOpenGlComponent(release_label);
    release_label->setTextSize (12.0f);
    release_label->setJustification(juce::Justification::centred);

    envelopeSectionBorder.setName("envelope border");
    envelopeSectionBorder.setText(_params.idPrepend+" Envelope");
    envelopeSectionBorder.setTextLabelPosition(juce::Justification::centred);
    addAndMakeVisible(envelopeSectionBorder);
//    setLookAndFeel(DefaultLookAndFeel::instance());
}

EnvelopeSection::~EnvelopeSection() { }

void EnvelopeSection::mouseUp(const juce::MouseEvent& e) {
    SynthSection::mouseUp(e);
    notifyParentOfValueChange();
}

void EnvelopeSection::paintBackground(juce::Graphics& g) {
  setLabelFont(g);
  // drawLabelForComponent(g, TRANS("DELAY"), delay_.get());
  // drawLabelForComponent(g, TRANS("Attack"), attack_.get());
  // drawLabelForComponent(g, TRANS("HOLD"), hold_.get());
  // drawLabelForComponent(g, TRANS("Decay"), decay_.get());
  // drawLabelForComponent(g, TRANS("Sustain"), sustain_.get());
  // drawLabelForComponent(g, TRANS("Release"), release_.get());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);

  envelopeSectionBorder.paint(g);
}

void EnvelopeSection::notifyParentOfValueChange()
{
    _params.notify->setValueNotifyingHost(true);
}


void EnvelopeSection::resized() {

    int labelsectionheight = findValue(Skin::kLabelHeight);

    juce::Rectangle<int> area (getLocalBounds());
    envelopeSectionBorder.setBounds(area);
    area.removeFromTop(17);
    area.reduce(10, 0);

    juce::Rectangle<int> envArea = area.removeFromTop(area.getHeight() - getKnobSectionHeight());
    envelope_->setBounds(envArea);

    juce::Rectangle<int> knobs_area = area.removeFromTop(getKnobSectionHeight());
    placeKnobsInArea(knobs_area, { attack_.get(), decay_.get(), sustain_.get(), release_.get() }, true);

    juce::Rectangle<int> attack_label_rect (attack_->getX(), attack_->getBottom() - 10, attack_->getWidth(), labelsectionheight );
    attack_label->setBounds(attack_label_rect);
    juce::Rectangle<int> decay_label_rect (decay_->getX(), decay_->getBottom() - 10, decay_->getWidth(), labelsectionheight );
    decay_label->setBounds(decay_label_rect);
    juce::Rectangle<int> sustain_label_rect (sustain_->getX(), sustain_->getBottom() - 10, sustain_->getWidth(), labelsectionheight );
    sustain_label->setBounds(sustain_label_rect);
    juce::Rectangle<int> release_label_rect (release_->getX(), release_->getBottom() - 10, release_->getWidth(), labelsectionheight );
    release_label->setBounds(release_label_rect);

    envelope_->setSizeRatio(getSizeRatio());

    static constexpr float kMagnifyingHeightRatio = 0.2f;
    int magnify_height = envelope_->getHeight() * kMagnifyingHeightRatio;
    drag_magnifying_glass_->setBounds(envelope_->getRight() - magnify_height, envelope_->getY(),
                                    magnify_height, magnify_height);
    envelope_->magnifyReset();

    SynthSection::resized();
}

void EnvelopeSection::setADSRVals(float a, float d, float s, float r, float ap, float dp, float rp)
{
    envelope_->setADSRVals(a, d, s, r, ap, dp, rp);
}

void EnvelopeSection::reset() {
  envelope_->resetPositions();
  SynthSection::reset();
}

void EnvelopeSection::magnifyDragged(juce::Point<float> delta) {
  envelope_->magnifyZoom(delta);
}

void EnvelopeSection::magnifyDoubleClicked() {
  envelope_->magnifyReset();
}
