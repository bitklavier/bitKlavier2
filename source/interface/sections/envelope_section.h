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

#pragma once


#include "synth_section.h"
#include "chowdsp_plugin_state/chowdsp_plugin_state.h"
#include "EnvParams.h"
#include "OpenGL_LabeledBorder.h"

class EnvelopeEditor;
class SynthSlider;

class DragMagnifyingGlass : public OpenGlShapeButton {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void magnifyDragged(juce::Point<float> delta) = 0;
        virtual void magnifyDoubleClicked() = 0;
    };

    DragMagnifyingGlass();

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    juce::Point<float> last_position_;
    juce::Point<int> mouse_down_position_;
    std::vector<Listener*> listeners_;
};

class EnvelopeSection : public SynthSection, DragMagnifyingGlass::Listener {

public:
    EnvelopeSection( EnvParams &params, chowdsp::ParameterListeners& listeners, SynthSection &parent, juce::UndoManager* um = nullptr);

    virtual ~EnvelopeSection();

    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void reset() override;
    void magnifyDragged(juce::Point<float> delta) override;
    void magnifyDoubleClicked() override;
    void mouseUp(const juce::MouseEvent& e) override;
    void notifyParentOfValueChange();

    void setADSRVals(float a, float d, float s, float r, float ap, float dp, float rp);

  private:
    std::vector<std::unique_ptr<SynthSlider>> viewable;
    std::shared_ptr<EnvelopeEditor> envelope_;
    std::unique_ptr<SynthSlider> delay_;
    std::unique_ptr<SynthSlider> attack_;
    std::unique_ptr<SynthSlider> attack_power_;
    std::unique_ptr<SynthSlider> hold_;
    std::unique_ptr<SynthSlider> decay_;
    std::unique_ptr<SynthSlider> decay_power_;
    std::unique_ptr<SynthSlider> sustain_;
    std::unique_ptr<SynthSlider> release_;
    std::unique_ptr<SynthSlider> release_power_;
    std::unique_ptr<DragMagnifyingGlass> drag_magnifying_glass_;
    std::unique_ptr<chowdsp::SliderAttachment> delay_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> attack_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> attackPower_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> hold_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> decay_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> decayPower_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> sustain_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> release_attachment;
    std::unique_ptr<chowdsp::SliderAttachment> releasePower_attachment;

    std::shared_ptr<PlainTextComponent> attack_label;
    std::shared_ptr<PlainTextComponent> decay_label;
    std::shared_ptr<PlainTextComponent> sustain_label;
    std::shared_ptr<PlainTextComponent> release_label;

    std::shared_ptr<OpenGL_LabeledBorder> envelopeBorder;
    EnvParams& _params;

    chowdsp::ScopedCallbackList sliderChangedCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeSection)
};

