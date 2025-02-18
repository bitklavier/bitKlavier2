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


#include "open_gl_image_component.h"

namespace bitklavier {
  struct ModulationConnection;
  struct StateConnection;
} // namespace vital

class SynthGuiInterface;

class ModulationButton : public PlainShapeComponent {
  public:
    static constexpr float kFontAreaHeightRatio = 0.3f;
    static constexpr int kModulationKnobColumns = 3;
    static constexpr int kModulationKnobRows = 2;
    static constexpr int kMaxModulationKnobs = kModulationKnobRows * kModulationKnobColumns;
    static constexpr float kMeterAreaRatio = 0.05f;

    enum MenuId {
      kCancel = 0,
      kDisconnect,
      kModulationList
    };

    enum MouseState {
      kNone,
      kHover,
      kMouseDown,
      kMouseDragging,
      kDraggingOut
    };

    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void modulationConnectionChanged() { }
        virtual void modulationDisconnected(bitklavier::StateConnection* connection, bool last){}
        virtual void modulationDisconnected(bitklavier::ModulationConnection* connection, bool last) { }
        virtual void modulationSelected(ModulationButton* source) { }
        virtual void modulationLostFocus(ModulationButton* source) { }
        virtual void startModulationMap(ModulationButton* source, const juce::MouseEvent& e) { }
        virtual void modulationDragged(const juce::MouseEvent& e) { }
        virtual void modulationWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) { }
        virtual void endModulationMap() { }
        virtual void modulationClicked(ModulationButton* source) { }
        virtual void modulationCleared() { }
    };
  
    ModulationButton(juce::String name);
    virtual ~ModulationButton();
    void init(OpenGlWrapper& ) override;
    void paintBackground(juce::Graphics& g) override;
    void parentHierarchyChanged() override;
    void resized() override;
    bool isInit() override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void focusLost(FocusChangeType cause) override;
    void addListener(Listener* listener);
    void disconnectIndex(int index);

    void select(bool select);
    bool isSelected() const { return selected_; }
    void setActiveModulation(bool active);
    bool isActiveModulation() const { return active_modulation_; }

    void setForceEnableModulationSource();
    bool hasAnyModulation();
    void setFontSize(float size) { font_size_ = size; }
    juce::Rectangle<int> getModulationAmountBounds(int index, int total);
    juce::Rectangle<int> getModulationAreaBounds();
    juce::Rectangle<int> getMeterBounds();
    void setConnectRight(bool connect) { connect_right_ = connect; repaint(); }
    void setDrawBorder(bool border) { draw_border_ = border; repaint(); }
    void overrideText(juce::String text) { text_override_ = std::move(text); repaint(); }
    void setStateModulation(bool state) { state_modulation = state; };
    bool isStateModulation() const { return state_modulation; }
  private:
    void disconnectModulation(bitklavier::ModulationConnection* connection);
    bool initialized;
    juce::String text_override_;
    SynthGuiInterface* parent_;
    std::vector<Listener*> listeners_;
    MouseState mouse_state_;
    bool selected_;
    bool connect_right_;
    bool draw_border_;
    bool active_modulation_;
    bool state_modulation;
    OpenGlImageComponent drag_drop_;
    Component drag_drop_area_;
    float font_size_;

    juce::Colour drag_drop_color_;
    bool show_drag_drop_;
    float drag_drop_alpha_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationButton)
};

