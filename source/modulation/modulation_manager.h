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

#include "bar_renderer.h"
#include "modulation_button.h"
#include "open_gl_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include "synth_slider.h"
#include <tracktion_ValueTreeUtilities.h>
#include "ModuleListInterface.h"
#include "open_gl_combo_box.h"

class ModulationSection;
class ExpandModulationButton;
class ModulationMatrix;
class ModulationMeter;
class ModulationDestination;
class SynthBase;

namespace bitklavier {
    class ModulationConnectionBank;
    class StateConnection;
}

class ModulationAmountKnob : public SynthSlider {
  public:
    enum MenuOptions {
      kDisconnect = 0xff,
      kToggleBypass,
      kToggleBipolar,
      kToggleStereo,
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void disconnectModulation(ModulationAmountKnob* modulation_knob) = 0;
        virtual void setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) = 0;
        virtual void setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) = 0;
        virtual void setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) = 0;
    };

    ModulationAmountKnob(juce::String name, int index, const juce::ValueTree &v);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void handleModulationMenuCallback(int result);

    void makeVisible(bool visible);
    void hideImmediately();

    void setCurrentModulator(bool current);
    void setDestinationComponent(juce::Component* component, const std::string& name);
    juce::Colour getInternalColor();
    void setSource(const std::string& name);

    juce::Colour withBypassSaturation(juce::Colour color) const {
      if (bypass_)
        return color.withSaturation(0.0f);
      return color;
    }

    virtual juce::Colour getUnselectedColor() const override {
      return withBypassSaturation(SynthSlider::getUnselectedColor());
    }

    virtual juce::Colour getSelectedColor() const override {
      return withBypassSaturation(SynthSlider::getSelectedColor());
    }

    virtual juce::Colour getThumbColor() const override {
      return withBypassSaturation(SynthSlider::getThumbColor());
    }
    void valueChanged() override {
        //DBG("valuechanged");
        SynthSlider::valueChanged();
    }

    void setBypass(bool bypass) { bypass_ = bypass; setColors(); }
    void setStereo(bool stereo) { stereo_ = stereo; }
    void setBipolar(bool bipolar) { bipolar_ = bipolar; }
    bool isBypass() { return bypass_; }
    bool isStereo() { return stereo_; }
    bool isBipolar() { return bipolar_; }
    bool isOffsetMod() { return offset_; } // mod amount becomes offset from slider val when offset_ = true; otherwise mod amount is literal target
    bool enteringValue() { return text_entry_ && text_entry_->isVisible(); }
    bool isCurrentModulator() { return current_modulator_; }
    int index() { return index_; }

    void setAux(juce::String name) {
      aux_name_ = name;
      setName(aux_name_);
      setModulationAmount(1.0f);
    }
    bool hasAux() { return !aux_name_.isEmpty(); }
    void removeAux() {
      aux_name_ = "";
      setName(name_);
      setModulationAmount(0.0f);
    }
    juce::String getOriginalName() { return name_; }

    force_inline bool hovering() const {
      return hovering_;
    }

    void addModulationAmountListener(Listener* listener) { listeners_.push_back(listener); }
    void setDestinationSlider(SynthSlider* dest);
    SynthSlider *destination = nullptr;
    float get0to1value() { return my0to1amt; }
    float my0to1amt;

  private:
    void toggleBypass();

    std::vector<Listener*> listeners_;

    juce::Point<int> mouse_down_position_;
    juce::Component* color_component_;
    juce::String aux_name_;
    juce::String name_;
    bool editing_;
    int index_;
    bool showing_;
    bool hovering_;
    bool current_modulator_;
    bool bypass_;
    bool stereo_;
    bool bipolar_;

    /*
     * offset_
     *  true => mod value is offset from sliderVal, otherwise,
     *  false => mod value is literal target val.
     *
     *  false is the only mode for now, for ramp mods, and has no impact on lfo mods
     *  - might want to clean this up at some point, but works for now
     */
    bool offset_ = false;

    bool draw_background_;
    juce::ValueTree state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationAmountKnob)
};

class ModulationIndicator : public OpenGlQuad {
public:
    enum MenuOptions {
        kDisconnect = 0xff,
        kToggleBypass,
        kToggleBipolar,
        kToggleStereo,
    };
    ModulationIndicator (juce::String name, int index, const juce::ValueTree &v):
    OpenGlQuad(Shaders::kRoundedRectangleFragment) , index_(index), state(v)
    {
        setInterceptsMouseClicks(true,false);
        setName(name);
    }
    juce::ValueTree state;
    void makeVisible(bool visible)
    {
        if (visible == showing_)
            return;

        showing_ = visible;
        setVisible(visible);
        setAlpha((showing_ || hovering_) ? 1.0f : 0.0f);
    }
    void render(OpenGlWrapper& open_gl, bool animate) override
    {
        OpenGlQuad::render(open_gl, animate);
    }
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void disconnectModulation(ModulationIndicator* modulation_knob) = 0;
        virtual void modulationClicked(ModulationIndicator* modulation) = 0;
//        virtual void beginModulationEdit(ModulationIndicator* mod) = 0;
//        virtual void endModulationEdit(ModulationIndicator* mod) = 0;
    };
    void addModulationIndicatorListener(Listener* listener) { listeners_.push_back(listener); }
    void setSource(const std::string& name);
    void setDestination(const std::string& name);

    void setCurrentModulator(bool current);
    void mouseDown(const juce::MouseEvent&) override;
    void positionModulationIndicators();
    void positionModulationIndicatorsInside();
    void handleModulationMenuCallback(int result);
    int index() { return index_; }

private:
    int index_;

    bool showing_;
    bool current_modulator_;
    bool hovering_;
    std::vector<Listener*> listeners_;
};

class ModulationExpansionBox : public OpenGlQuad {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void expansionFocusLost() = 0;
    };

    ModulationExpansionBox() : OpenGlQuad(Shaders::kRoundedRectangleFragment) { }

    void focusLost(FocusChangeType cause) override {
      OpenGlQuad::focusLost(cause);

      for (Listener* listener : listeners_)
        listener->expansionFocusLost();
    }

    void setAmountControls(std::vector<ModulationAmountKnob*> amount_controls) { amount_controls_ = amount_controls; }
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<ModulationAmountKnob*> amount_controls_;
    std::vector<Listener*> listeners_;
};

class ModulationManager : public SynthSection,
                          public ModulationButton::Listener,
                          public ModulationAmountKnob::Listener,
                          public SynthSlider::SliderListener,
                          public ModulationExpansionBox::Listener,
                          public ModulesInterface::Listener,
public ModulationIndicator::Listener,
public StateModulatedComponent::Listener,
public OpenGLComboBox::Listener
// public juce::ValueTree::Listener
{
  public:
    static constexpr int kIndicesPerMeter = 6;
    static constexpr float kDragImageWidthPercent = 0.018f;

    ModulationManager(const juce::ValueTree& tree,SynthBase* bank
                      );
    ~ModulationManager();


    void createModulationMeter(
                               SynthSlider* slider, OpenGlMultiQuad* quads, int index);
    void createModulationSlider(std::string name, SynthSlider* slider, bool poly);

    void connectModulation(std::string source, std::string destination);
    void connectStateModulation(std::string source, std::string destination);
    void removeModulation(std::string source, std::string destination);
    void removeStateModulation(std::string source, std::string destination);
    void setModulationSliderValue(int index, float value);
    void setModulationSliderBipolar(int index, bool bipolar);
    void setModulationSliderValues(int index, float value);
    void setModulationSliderScale(int index);
    void setModulationValues(std::string source, std::string destination,
                             float amount, bool bipolar, bool stereo, bool bypass);
    void setStateModulationValues(std::string source, std::string destination, const juce::ValueTree&);
    void setStateModulationAmounts();
    void reset() override;
    void initAuxConnections();
    void makeCurrentStateModulatorsVisible();
    void resized() override;

    void parentHierarchyChanged() override;
    void updateModulationMeterLocations();
    void modulationAmountChanged(SynthSlider* slider) override;
    void modulationRemoved(SynthSlider* slider) override;

    void modulationDisconnected(bitklavier::ModulationConnection* connection, bool last) override;
    void modulationDisconnected(bitklavier::StateConnection* connection, bool last) override;
    void modulationSelected(ModulationButton* source) override;
    void modulationClicked(ModulationButton* source) override;
    void modulationCleared() override;
    bool hasFreeConnection();
    void startModulationMap(ModulationButton* source, const juce::MouseEvent& e) override;
    void modulationDragged(const juce::MouseEvent& e) override;
    void modulationWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void clearTemporaryModulation();
    void clearTemporaryHoverModulation();
    void modulationDraggedToHoverSlider(ModulationAmountKnob* hover_slider);
    void modulationDraggedToComponent(juce::Component* component, bool bipolar);
    void setTemporaryModulationBipolar(juce::Component* component, bool bipolar);
    void endModulationMap() override;
    void modulationLostFocus(ModulationButton* source) override;

    void expansionFocusLost() override {
      hideModulationAmountCallout();
    }

    void preparationClosed(bool isModulation);
    void clearModulationSource();
    void disconnectModulation(ModulationIndicator* modulation_knob) override;
    void disconnectModulation(ModulationAmountKnob* modulation_knob) override;
    void setModulationSettings(ModulationAmountKnob* modulation_knob);
    void setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) override;
    void setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) override;
    void setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) override;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void drawModulationDestinations(OpenGlWrapper& open_gl);
    void drawCurrentModulator(OpenGlWrapper& open_gl);
    void drawDraggingModulation(OpenGlWrapper& open_gl);
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void renderMeters(OpenGlWrapper& open_gl, bool animate);
    void renderSourceMeters(OpenGlWrapper& open_gl, int index);
    void updateSmoothModValues();
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void paintBackground(juce::Graphics& g) override { positionModulationAmountSliders(); }

    void modulationClicked(ModulationIndicator*);
    void setModulationAmounts();
    void setVisibleMeterBounds();
    void hoverStarted(SynthButton* button) override;
    void hoverEnded(SynthButton* button) override;
    void hoverStarted(SynthSlider* slider) override;
    void hoverEnded(SynthSlider* slider) override;
    void menuFinished(SynthSlider* slider) override;
    void modulationsChanged(const std::string& name) override;
    int getIndexForModulationSlider(juce::Slider* slider);
    int getIndexForModulationIndicator(juce::Component* component);
    int getModulationIndex(std::string source, std::string destination);
    int getStateModulationIndex(std::string source, std::string destination);
    bitklavier::StateConnection* getConnectionForModulationIndicator(ModulationIndicator* component);
    bitklavier::ModulationConnection* getConnectionForModulationSlider(juce::Slider* slider);
    bitklavier::ModulationConnection* getConnection(int index);
    bitklavier::StateConnection* getStateConnection(int index);
    bitklavier::StateConnection* getStateConnection(const std::string& source, const std::string& dest);
    bitklavier::ModulationConnection* getConnection(const std::string& source, const std::string& dest);
    void mouseDown(SynthSlider* slider) override;
    void mouseUp(SynthSlider* slider) override;
    void doubleClick(SynthSlider* slider) override;
    void beginModulationEdit(SynthSlider* slider) override;
    void endModulationEdit(SynthSlider* slider) override;
//    void beginModulationEdit(ModulationIndicator* slider) override;
//    void endModulationEdit(ModulationIndicator* slider) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void hideUnusedHoverModulations();
    float getAuxMultiplier(int index);
    void addAuxConnection(int from_index, int to_index);
    void removeAuxDestinationConnection(int to_index);
    void removeAuxSourceConnection(int from_index);
    void hoverStarted(StateModulatedComponent* component) override;
    void hoverEnded(StateModulatedComponent* component) override;
    void hoverStarted(OpenGLComboBox* component) override;
    void hoverEnded(OpenGLComboBox* component) override;

    // this will be called by both
    //  ModulesInterface<ModulationSection>::Listener,
    void added() override
    {
        DBG("ModulationManager::componentAdded added: called");
        componentAdded();
        DBG("done adding");
    }
    void removed() override
    {
        DBG("ModulationManager::remove");
        componentAdded();
        DBG("doneremoving");
    }
    void effectsMoved() override
    {
        return;
    }
    // void valueTreeChildRemoved(juce::ValueTree &parentTree, juce::ValueTree &childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
  private:
    SynthBase *base;
    void setDestinationQuadBounds(ModulationDestination* destination);
    void makeCurrentModulatorAmountsVisible();
    void makeModulationsVisible(SynthSlider* destination, bool visible);
    void makeModulationsVisible(StateModulatedComponent* destination, bool visible);
    void makeModulationsVisible(OpenGLComboBox* destination, bool visible);
    void makeModulationsVisible(SynthButton* destination, bool visible);
    void positionModulationAmountSlidersInside(const std::string& source,
                                               std::vector<bitklavier::ModulationConnection*> connections);
    void positionModulationAmountSlidersCallout(const std::string& source,
                                                std::vector<bitklavier::ModulationConnection*> connections);
    void showModulationAmountCallout(const std::string& source);
    void hideModulationAmountCallout();
    void positionModulationAmountSliders(const std::string& source);
    void positionModulationAmountSliders();
    bool enteringHoverValue();
    void showModulationAmountOverlay(ModulationAmountKnob* slider);
    void hideModulationAmountOverlay();
    void componentAdded();

    // juce::CriticalSection open_gl_critical_section_;
    std::unique_ptr<juce::Component> modulation_destinations_;
    std::map<juce::Viewport*, int> num_rotary_meters;
    std::map<juce::Viewport*, int> num_linear_meters;
    std::map<juce::Viewport*, int> num_button_meters;
    ModulationButton* current_source_;
    ExpandModulationButton* current_expanded_modulation_;
    juce::Component::SafePointer<ModulationDestination> temporarily_set_destination_;
    juce::Component::SafePointer<SynthSlider>           temporarily_set_synth_slider_;
    juce::Component::SafePointer<SynthButton>           temporarily_set_button_;
    juce::Component::SafePointer<StateModulatedComponent> temporarily_set_state_component_;
    juce::Component::SafePointer<OpenGLComboBox>        temporarily_set_state_combo_box_;
    juce::Component::SafePointer<ModulationAmountKnob>  temporarily_set_hover_slider_;
    bool temporarily_set_bipolar_;
    OpenGlQuad drag_quad_;
    std::shared_ptr<ModulationExpansionBox> modulation_expansion_box_;
    OpenGlQuad current_modulator_quad_;
    OpenGlQuad editing_rotary_amount_quad_;
    OpenGlQuad editing_linear_amount_quad_;
    StateModulatedComponent* editing_state_component_ = nullptr;

    typedef struct comboBoxMod {
        juce::PopupMenu* popup_menu = nullptr;
        juce::ValueTree modulation;
        juce::String paramID;
    } comboBoxMod;
    comboBoxMod editing_button_mod;
    comboBoxMod editing_comboBox_mod;

    std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_destinations_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_destinations_;
    std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_meters_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_meters_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> button_meters_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> button_destinations_;
    juce::Point<int> mouse_drag_start_;
    juce::Point<int> mouse_drag_position_;
    bool modifying_;
    bool dragging_;
    bool changing_hover_modulation_;

    ModulationButton* current_modulator_;
    std::map<std::string, ModulationButton*> modulation_buttons_;
    std::map<std::string, std::unique_ptr<ExpandModulationButton>> modulation_callout_buttons_;

    std::map<std::string, bool> active_mod_values_;
    long long last_milliseconds_;
    std::unique_ptr<BarRenderer> modulation_source_meters_;

    std::map<std::string, ModulationDestination*> destination_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::map<std::string, SynthButton*> button_model_lookup_;
    std::map<std::string, StateModulatedComponent*> state_model_lookup_;
    std::map<std::string, OpenGLComboBox*> combo_box_lookup;
    std::map<std::string, ModulationAmountKnob*> modulation_amount_lookup_;
    std::map<std::string, ModulationIndicator*> modulation_indicators_lookup_;

    std::vector<std::unique_ptr<ModulationDestination>> all_destinations_;
    std::map<std::string, std::unique_ptr<ModulationMeter>> meter_lookup_;
    std::map<int, int> aux_connections_from_to_;
    std::map<int, int> aux_connections_to_from_;
    std::unique_ptr<ModulationAmountKnob> modulation_amount_sliders_[bitklavier::kMaxModulationConnections];
    std::unique_ptr<ModulationAmountKnob> modulation_hover_sliders_[bitklavier::kMaxModulationConnections];
    std::unique_ptr<ModulationAmountKnob> selected_modulation_sliders_[bitklavier::kMaxModulationConnections];

    std::unique_ptr<ModulationIndicator> modulation_indicators_[bitklavier::kMaxStateConnections];
    std::unique_ptr<ModulationIndicator> modulation_hover_indicators_[bitklavier::kMaxStateConnections];
    std::unique_ptr<ModulationIndicator> selected_modulation_indicators_[bitklavier::kMaxStateConnections];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};

