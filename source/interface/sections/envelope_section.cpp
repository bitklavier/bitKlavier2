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

EnvelopeSection::EnvelopeSection(juce::String name, std::string value_prepend, EnvParams &params, chowdsp::ParameterListeners& listeners, SynthSection &parent) : SynthSection(name) {

    setComponentID(parent.getComponentID());
    delay_ = std::make_unique<SynthSlider>("delay");
    delay_attachment = std::make_unique<chowdsp::SliderAttachment>(params.delayParam, listeners, *delay_, nullptr);
    //delay_ = std::make_unique<SynthSlider>(value_prepend + "_delay");
  addSlider(delay_.get());

  delay_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  delay_->setPopupPlacement(juce::BubbleComponent::below);
  delay_->parentHierarchyChanged();
  delay_->setVisible(false);


  attack_ = std::make_unique<SynthSlider>("attack");
  //attack_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attackParam, listeners, *attack_, nullptr);
  addSlider(attack_.get());
  attack_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  attack_->setPopupPlacement(juce::BubbleComponent::below);
  attack_->parentHierarchyChanged();
  attack_->setValue(params.attackParam->getDefaultValue());

  attack_power_ = std::make_unique<SynthSlider>( "attack_power");
  addSlider(attack_power_.get());
  attack_power_->setRange(-10., 10.); // don't need to do this in the original Vital version, not sure why we need this here
  attack_power_->setVisible(false);

  hold_ = std::make_unique<SynthSlider>(parent.getComponentID() +"_hold");
  //hold_ = std::make_unique<SynthSlider>(value_prepend + "_hold");
  //hold_attachment = std::make_unique<chowdsp::SliderAttachment>(params.holdParam, listeners, *hold_, nullptr);
  addSlider(hold_.get());
  hold_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  hold_->setPopupPlacement(juce::BubbleComponent::below);
  hold_->parentHierarchyChanged();
  hold_->setVisible(false);

  decay_ = std::make_unique<SynthSlider>( "decay");
  addSlider(decay_.get());
  decay_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  decay_->setPopupPlacement(juce::BubbleComponent::below);
  decay_->setValue(params.decayParam->getDefaultValue());
  
  decay_power_ = std::make_unique<SynthSlider>( "decay_power");
  addSlider(decay_power_.get());
  decay_power_->setRange(-10., 10.);
  decay_power_->setVisible(false);

  //release_ = std::make_unique<SynthSlider>(value_prepend + "_release");
  release_ = std::make_unique<SynthSlider>("release");
  addSlider(release_.get());
  release_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  release_->setPopupPlacement(juce::BubbleComponent::below);
  release_->setValue(params.releaseParam->getDefaultValue());

  release_power_ = std::make_unique<SynthSlider>( "release_power");
  addSlider(release_power_.get());
  release_power_->setRange(-10., 10.);
  release_power_->setVisible(false);

  sustain_ = std::make_unique<SynthSlider>("sustain");
  //sustain_ = std::make_unique<SynthSlider>(value_prepend + "_sustain");
  addSlider(sustain_.get());
  sustain_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  sustain_->setPopupPlacement(juce::BubbleComponent::below);
  sustain_->setValue(params.sustainParam->getDefaultValue());

  envelope_ = std::make_shared<EnvelopeEditor>(value_prepend);
  addOpenGlComponent(envelope_);
  envelope_->setName(value_prepend);
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

  drag_magnifying_glass_ = std::make_unique<DragMagnifyingGlass>();
  drag_magnifying_glass_->addListener(this);
  addAndMakeVisible(drag_magnifying_glass_.get());
  addOpenGlComponent(drag_magnifying_glass_->getGlComponent());

//  params.doForAllParameters([this, &listeners](auto& param, size_t) {
//      if( auto *newparm = dynamic_cast<chowdsp::FloatParameter*>((static_cast<juce::RangedAudioParameter*>(&param))))
//      {
//          if(newparm->paramID == "attack") {
//              DBG("yeet");
//              attack_attachment = chowdsp::SliderAttachment(*newparm, listeners, *attack_, nullptr);
//          }
//      }
//
//      });
//

  delay_attachment = std::make_unique<chowdsp::SliderAttachment>(params.delayParam, listeners, *delay_, nullptr);
  attack_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attackParam, listeners, *attack_, nullptr);
  attackPower_attachment = std::make_unique<chowdsp::SliderAttachment>(params.attackPowerParam, listeners, *attack_power_, nullptr);
  hold_attachment = std::make_unique<chowdsp::SliderAttachment>(params.holdParam, listeners, *hold_, nullptr);
  decay_attachment = std::make_unique<chowdsp::SliderAttachment>(params.decayParam, listeners, *decay_, nullptr);
  decayPower_attachment = std::make_unique<chowdsp::SliderAttachment>(params.decayPowerParam, listeners, *decay_power_, nullptr);
  sustain_attachment = std::make_unique<chowdsp::SliderAttachment>(params.sustainParam, listeners, *sustain_, nullptr);
  release_attachment = std::make_unique<chowdsp::SliderAttachment>(params.releaseParam, listeners, *release_, nullptr);
  releasePower_attachment = std::make_unique<chowdsp::SliderAttachment>(params.releasePowerParam, listeners, *release_power_, nullptr);

  //setSkinOverride(Skin::kEnvelope);
}

EnvelopeSection::~EnvelopeSection() { }

void EnvelopeSection::paintBackground(juce::Graphics& g) {
  setLabelFont(g);
  drawLabelForComponent(g, TRANS("DELAY"), delay_.get());
  drawLabelForComponent(g, TRANS("Attack"), attack_.get());
  drawLabelForComponent(g, TRANS("HOLD"), hold_.get());
  drawLabelForComponent(g, TRANS("Decay"), decay_.get());
  drawLabelForComponent(g, TRANS("Sustain"), sustain_.get());
  drawLabelForComponent(g, TRANS("Release"), release_.get());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void EnvelopeSection::resized() {
  static constexpr float kMagnifyingHeightRatio = 0.2f;
  int knob_section_height = getKnobSectionHeight();
  int knob_y = getHeight() - knob_section_height;

  int widget_margin = findValue(Skin::kWidgetMargin);
  int envelope_height = knob_y - widget_margin;
  envelope_->setBounds(widget_margin, widget_margin, getWidth() - 2 * widget_margin, envelope_height);

  juce::Rectangle<int> knobs_area(0, knob_y, getWidth(), knob_section_height);

  // for now, leave out delay and hold, focus on just ADSR
//  placeKnobsInArea(knobs_area, { delay_.get(), attack_.get(), hold_.get(),
//                                 decay_.get(), sustain_.get(), release_.get() });

  placeKnobsInArea(knobs_area, { attack_.get(), decay_.get(), sustain_.get(), release_.get() });

  SynthSection::resized();
  envelope_->setSizeRatio(getSizeRatio());

  int magnify_height = envelope_->getHeight() * kMagnifyingHeightRatio;
  drag_magnifying_glass_->setBounds(envelope_->getRight() - magnify_height, envelope_->getY(),
                                    magnify_height, magnify_height);

  envelope_->magnifyReset();
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
