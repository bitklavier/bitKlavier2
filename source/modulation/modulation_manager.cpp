/* Copyright 2013-2019 Matt Tytel
 *
 * bitklavier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bitklavier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bitklavier.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "modulation_manager.h"
#include "ModulationModuleSection.h"
#include "bar_renderer.h"
#include "skin.h"
#include "modulation_meter.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "ModulationConnection.h"
#include "FullInterface.h"
#include "open_gl_combo_box.h"
#include "synth_base.h"
#include "StateModulatedComponent.h"

namespace {
    constexpr float kDefaultModulationRatio = 0.25f;
    constexpr float kModSourceMeterWidth = 0.0018f;
    constexpr float kModSourceMeterBuffer = 0.002f;
    constexpr float kModSourceMinRadius = 0.005f;
    constexpr float kModSmoothDecay = 0.25f;

    bool showingInParents(juce::Component *component) {
        if (component == nullptr || component->getParentComponent() == nullptr)
            return true;

        return component->isVisible() && showingInParents(component->getParentComponent());
    }
}

class ExpandModulationButton : public OpenGlToggleButton {
public:
    ExpandModulationButton() : OpenGlToggleButton("expand modulation"),
                               num_sliders_(0), amount_quad_(Shaders::kRingFragment) {
        setLightenButton();
        setTriggeredOnMouseDown(true);
        setMouseClickGrabsKeyboardFocus(false);
        amount_quad_.setTargetComponent(this);
        amount_quad_.setThickness(2.0f);
    }

    int getNumColumns(int num_sliders) {
        float height_width_ratio = getHeight() * 1.0f / getWidth();

        int columns = 1;
        while (columns * (int) (height_width_ratio * columns) < num_sliders)
            columns++;
        return columns;
    }

    void setSliders(std::vector<ModulationAmountKnob *> sliders) {
        sliders_ = sliders;
        for (int i = 0; i < sliders.size(); ++i)
            colors_[i] = sliders_[i]->findColour(Skin::kRotaryArc, true);
        num_sliders_ = static_cast<int>(sliders_.size());
    }

    std::vector<ModulationAmountKnob *> getSliders() { return sliders_; }

    void renderSliderQuads(OpenGlWrapper &open_gl, bool animate) {
        int num_sliders = num_sliders_;

        float width = getWidth();
        float height = getHeight();

        int columns = getNumColumns(num_sliders);
        float cell_width = width / columns;
        int rows = (num_sliders + columns - 1) / columns;
        int y_offset = (height - (rows * cell_width)) / 2;
        float gl_width = 2.0f * cell_width / width;
        float gl_height = 2.0f * cell_width / height;

        int row = 0;
        int column = 0;
        for (int i = 0; i < num_sliders; ++i) {
            float x = column * cell_width;
            float y = height - y_offset - (row + 1) * cell_width;

            amount_quad_.setColor(colors_[i]);
            amount_quad_.setAltColor(colors_[i].withMultipliedAlpha(0.5f));
            amount_quad_.setQuad(0, 2.0f * x / width - 1.0f, 1.0f - 2.0f * y / height - gl_height, gl_width, gl_height);
            amount_quad_.render(open_gl, animate);

            column++;
            if (column >= columns) {
                row++;
                column = 0;
            }
        }
    }

private:
    std::vector<ModulationAmountKnob *> sliders_;
    int num_sliders_;
    juce::Colour colors_[bitklavier::kMaxModulationConnections];

    OpenGlQuad amount_quad_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpandModulationButton)
};
using SliderPtr   = juce::Component::SafePointer<SynthSlider>;
using ButtonPtr   = juce::Component::SafePointer<SynthButton>;
using ComboPtr    = juce::Component::SafePointer<OpenGLComboBox>;
using StatePtr    = juce::Component::SafePointer<StateModulatedComponent>;
class ModulationDestination : public juce::Component {
public:
    ModulationDestination(SynthSlider *source) :  margin_(0), index_(0),
                                                 size_multiple_(1.0f),
                                                 active_(false), rectangle_(false), rotary_(true),
                                                 destination(source) {
        setComponentID(source->getComponentID());
    }

    ModulationDestination(SynthButton *source) : margin_(0), index_(0),
                                                 size_multiple_(1.0f), active_(false),
                                                 rectangle_(true), rotary_(false), destination(source) {
        setComponentID(source->getComponentID());
    }

    ModulationDestination(StateModulatedComponent *source) : margin_(0), index_(0),
                                                             size_multiple_(1.0f),
                                                             active_(false),
                                                             rectangle_(true), rotary_(false),
                                                             destination(source) {
        setComponentID(source->getComponentID());
    }

    ModulationDestination(OpenGLComboBox *source) :
     margin_(0), index_(0),
                                                 size_multiple_(1.0f), active_(false),
                                                 rectangle_(true), rotary_(false), destination(source) {
        setComponentID(source->getComponentID());
    }

    //ModulationDestination() = delete;
    //    void mouseEnter(const juce::MouseEvent &e)
    //    {
    //        //DBG("mosueenetere moddest");
    //    }
    virtual ~ModulationDestination() {}

    void setActive(bool active) { active_ = active; }

    void setSizeMultiple(float multiple) {
        size_multiple_ = multiple;
        repaint();
    }

    juce::Rectangle<float> getFillBounds() {
        static constexpr float kBufferPercent = 0.4f;

        float width = getWidth();
        float height = getHeight();

        if (!rectangle_ && rotary_) {
            float offset = std::get<SliderPtr>(destination)->findValue(Skin::kKnobOffset);
            float rotary_width = size_multiple_ *std::get<SliderPtr>(destination)->findValue(Skin::kKnobModMeterArcSize);
            float x = (width - rotary_width) / 2.0f;
            float y = offset + (height - rotary_width) / 2.0f;
            return juce::Rectangle<float>(x, y, rotary_width, rotary_width);
        }

        if (rectangle_)
            return getLocalBounds().toFloat();

        if (auto* slider = std::get_if<SliderPtr>(&destination)) {
            if (*slider != nullptr) {
                if ((*slider)->getSliderStyle() == juce::Slider::LinearBar)
                {
                    // LinearBar logic
                    float y = height * 0.5f * (1.0f - SynthSlider::kLinearWidthPercent);
                    float glow_height = height * SynthSlider::kLinearWidthPercent;
                    y -= 2.0f * glow_height * kBufferPercent;
                    glow_height += 4.0f * kBufferPercent * glow_height;

                    return juce::Rectangle<float>(margin_, y, width - 2 * margin_, glow_height);
                }
            }
        }

        float x = width * 0.5f * (1.0f - SynthSlider::kLinearWidthPercent);
        float glow_width = width * SynthSlider::kLinearWidthPercent;
        x -= 2.0f * glow_width * kBufferPercent;
        glow_width += 4.0f * kBufferPercent * glow_width;
        return juce::Rectangle<float>(x, margin_, glow_width, height - 2 * margin_);
    }

    void setRectangle(bool rectangle) { rectangle_ = rectangle; }
    void setRotary(bool rotary) { rotary_ = rotary; }
    void setMargin(int margin) { margin_ = margin; }
    void setIndex(int index) { index_ = index; }

    bool hasExtraModulationTarget() {
        return std::holds_alternative<SliderPtr>(destination) && std::get<SliderPtr>(destination)->getExtraModulationTarget() != nullptr;
    }

    bool isRotary() { return !rectangle_ && rotary_; }
    bool isRectangle() { return rectangle_; }
    bool isStateModulated() { return std::holds_alternative<StatePtr>(destination) || std::holds_alternative<ComboPtr>(destination); }

    bool isActive() { return active_; }
    int getIndex() { return index_; }
    std::variant<juce::Component::SafePointer<SynthSlider>, juce::Component::SafePointer<SynthButton>, juce::Component::SafePointer<OpenGLComboBox>, juce::Component::SafePointer<StateModulatedComponent>> destination;

private:
    juce::Component *viewport_container_;

    int margin_;
    int index_;
    float size_multiple_;
    bool active_;
    bool rectangle_;
    bool rotary_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationDestination)
};

/*
 *
 * ModulationAmountKnob stuff
 *
 */
ModulationAmountKnob::ModulationAmountKnob(juce::String name, int index, const juce::ValueTree &vt) : SynthSlider(name,juce::ValueTree{}),
    color_component_(nullptr), index_(index), state(vt) {
    setModulationKnob();
    bypass_ = false;
    stereo_ = false;
    bipolar_ = false;
    draw_background_ = false;
    name_ = name;
    editing_ = false;

    setShowPopupOnHover(true);
    setTextEntrySizePercent(2.0f, 1.0f);
    setDoubleClickReturnValue(false, 0.0f);
    setWantsKeyboardFocus(false);
    showing_ = true;
    hovering_ = false;
    current_modulator_ = false;
    setRange(0.f, 1.f, 0.f);
}

void ModulationAmountKnob::mouseDown(const juce::MouseEvent &e) {
    /*
     * removing the bypass popup and so on, for now
     * - most of the options don't seem to actually do anything, and i'm not sure this is a feature we'd want to add at this point anyhow...
     */

    if (e.mods.isMiddleButtonDown())
        toggleBypass();

    // remove option-click, at least for now; it's crashing
    if (e.mods.isAltDown()) return;

    if (e.mods.isPopupMenu()) {
        SynthSlider::mouseExit(e);

        PopupItems options;
        options.addItem(kManualEntry, "Enter Value");
        options.addItem(kDisconnect, "Remove Modification");
        //options.addItem(kToggleBypass, bypass_ ? "Unbypass" : "Bypass");
        //options.addItem(kToggleBipolar, bipolar_ ? "Make Unipolar" : "Make Bipolar");
        //options.addItem(kToggleStereo, stereo_ ? "Make Mono" : "Make Stereo");
        //options.addItem(-1, "");

        //    if (has_parameter_assignment_)
        //      options.addItem(kArmMidiLearn, "Learn MIDI Assignment");
        //
        //    if (has_parameter_assignment_ && synth_interface_->getSynth()->isMidiMapped(getComponentID().toStdString()))
        //      options.addItem(kClearMidiLearn, "Clear MIDI Assignment");

        // options.addItem(kManualEntry, "Enter Value Manually");

        hovering_ = false;
        redoImage();

        auto callback = [=](int selection, int) { handleModulationMenuCallback(selection); };
        auto cancel = [=]() {
            for (SliderListener *listener: slider_listeners_)
                listener->menuFinished(this);
        };
        parent_->showPopupSelector(this, e.getPosition(), options, callback, cancel);

        for (SliderListener *listener: slider_listeners_)
            listener->mouseDown(this);
    }
    else
    {
        SynthSlider::mouseDown(e);
        juce::MouseInputSource source = e.source;

        if (source.isMouse() && source.canDoUnboundedMovement()) {
            editing_ = true;
            source.hideCursor();
            source.enableUnboundedMouseMovement(true);
            mouse_down_position_ = e.getScreenPosition();
            for (SliderListener *listener: slider_listeners_)
                listener->beginModulationEdit(this);
        }
    }
}

void ModulationAmountKnob::mouseUp(const juce::MouseEvent &e) {
    if (!e.mods.isPopupMenu()) {
        SynthSlider::mouseUp(e);

        juce::MouseInputSource source = e.source;
        if (source.isMouse() && source.canDoUnboundedMovement()) {
            source.showMouseCursor(juce::MouseCursor::NormalCursor);
            source.enableUnboundedMouseMovement(false);
            if (getScreenBounds().contains(e.getScreenPosition()))
                editing_ = false;
            source.setScreenPosition(mouse_down_position_.toFloat());
        }
    }
}

void ModulationAmountKnob::mouseExit(const juce::MouseEvent &e) {
    if (!editing_) {
        for (SliderListener *listener: slider_listeners_)
            listener->endModulationEdit(this);
    }
    editing_ = false;
    SynthSlider::mouseExit(e);
}

void ModulationAmountKnob::toggleBypass() {
    bypass_ = !bypass_;
    for (Listener *listener: listeners_)
        listener->setModulationBypass(this, bypass_);
    setColors();
}

void ModulationAmountKnob::handleModulationMenuCallback(int result) {
    if (result == kDisconnect) {
        for (Listener *listener: listeners_)
            listener->disconnectModulation(this);
    } else if (result == kToggleBypass)
        toggleBypass();
    else if (result == kToggleBipolar) {
        bipolar_ = !bipolar_;
        for (Listener *listener: listeners_)
            listener->setModulationBipolar(this, bipolar_);
    } else if (result == kToggleStereo) {
        stereo_ = !stereo_;
        for (Listener *listener: listeners_)
            listener->setModulationStereo(this, stereo_);
    } else
        handlePopupResult(result);

    if (result != kManualEntry) {
        for (SliderListener *listener: slider_listeners_)
            listener->menuFinished(this);
    }
}

void ModulationAmountKnob::makeVisible(bool visible) {
    if (visible == showing_)
        return;

    showing_ = visible;
    setVisible(visible);
    setAlpha((showing_ || hovering_) ? 1.0f : 0.0f);
}

void ModulationAmountKnob::hideImmediately() {
    setAlpha(0.0f, true);
    showing_ = false;
    hovering_ = false;
    setVisible(false);
}

void ModulationAmountKnob::setCurrentModulator(bool current) {
    if (current_modulator_ == current)
        return;

    setColour(Skin::kRotaryArc, findColour(Skin::kModulationMeterControl, true));
    current_modulator_ = current;
}

void ModulationAmountKnob::setDestinationComponent(juce::Component *component, const std::string &name) {
    //  setPopupPrefix(bitklavier::juce::Parameters::getDisplayName(name) + ": ");

    if (color_component_)
        setColour(Skin::kRotaryArc, color_component_->findColour(Skin::kRotaryArc, true));
}

void ModulationAmountKnob::setDestinationSlider(SynthSlider *dest) {
    destination = dest;
    if(destination == nullptr)
        return;

    if (isBipolar() || isOffsetMod())
        this->setRange(0, dest->getNormalisableRange().getRange().getLength(), 0.f);
    else // mod val is actual target val
        this->setNormalisableRange (dest->getNormalisableRange());
        //this->setRange(dest->getMinimum(), dest->getMaximum(), 0.f);

    this->textFromValueFunction = dest->attachment->getParameter()->getStringFromValueFunction();
}

juce::Colour ModulationAmountKnob::getInternalColor() {
    if (color_component_)
        return color_component_->findColour(Skin::kRotaryArc, true);
    return findColour(Skin::kModulationMeterControl, true);
}

void ModulationAmountKnob::setSource(const std::string &name) {
    //  setPopupPrefix(ModulationMatrix::getMenuSourceDisplayName(name) + ": ");
    repaint();
}

/*
 *
 * ModulationIndicator stuff
 *
 */
void ModulationIndicator::setSource(const std::string &name) {
    repaint();
}

void ModulationIndicator::mouseDown(const juce::MouseEvent &e) {
    if (e.mods.isPopupMenu()) {
        mouseExit(e);

        PopupItems options;
        options.addItem(kDisconnect, "Remove");

        options.addItem(-1, "");

        hovering_ = false;

        auto callback = [=](int selection, int) { handleModulationMenuCallback(selection); };
        auto cancel = [=]() {};
        parent_->showPopupSelector(this, e.getPosition(), options, callback, cancel);
    }
    else {
        DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
        for (auto l: listeners_)
            l->modulationClicked(this);
    }
}

void ModulationIndicator::setDestination(const std::string &name) {}

void ModulationIndicator::handleModulationMenuCallback(int result) {
    if (result == kDisconnect) {
        for (Listener *listener: listeners_)
            listener->disconnectModulation(this);
    }
}

void ModulationIndicator::setCurrentModulator(bool current) {
    if (current_modulator_ == current)
        return;

    setColour(Skin::kRotaryArc, findColour(Skin::kModulationMeterControl, true));
    current_modulator_ = current;
}

/*
 *
 * ModulationManager stuff
 *
 */
ModulationManager::ModulationManager(const juce::ValueTree &tree, SynthBase *base
) : SynthSection("modulation_manager"),base(base),
    drag_quad_(Shaders::kRingFragment),
    current_modulator_quad_(Shaders::kRoundedRectangleBorderFragment),
    editing_rotary_amount_quad_(Shaders::kRotaryModulationFragment),
    editing_linear_amount_quad_(Shaders::kLinearModulationFragment),
    modifying_(false), dragging_(false), changing_hover_modulation_(false),
    current_modulator_(nullptr), modulation_expansion_box_(std::make_shared<ModulationExpansionBox>()) {
    current_modulator_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
    drag_quad_.setTargetComponent(this);
    editing_rotary_amount_quad_.setTargetComponent(this);
    editing_rotary_amount_quad_.setActive(false);
    editing_rotary_amount_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
    editing_linear_amount_quad_.setTargetComponent(this);
    editing_linear_amount_quad_.setActive(false);
    editing_linear_amount_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
    addOpenGlComponent(modulation_expansion_box_);
    modulation_expansion_box_->setVisible(false);
    modulation_expansion_box_->setWantsKeyboardFocus(true);
    modulation_expansion_box_->addListener(this);
    modulation_expansion_box_->setAlwaysOnTop(true);

    setSkinOverride(Skin::kModulationDragDrop);

    last_milliseconds_ = juce::Time::currentTimeMillis();
    current_source_ = nullptr;
    current_expanded_modulation_ = nullptr;
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_hover_slider_ = nullptr;
    temporarily_set_button_ = nullptr;
    temporarily_set_bipolar_ = false;
    temporarily_set_state_component_ = nullptr;

    setInterceptsMouseClicks(false, true);

    modulation_destinations_ = std::make_unique<juce::Component>();
    modulation_destinations_->setInterceptsMouseClicks(false, true);

    for (auto &slider: slider_model_lookup_) {
        std::string name = slider.first;
        //    const bitklavier::Output* mono_total = mono_modulations[name];

        //    if (mono_total == nullptr)
        //      continue;

        bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve();
        juce::Viewport *viewport = slider.second->findParentComponentOfClass<juce::Viewport>();
        //    const bitklavier::Output* poly_total = poly_modulations[name];

        //    if (rotary) {
        //      int index = num_rotary_meters[viewport] - 1;
        //      num_rotary_meters[viewport] = index;
        //      createModulationMeter(mono_total, poly_total, slider.second, rotary_meters_[viewport].get(), index);
        //    }
        //    else {
        //      int index = num_linear_meters[viewport] - 1;
        //      num_linear_meters[viewport] = index;
        //      createModulationMeter(mono_total, poly_total, slider.second, linear_meters_[viewport].get(), index);
        //    }

        //    slider.second->addSliderListener(this);
        //    createModulationSlider(name, slider.second, poly_total != nullptr);
    }

    addChildComponent(modulation_destinations_.get());

    bitklavier::ModulationConnectionBank &bank = base->getModulationBank();
    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        std::string name = "modulation_" + std::to_string(i + 1) + "_amount";
        //purple mod circle on the button
        modulation_amount_sliders_[i] = std::make_unique<ModulationAmountKnob>(name, i, bank.atIndex(i)->state);
        modulation_amount_sliders_[i]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        addSlider(modulation_amount_sliders_[i].get(), true, true);
        modulation_amount_sliders_[i]->addSliderListener(this);
        modulation_amount_sliders_[i]->addModulationAmountListener(this);
        modulation_amount_lookup_[name] = modulation_amount_sliders_[i].get();

        //purple mod circle under slider
        modulation_hover_sliders_[i] = std::make_unique<ModulationAmountKnob>(name, i, bank.atIndex(i)->state);
        modulation_hover_sliders_[i]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        addSlider(modulation_hover_sliders_[i].get(), true, true);
        modulation_hover_sliders_[i]->setAlpha(0.0f, true);
        modulation_hover_sliders_[i]->addSliderListener(this);
        modulation_hover_sliders_[i]->addModulationAmountListener(this);
        modulation_hover_sliders_[i]->setDrawWhenNotVisible(true);

        //green modulatinocircel under the slider
        selected_modulation_sliders_[i] = std::make_unique<ModulationAmountKnob>(name, i, bank.atIndex(i)->state);
        selected_modulation_sliders_[i]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        addSlider(selected_modulation_sliders_[i].get(), true, true);
        selected_modulation_sliders_[i]->setAlpha(0.0f, true);
        selected_modulation_sliders_[i]->addSliderListener(this);
        selected_modulation_sliders_[i]->addModulationAmountListener(this);
        selected_modulation_sliders_[i]->setDrawWhenNotVisible(true);
    }

    bitklavier::StateConnectionBank &state_bank = base->getStateBank();
    for (int i = 0; i < bitklavier::kMaxStateConnections; ++i) {
        std::string name = "modulation_" + std::to_string(i + 1) + "_amount";
        //purple mod circle on the button
        modulation_indicators_[i] = std::make_unique<ModulationIndicator>(name, i, state_bank.atIndex(i)->state);
        addAndMakeVisible(modulation_indicators_[i].get());
        modulation_indicators_[i]->setParent(this);
        modulation_indicators_[i]->addModulationIndicatorListener(this);
        modulation_amount_lookup_[name] = modulation_amount_sliders_[i].get();
        modulation_indicators_[i]->setLookAndFeel(DefaultLookAndFeel::instance());
        modulation_indicators_[i]->setColor(juce::Colours::red);

        //purple mod circle under slider
        modulation_hover_indicators_[i] = std::make_unique<ModulationIndicator>(name, i, state_bank.atIndex(i)->state);
        addAndMakeVisible(modulation_hover_indicators_[i].get());
        modulation_hover_indicators_[i]->setParent(this);
        modulation_hover_indicators_[i]->addModulationIndicatorListener(this);
        modulation_hover_indicators_[i]->setLookAndFeel(DefaultLookAndFeel::instance());
        modulation_hover_indicators_[i]->setColor(juce::Colours::white);

        //        modulation_selected_indicators_[i] = std::make_unique<ModulationIndicator>(name, i, state_bank.atIndex(i)->state);
        //        addAndMakeVisible(modulation_indicators_[i].get());
        //        modulation_selected_indicators_[i]->setParent(this);
        //        modulation_selected_indicators_[i]->addModulationIndicatorListener(this);

        //green modulatinocircel under the slider
        selected_modulation_indicators_[i] = std::make_unique<ModulationIndicator>(
            name, i, state_bank.atIndex(i)->state);
        addAndMakeVisible(selected_modulation_indicators_[i].get());
        selected_modulation_indicators_[i]->setAlpha(0.0f, true);
        selected_modulation_indicators_[i]->addModulationIndicatorListener(this);
        selected_modulation_indicators_[i]->setLookAndFeel(DefaultLookAndFeel::instance());
        selected_modulation_indicators_[i]->setColor(juce::Colours::greenyellow);
        selected_modulation_indicators_[i]->setParent(this);
    }
}

void ModulationManager::createModulationMeter( SynthSlider *slider, OpenGlMultiQuad *quads, int index)
{
    std::string name = slider->getComponentID().toStdString();

    std::unique_ptr<ModulationMeter> meter = std::make_unique<ModulationMeter>(
        slider, quads, index);
    addChildComponent(meter.get());
    meter->setName(name);
    meter->setBounds(getLocalArea(slider, slider->getLocalBounds()));
    meter_lookup_[name] = std::move(meter);
}

void ModulationManager::modulationClicked(ModulationIndicator *indicator)
{
    int index = getIndexForModulationIndicator(indicator);

    bitklavier::StateConnection *connection = getStateConnection(index);

    for (auto [name, comp]: state_model_lookup_) {
        if (name == connection->destination_name) {
            if(comp == nullptr) {
                break;
            }
            auto destination_bounds = getLocalArea(comp, comp->getLocalBounds());
            int center_x = destination_bounds.getCentreX();
            int left = destination_bounds.getX();
            int right = destination_bounds.getRight();
            int bottom = destination_bounds.getBottom();
            int top = destination_bounds.getY();
            int center_y = destination_bounds.getCentreY();

            if (editing_state_component_ != nullptr && editing_state_component_->getComponentID() != juce::String(
                    connection->destination_name)) {
                auto *parent = findParentComponentOfClass<SynthGuiInterface>();
                editing_state_component_->setVisible(false);
                editing_state_component_->getImageComponent()->setVisible(false);
                parent->getOpenGlWrapper()->context.executeOnGLThread([this](juce::OpenGLContext &context) {
                    auto *parent = findParentComponentOfClass<SynthGuiInterface>();
                    destroyOpenGlComponent(*editing_state_component_->getImageComponent(), *parent->getOpenGlWrapper());
                }, true);
                delete editing_state_component_;
                editing_state_component_ = nullptr;
            } else if (editing_state_component_ != nullptr && editing_state_component_->getComponentID() ==
                       juce::String(connection->destination_name)) {
                // Same destination editor is already open: retarget it to this specific connection
                editing_state_component_->modulationState = connection->state;
                // Ensure the UI reflects the newly selected connection's values
                editing_state_component_->syncToValueTree();
                editing_state_component_->setVisible(true);
                editing_state_component_->getImageComponent()->setVisible(true);
            }

            if (editing_state_component_ == nullptr) {
                editing_state_component_ = comp->clone();
                editing_state_component_->modulationState = connection->state;
                addAndMakeVisible(editing_state_component_);
                addOpenGlComponent(editing_state_component_->getImageComponent());
                // Prime the UI with this connection's stored values
                editing_state_component_->syncToValueTree();

                // //code for putting the editing/mod component above or below the destination component
                // int mod_offset = 0;
                // if (top > comp->getHeight()) mod_offset = comp->getHeight(); // enough space to put it above the destination component
                // else mod_offset = -comp->getHeight(); // otherwise, put it below
                // editing_state_component_->setBounds(
                //     center_x - comp->getWidth() / 2,
                //     top - mod_offset ,
                //     comp->getWidth(),
                //     comp->getHeight());

                //or just put it directly on top?
                editing_state_component_->setBounds(destination_bounds);
                editing_state_component_->addListener(this);
                editing_state_component_->setComponentID(juce::String(connection->destination_name));
            }
            editing_state_component_->syncToValueTree();
            setStateModulationValues(connection->source_name, connection->destination_name, connection->state);
            editing_state_component_->getImageComponent()->redrawImage(true);
        }
    }

    for (auto [name,comp] : combo_box_lookup) {
        if (name == connection->destination_name) {
            if(comp == nullptr) {
                break;
            }
            auto destination_bounds = getLocalArea(comp, comp->getLocalBounds());
            int center_x = destination_bounds.getCentreX();
            int left = destination_bounds.getX();
            int right = destination_bounds.getRight();

            int bottom = destination_bounds.getBottom();
            int top = destination_bounds.getY();
            int center_y = destination_bounds.getCentreY();
            editing_comboBox_mod.popup_menu = comp->clone();
            editing_comboBox_mod.modulation = connection->state;
            editing_comboBox_mod.paramID = comp->getParamID();
            static juce::var nullVar;
            if(editing_comboBox_mod.modulation.getProperty(editing_comboBox_mod.paramID)!=nullVar) {
                int ticked  = static_cast <int>(editing_comboBox_mod.modulation.getProperty(editing_comboBox_mod.paramID)) + 1;
                for (juce::PopupMenu::MenuItemIterator iterator (*editing_comboBox_mod.popup_menu, true); iterator.next();)
                {
                    auto& item = iterator.getItem();

                    if (item.itemID != 0)
                        item.isTicked = (item.itemID == ticked);
                }

            }
            setStateModulationValues(connection->source_name, connection->destination_name, connection->state);
            editing_comboBox_mod.popup_menu->showMenuAsync(juce::PopupMenu::Options().withTargetComponent (comp), [this](int idx) {
                if(idx == 0) return;

                editing_comboBox_mod.modulation.setProperty(editing_comboBox_mod.paramID, idx-1,nullptr);

         });

        }
    }

    for (auto [name,comp] : button_model_lookup_) {
        if (name == connection->destination_name) {
            if(comp == nullptr) {
                break;
            }
            auto destination_bounds = getLocalArea(comp, comp->getLocalBounds());
            int center_x = destination_bounds.getCentreX();
            int left = destination_bounds.getX();
            int right = destination_bounds.getRight();

            int bottom = destination_bounds.getBottom();
            int top = destination_bounds.getY();
            int center_y = destination_bounds.getCentreY();
            editing_button_mod.popup_menu = new juce::PopupMenu();
            editing_button_mod.popup_menu->addItem(1,"Off",true,false);
            editing_button_mod.popup_menu->addItem(2,"On",true,false);
            editing_button_mod.modulation = connection->state;
            editing_button_mod.paramID = comp->getComponentID().fromLastOccurrenceOf("_", false, false);
            static juce::var nullVar;
            if(editing_button_mod.modulation.getProperty(editing_button_mod.paramID)!=nullVar) {
                int ticked  = static_cast <int>(editing_button_mod.modulation.getProperty(editing_button_mod.paramID)) + 1;
                // editing_comboBox_mod.popup_menu.s
                for (juce::PopupMenu::MenuItemIterator iterator (*editing_button_mod.popup_menu, true); iterator.next();)
                {
                    auto& item = iterator.getItem();

                    if (item.itemID != 0)
                        item.isTicked = (item.itemID == ticked);
                }

            }
            setStateModulationValues(connection->source_name, connection->destination_name, connection->state);
           editing_button_mod.popup_menu->showMenuAsync(juce::PopupMenu::Options().withTargetComponent (comp), [this](int idx) {
                    if(idx == 0) return;
                    editing_button_mod.modulation.setProperty(editing_button_mod.paramID, idx-1,nullptr);

            });
        }
    }
}

void ModulationManager::createModulationSlider(std::string name, SynthSlider *slider, bool poly) {
    std::unique_ptr<ModulationDestination> destination = std::make_unique<ModulationDestination>(slider);
    modulation_destinations_->addAndMakeVisible(destination.get());
    destination->setRectangle(slider->isTextOrCurve());
    destination->setRotary(slider->isRotary());
    destination->setSizeMultiple(slider->getKnobSizeScale());

    destination_lookup_[name] = destination.get();
    all_destinations_.push_back(std::move(destination));
}

ModulationManager::~ModulationManager() { if (editing_state_component_ != nullptr) delete editing_state_component_; }

void ModulationManager::resized() {
    float meter_thickness = findValue(Skin::kKnobModMeterArcThickness);
    juce::Colour meter_center_color = findColour(Skin::kModulationMeter, true);
    juce::Colour meter_left_color = findColour(Skin::kModulationMeterLeft, true);
    juce::Colour meter_right_color = findColour(Skin::kModulationMeterRight, true);

    editing_rotary_amount_quad_.setColor(meter_center_color);
    editing_rotary_amount_quad_.setAltColor(meter_center_color);
    editing_rotary_amount_quad_.setModColor(meter_center_color);
    editing_linear_amount_quad_.setColor(meter_center_color);
    editing_linear_amount_quad_.setAltColor(meter_center_color);
    editing_linear_amount_quad_.setModColor(meter_center_color);

    for (auto &rotary_meter_group: rotary_meters_) {
        rotary_meter_group.second->setThickness(meter_thickness);
        rotary_meter_group.second->setModColor(meter_center_color);
        rotary_meter_group.second->setColor(meter_left_color);
        rotary_meter_group.second->setAltColor(meter_right_color);
    }

    for (auto &linear_meter_group: linear_meters_) {
        linear_meter_group.second->setModColor(meter_center_color);
        linear_meter_group.second->setColor(meter_left_color);
        linear_meter_group.second->setAltColor(meter_right_color);
    }

    modulation_destinations_->setBounds(getLocalBounds());
    // modulation_source_meters_->setBounds(getLocalBounds());

    updateModulationMeterLocations();

    juce::Colour meter_control = findColour(Skin::kModulationMeterControl, true);
    current_modulator_quad_.setColor(meter_control);
    drag_quad_.setColor(meter_control);
    drag_quad_.setAltColor(findColour(Skin::kWidgetBackground, true));

    modulation_expansion_box_->setColor(findColour(Skin::kBody, true));

    juce::Colour lighten_screen = findColour(Skin::kLightenScreen, true);
    float rounding = parent_->findValue(Skin::kLabelBackgroundRounding);

    for (auto &rotary_destination_group: rotary_destinations_)
        rotary_destination_group.second->setColor(lighten_screen);

    for (auto &linear_destination_group: linear_destinations_) {
        linear_destination_group.second->setColor(lighten_screen);
        linear_destination_group.second->setRounding(rounding);
    }
    for (auto &button_group: button_destinations_) {
        button_group.second->setColor(lighten_screen);
        button_group.second->setRounding(rounding);
    }
    SynthSection::resized();
    clearModulationSource();
    positionModulationAmountSliders();
}

void ModulationManager::parentHierarchyChanged() {
    SynthSection::parentHierarchyChanged();
    //  if (!modulation_source_readouts_.empty())
    //    return;

    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    //  for (auto& mod_button : modulation_buttons_) {
    //    modulation_source_readouts_[mod_button.first] = parent->getSynth()->getStatusOutput(mod_button.first);
    //    smooth_mod_values_[mod_button.first] = 0.0f;
    //    active_mod_values_[mod_button.first] = false;
    //  }
    //
    //  num_voices_readout_ = parent->getSynth()->getStatusOutput("num_voices");
}

void ModulationManager::updateModulationMeterLocations() {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();

    for (auto &meter: meter_lookup_) {
        SynthSlider *model = slider_model_lookup_[meter.first];
        if (model)
            meter.second->setBounds(getLocalArea(model, model->getModulationMeterBounds()));

        if (parent) {
            int num_modulations = parent->getSynth()->getNumModulations(meter.first);
            meter.second->setModulated(num_modulations);
            meter.second->setVisible(num_modulations);
        }
    }
}

void ModulationManager::modulationAmountChanged(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    std::string slider_name = slider->getComponentID().toStdString();
    std::string source_name = current_modulator_->getComponentID().toStdString();
    setModulationValues(source_name, slider_name,
                        slider->getModulationAmount(), slider->isModulationBipolar(),
                        slider->isModulationStereo(), slider->isModulationBypassed());
    modulation_buttons_[source_name]->repaint();
}

void ModulationManager::modulationRemoved(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    std::string slider_name = slider->getComponentID().toStdString();
    std::string source_name = current_modulator_->getComponentID().toStdString();

    removeModulation(source_name, slider_name);
    modulation_buttons_[source_name]->repaint();
}

void ModulationManager::modulationDisconnected(bitklavier::ModulationConnection *connection, bool last) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (current_modulator_ == nullptr)
        return;

    if (meter_lookup_.count(connection->destination_name)) {
        meter_lookup_[connection->destination_name]->setModulated(!last);
        meter_lookup_[connection->destination_name]->setVisible(!last);
    }
}

void ModulationManager::modulationDisconnected(bitklavier::StateConnection *connection, bool last) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (current_modulator_ == nullptr)
        return;

    //    if (meter_lookup_.count(connection->destination_name)) {
    //        meter_lookup_[connection->destination_name]->setModulated(!last);
    //        meter_lookup_[connection->destination_name]->setVisible(!last);
    //    }
}

//gets called whenever a modulation button is dragged from
void ModulationManager::modulationSelected(ModulationButton *source) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    for (auto &button: modulation_buttons_)
        button.second->setActiveModulation(button.second == source);

    current_modulator_ = source;
    for (auto &hover_slider: modulation_hover_sliders_)
        hover_slider->makeVisible(false);
    for (auto &hover_indicator: modulation_hover_indicators_)
        hover_indicator->makeVisible(false);

    makeCurrentModulatorAmountsVisible();
    makeCurrentStateModulatorsVisible();
    setModulationAmounts();
    positionModulationAmountSliders();
}

void ModulationManager::modulationClicked(ModulationButton *source) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    hideUnusedHoverModulations();
    positionModulationAmountSliders();
}

void ModulationManager::modulationCleared() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    makeCurrentModulatorAmountsVisible();
}

bool ModulationManager::hasFreeConnection() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    bitklavier::ModulationConnectionBank &bank = parent->getSynth()->getModulationBank();
    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        bitklavier::ModulationConnection *connection = bank.atIndex(i);
        if (connection->source_name.empty() && connection->destination_name.empty())
            return true;
    }

    return false;
}


void ModulationManager::componentAdded() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);

    FullInterface *full = findParentComponentOfClass<FullInterface>();
    if(full == nullptr)
        return;
    auto sliders = full->getAllSliders();
    auto modulatable_buttons = full->getAllButtons();
    auto mod_buttons = full->getAllModulationButtons();
    auto mod_state_components = full->getAllStateModulatedComponents();
    auto mod_combo_box = full->getAllComboBox();
    std::erase_if(sliders, [](const std::pair<std::string, SynthSlider *> &element) {
        return !element.second->isVisible();
    });

    std::unordered_set<std::string> uniquePrefixes;

    for (const auto &[key, value]: sliders) {
        size_t pos = key.find('_');
        if (pos != std::string::npos)
            uniquePrefixes.insert(key.substr(0, pos));
    }

    std::unordered_set<std::string> uniqueModPrefixes;
    for (const auto &[key, value]: mod_buttons) {
        size_t pos = key.find('_');
        if (pos != std::string::npos)
            uniqueModPrefixes.insert(key.substr(0, pos));
    }
    //should only have one prep view on at a time
    /*
     * checking to make sure that there is only one prep view open at a time
     * would also fail if there is a component without a UUID attached
     * need to add setComponentID() for any sections within a preparationView, to the same as the owning preparationView
     * see peak_meter_section.cpp for an example
     */
    jassert(uniquePrefixes.size()<=1);
    jassert(uniqueModPrefixes.size()<=1);
    std::string _dst, _src;
    if (!uniquePrefixes.empty())
        _dst = *uniquePrefixes.begin();
    if (!uniqueModPrefixes.empty())
        _src = *uniqueModPrefixes.begin();

    auto dest = base->getActivePianoValueTree().getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::uuid, juce::String(_dst));
    auto src =  base->getActivePianoValueTree().getChildWithName(IDs::PREPARATIONS).getChildWithProperty(IDs::uuid, juce::String(_src));
    if (dest.isValid() && src.isValid()) {
        auto *interface = findParentComponentOfClass<SynthGuiInterface>();
        //check the modconnections array first to see if there is psuedoconnection
        auto modconnections =  base->getActivePianoValueTree().getChildWithName(IDs::MODCONNECTIONS);
        bool isConnected = false;

        for (auto modconnection: modconnections) {
            if (modconnection.getProperty(IDs::src, -1) == src.getProperty(IDs::nodeID) && modconnection.
                getProperty(IDs::dest, -1) == dest.getProperty(IDs::nodeID)) {
                isConnected = true;
                break;
            }
        }

        if (!isConnected) {
            sliders.clear();
            modulatable_buttons.clear();
            mod_state_components.clear();
            mod_combo_box.clear();
        }
    } else if (!dest.isValid() && !src.isValid()) {
        sliders.clear();
        modulatable_buttons.clear();
        mod_state_components.clear();
        mod_combo_box.clear();
    }
    for (auto slider : sliders) {
        slider.second->addListener(this);
    }
    full->open_gl_.context.executeOnGLThread([this, &full](juce::OpenGLContext &openGLContext) {
                                                 for (auto &multiquad: rotary_destinations_) {
                                                     multiquad.second->destroy(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: rotary_meters_) {
                                                     multiquad.second->destroy(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: linear_meters_) {
                                                     multiquad.second->destroy(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: linear_destinations_) {
                                                     multiquad.second->destroy(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: button_destinations_) {
                                                     multiquad.second->destroy(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: button_meters_) {
                                                     multiquad.second->destroy(full->open_gl_);
                                                 }
                                             },
                                             true); {
        juce::ScopedLock lock(full->open_gl_critical_section_);
        rotary_destinations_.clear();
        rotary_meters_.clear();
        linear_destinations_.clear();
        linear_meters_.clear();
        modulation_buttons_.clear();
        button_destinations_.clear();
        button_meters_.clear();
        button_model_lookup_.clear();
        slider_model_lookup_.clear();
        state_model_lookup_.clear();
        combo_box_lookup.clear();
        modulation_callout_buttons_.clear();
        meter_lookup_.clear();
        destination_lookup_.clear();
        all_destinations_.clear();
        clearTemporaryModulation();
        clearTemporaryHoverModulation();
        temporarily_set_destination_ = nullptr;
        temporarily_set_hover_slider_ = nullptr;
        temporarily_set_synth_slider_ = nullptr;
        temporarily_set_state_combo_box_ = nullptr;
        modulationCleared();
        //count things up
        modulation_buttons_ = mod_buttons;
        for (auto &modulation_button: modulation_buttons_) {
            modulation_button.second->addListener(this);

            modulation_callout_buttons_[modulation_button.first] = std::make_unique<ExpandModulationButton>();
            addChildComponent(modulation_callout_buttons_[modulation_button.first].get());
            // addOpenGlComponent(modulation_callout_buttons_[modulation_button.first]->getGlComponent());
            modulation_callout_buttons_[modulation_button.first]->addListener(this);
        }

        slider_model_lookup_ = sliders;
        for (auto &slider: slider_model_lookup_) {
            //        if (mono_modulations[slider.first]) {
            std::string name = slider.first;
            bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve();
            juce::Viewport *viewport = slider.second->findParentComponentOfClass<juce::Viewport>();
            if (rotary)
                num_rotary_meters[viewport] = num_rotary_meters[viewport] + 1;
            else
                num_linear_meters[viewport] = num_linear_meters[viewport] + 1;
        }

        button_model_lookup_ = modulatable_buttons;
        for (auto &button: button_model_lookup_) {
            std::string name = button.first;
            //bool rotary = button.second->isRotary() && !button.second->isTextOrCurve();
            juce::Viewport *viewport = button.second->findParentComponentOfClass<juce::Viewport>();
            num_button_meters[viewport] = num_button_meters[viewport] + 1;
        }
        state_model_lookup_ = mod_state_components;
        for (auto &[name,comp]: state_model_lookup_) {
            juce::Viewport *viewport = comp->findParentComponentOfClass<juce::Viewport>();
            num_button_meters[viewport] = num_button_meters[viewport] + 1;
        }
        combo_box_lookup = mod_combo_box;
        for (auto &[name, comp] : combo_box_lookup) {
            juce::Viewport *viewport = comp->findParentComponentOfClass<juce::Viewport>();
            num_button_meters[viewport] = num_button_meters[viewport]+1;
        }
        for (auto &buttons: num_button_meters) {
            button_destinations_[buttons.first] = std::make_unique<OpenGlMultiQuad>(
                buttons.second, Shaders::kRoundedRectangleFragment);
            button_destinations_[buttons.first]->setTargetComponent(this);
            button_destinations_[buttons.first]->setScissorComponent(buttons.first);
            button_destinations_[buttons.first]->setAlpha(0.0f, true);
            button_meters_[buttons.first] = std::make_unique<OpenGlMultiQuad>(buttons.second,
                                                                              Shaders::kLinearModulationFragment);
            button_meters_[buttons.first]->setTargetComponent(this);
            button_meters_[buttons.first]->setScissorComponent(buttons.first);
        }

        //init the overlay parameters
        for (auto &rotary_meters: num_rotary_meters) {
            //DBG("num rotary" + juce::String (rotary_meters.second));
            rotary_destinations_[rotary_meters.first] = std::make_unique<OpenGlMultiQuad>(rotary_meters.second,
                Shaders::kCircleFragment);
            rotary_destinations_[rotary_meters.first]->setTargetComponent(this);
            rotary_destinations_[rotary_meters.first]->setScissorComponent(rotary_meters.first);
            rotary_destinations_[rotary_meters.first]->setAlpha(0.0f, true); //DEBUG FIX

            rotary_meters_[rotary_meters.first] = std::make_unique<OpenGlMultiQuad>(rotary_meters.second,
                Shaders::kRotaryModulationFragment);
            rotary_meters_[rotary_meters.first]->setTargetComponent(this);
            rotary_meters_[rotary_meters.first]->setScissorComponent(rotary_meters.first);
        }
        for (auto &linear_meters: num_linear_meters) {
            linear_destinations_[linear_meters.first] = std::make_unique<OpenGlMultiQuad>(linear_meters.second,
                Shaders::kRoundedRectangleFragment);
            linear_destinations_[linear_meters.first]->setTargetComponent(this);
            linear_destinations_[linear_meters.first]->setScissorComponent(linear_meters.first);
            linear_destinations_[linear_meters.first]->setAlpha(0.0f, true);

            linear_meters_[linear_meters.first] = std::make_unique<OpenGlMultiQuad>(linear_meters.second,
                Shaders::kLinearModulationFragment);
            linear_meters_[linear_meters.first]->setTargetComponent(this);
            linear_meters_[linear_meters.first]->setScissorComponent(linear_meters.first);
        }

        //init the acutal meters used and connect them to destinations
        for (auto &slider: slider_model_lookup_) {
            std::string name = slider.first;

            bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve();
            juce::Viewport *viewport = slider.second->findParentComponentOfClass<juce::Viewport>(); //        }
            if (rotary) {
                int index = num_rotary_meters[viewport] - 1;
                num_rotary_meters[viewport] = index;
                createModulationMeter(slider.second, rotary_meters_[viewport].get(), index);
            } else {
                int index = num_linear_meters[viewport] - 1;
                num_linear_meters[viewport] = index;
                createModulationMeter(slider.second, linear_meters_[viewport].get(), index);
            }

            slider.second->addSliderListener(this);
            createModulationSlider(name, slider.second, false);
        }

        for (auto [name,comp] : combo_box_lookup) {
            auto viewport = comp->findParentComponentOfClass<juce::Viewport>();
            int index = num_button_meters[viewport] - 1;
            num_button_meters[viewport] = index;
            std::unique_ptr<ModulationDestination> destination = std::make_unique<ModulationDestination>(comp);
            modulation_destinations_->addAndMakeVisible(destination.get());
            destination_lookup_[name] = destination.get();
            all_destinations_.push_back(std::move(destination));
            comp->addListener(this);
        }
        for (auto [name,comp]: state_model_lookup_) {
            auto viewport = comp->findParentComponentOfClass<juce::Viewport>();
            int index = num_button_meters[viewport] - 1;
            num_button_meters[viewport] = index;
            std::unique_ptr<ModulationDestination> destination = std::make_unique<ModulationDestination>(comp);
            modulation_destinations_->addAndMakeVisible(destination.get());
            destination_lookup_[name] = destination.get();
            all_destinations_.push_back(std::move(destination));
            comp->addListener(this);
        }
        for (auto &button: button_model_lookup_) {
            std::string name = button.first;
            juce::Viewport *viewport = button.second->findParentComponentOfClass<juce::Viewport>();
            int index = num_button_meters[viewport] - 1;
            num_button_meters[viewport] = index;
            std::unique_ptr<ModulationDestination> destination = std::make_unique<ModulationDestination>(button.second);
            modulation_destinations_->addAndMakeVisible(destination.get());
            //            destination->setRectangle(slider->isTextOrCurve());
            //            destination->setRotary(slider->isRotary());
            //            destination->setSizeMultiple(slider->getKnobSizeScale());

            destination_lookup_[name] = destination.get();
            all_destinations_.push_back(std::move(destination));
            button.second->addButtonListener(this);
            //createModulationMeter(button.second, button_meters_[viewport].get(), index);
        }
    }
    full->open_gl_.context.executeOnGLThread([this, &full](juce::OpenGLContext &openGLContext) {
                                                 for (auto &multiquad: rotary_destinations_) {
                                                     multiquad.second->init(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: rotary_meters_) {
                                                     multiquad.second->init(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: linear_meters_) {
                                                     multiquad.second->init(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: linear_destinations_) {
                                                     multiquad.second->init(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: button_destinations_) {
                                                     multiquad.second->init(full->open_gl_);
                                                 }
                                                 for (auto &multiquad: button_meters_) {
                                                     multiquad.second->init(full->open_gl_);
                                                 }
                                             },
                                             true);

    resized();
}

void ModulationManager::startModulationMap(ModulationButton *source, const juce::MouseEvent &e) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (!hasFreeConnection())
        return;

    mouse_drag_position_ = getLocalPoint(source, e.getPosition());
    current_source_ = source;
    dragging_ = true;
    juce::Rectangle<int> global_bounds = getLocalArea(current_source_, current_source_->getLocalBounds());
    juce::Point<int> global_start = global_bounds.getCentre();
    mouse_drag_start_ = global_start;
    modulation_destinations_->setVisible(true);
    int widget_margin = findValue(Skin::kWidgetMargin);

    std::map<juce::Viewport *, int> rotary_indices;
    std::map<juce::Viewport *, int> linear_indices;
    std::map<juce::Viewport *, int> button_indices;
    for (auto &rotary_destination_group: rotary_destinations_)
        rotary_indices[rotary_destination_group.first] = 0;

    for (auto &linear_destination_group: linear_destinations_)
        linear_indices[linear_destination_group.first] = 0;

    for (auto &button_destination_group: button_destinations_)
        button_indices[button_destination_group.first] = 0;

    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    std::string source_name = source->getComponentID().toStdString();
    std::set<std::string> active_destinations;
    std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->getSourceConnections(source_name);
    for (bitklavier::ModulationConnection *connection: connections)
        active_destinations.insert(connection->destination_name);

    for (auto &destination: destination_lookup_) {
        SynthSlider *model = slider_model_lookup_[destination.first];
        if (model == nullptr) {
            if (button_model_lookup_[destination.first]) {
                auto *button = button_model_lookup_[destination.first];
                bool show = current_source_->getComponentID() != juce::String(destination.first) && current_source_->
                            isStateModulation();
                juce::Viewport *viewport = button->findParentComponentOfClass<juce::Viewport>();
                destination.second->setVisible(show);
                destination.second->setActive(active_destinations.count(destination.first));
                destination.second->setMargin(widget_margin);
                juce::Point<int> position = getLocalPoint(button, juce::Point<int>(0, 0));
                juce::Rectangle<int> _bounds = button->getLocalBounds() + position;
                destination.second->setBounds(_bounds);
                if (show) {
                    destination.second->setIndex(button_indices[viewport]);
                    button_indices[viewport] = button_indices[viewport] + 1;
                    setDestinationQuadBounds(destination.second);
                }
            } else if (state_model_lookup_[destination.first]) {
                auto *state = state_model_lookup_[destination.first];
                bool show = current_source_->getComponentID() != juce::String(destination.first) && current_source_->
                            isStateModulation() && destination.second->isShowing() && !destination.second->isActive();
                juce::Viewport *viewport = state->findParentComponentOfClass<juce::Viewport>();
                destination.second->setVisible(show);
                destination.second->setActive(active_destinations.count(destination.first));
                destination.second->setMargin(widget_margin);
                juce::Point<int> position = getLocalPoint(state, juce::Point<int>(0, 0));
                juce::Rectangle<int> _bounds = state->getLocalBounds() + position;
                destination.second->setBounds(_bounds);
                if (show) {
                    destination.second->setIndex(button_indices[viewport]);
                    button_indices[viewport] = button_indices[viewport] + 1;
                    setDestinationQuadBounds(destination.second);
                }
            } else if (combo_box_lookup[destination.first]) {

                auto *combo = combo_box_lookup[destination.first];
                bool show = current_source_->getComponentID() != juce::String(destination.first) && current_source_->
                            isStateModulation();
                juce::Viewport *viewport = combo->findParentComponentOfClass<juce::Viewport>();
                destination.second->setVisible(show);
                destination.second->setActive(active_destinations.count(destination.first));
                destination.second->setMargin(widget_margin);
                juce::Point<int> position = getLocalPoint(combo, juce::Point<int>(0, 0));
                juce::Rectangle<int> _bounds = combo->getLocalBounds() + position;
                destination.second->setBounds(_bounds);
                if (show) {
                    destination.second->setIndex(button_indices[viewport]);
                    button_indices[viewport] = button_indices[viewport] + 1;
                    setDestinationQuadBounds(destination.second);
                }
            }
            continue;
        }
        bool should_show = model->isShowing() && model->getSectionParent()->isActive() &&
                           current_source_->getComponentID() != juce::String(destination.first) && !current_source_->
                           isStateModulation();
        juce::Viewport *viewport = model->findParentComponentOfClass<juce::Viewport>();
        destination.second->setVisible(should_show);
        destination.second->setActive(active_destinations.count(destination.first));
        destination.second->setMargin(widget_margin);

        juce::Point<int> position = getLocalPoint(model, juce::Point<int>(0, 0));
        juce::Rectangle<int> slider_bounds = model->getLocalBounds() + position;
        destination.second->setBounds(slider_bounds);

        juce::Component *extra_target = model->getExtraModulationTarget();
        if (extra_target) {
            juce::Rectangle<int> bounds = destination.second->getFillBounds().toNearestInt() + position;

            juce::Point<int> top_left = getLocalPoint(extra_target, juce::Point<int>(0, 0));
            juce::Rectangle<int> extra_bounds(top_left.x, top_left.y, extra_target->getWidth(),
                                              extra_target->getHeight());
            bounds = bounds.getUnion(extra_bounds);
            destination.second->setBounds(bounds);
        }

        if (should_show) {
            if (destination.second->isRotary()) {
                destination.second->setIndex(rotary_indices[viewport]);
                rotary_indices[viewport] = rotary_indices[viewport] + 1;
            } else {
                destination.second->setIndex(linear_indices[viewport]);
                linear_indices[viewport] = linear_indices[viewport] + 1;
            }
            setDestinationQuadBounds(destination.second);
        }
    }
    //DEBUG FIX
    for (auto &index_count: rotary_indices) {
        rotary_destinations_[index_count.first]->setNumQuads(index_count.second);
        rotary_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
    }

    for (auto &index_count: linear_indices) {
        linear_destinations_[index_count.first]->setNumQuads(index_count.second);
        linear_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
    }
    for (auto &index_count: button_indices) {
        button_destinations_[index_count.first]->setNumQuads(index_count.second);
        button_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
    }
}

void ModulationManager::setDestinationQuadBounds(ModulationDestination *destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    juce::Point<float> top_left = destination->getBounds().getTopLeft().toFloat();
    juce::Rectangle<float> draw_bounds = destination->getLocalBounds().toFloat() + top_left;
    if (!destination->hasExtraModulationTarget())
        draw_bounds = destination->getFillBounds() + top_left;
    float global_width = getWidth();
    float global_height = getHeight();
    float x = 2.0f * draw_bounds.getX() / global_width - 1.0f;
    float y = 1.0f - 2.0f * draw_bounds.getBottom() / global_height;
    float width = 2.0f * draw_bounds.getWidth() / global_width;
    float height = 2.0f * draw_bounds.getHeight() / global_height;

    float offset = destination->isActive() ? -2.0f : 0.0f;
    juce::Viewport *viewport;
    std::visit([&]<typename T0>(const T0& ptr)
 {
     using T = std::decay_t<T0>;

     DBG("destination holds: " << typeid(T).name());

     if (ptr != nullptr)
         viewport = ptr->template findParentComponentOfClass<juce::Viewport>();

 }, destination->destination);

    if (destination->isRotary()) {
        rotary_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
    } else if (destination->isRectangle() || destination->isStateModulated()) {
        button_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
    } else {
        linear_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
    }
}

void ModulationManager::modulationDraggedToHoverSlider(ModulationAmountKnob *hover_slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (hover_slider->isCurrentModulator() || hover_slider->hasAux() || current_modulator_ == nullptr)
        return;

    std::string name = hover_slider->getOriginalName().toStdString();
    std::string source_name = current_modulator_->getComponentID().toStdString();
    bitklavier::ModulationConnection *connection = getConnection(source_name, name);
    if (connection == nullptr) {
        float value = hover_slider->getValue() * 0.5f;
        hover_slider->setValue(0.0f, juce::sendNotificationSync);
        temporarily_set_hover_slider_ = hover_slider;

        connectModulation(source_name, name);
        setModulationValues(source_name, name, value, false, false, false);
        connection = getConnection(source_name, name);

        int new_index = connection->index_in_all_mods;
        addAuxConnection(new_index, hover_slider->index());
        setModulationSliderValues(new_index, value);
    }
}

//this function is constantly called while you are dragging since everything is a component
void ModulationManager::modulationDraggedToComponent(juce::Component *component, bool bipolar) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);

     if (component && current_modulator_ &&
        destination_lookup_.count(component->getComponentID().toStdString())) {
         std::string name = component->getComponentID().toStdString();
         ModulationDestination* destination = destination_lookup_[name];
         std::string source_name = current_modulator_->getComponentID().toStdString();

         std::visit([&]<typename T0>(const T0& ptr)
         {
             using T = std::decay_t<T0>;

             if (ptr == nullptr)
                 return;

             if constexpr (std::is_same_v<T, SliderPtr> )
             {
                 if (source_name.contains("state"))
                     return;
                 if (getConnection(source_name, name) == nullptr)
                 {
                     // if (auto sp = std::get_if<SliderPtr>(&destination)) {
                     //     if (sp->get() == nullptr) return; // component gone
                     //     auto* slider = sp->get();
                     // }
                     float percent = ptr->valueToProportionOfLength(ptr->getValue());
                     float modulation_amount = 1.0f - percent;
                     if (bipolar)
                         modulation_amount = std::min(modulation_amount, percent) * 2.0f;
                     modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

                     temporarily_set_destination_   = destination;
                     temporarily_set_synth_slider_ = slider_model_lookup_[name];

                     connectModulation(source_name, name);
                     setModulationValues(source_name, name, modulation_amount, bipolar, false, false);
                     destination->setActive(true);
                     setDestinationQuadBounds(destination);

                     SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
                     auto connections = parent->getSynth()->getDestinationConnections(name);

                     for (auto* connection : connections)
                     {
                         if (connection->source_name == source_name &&
                             connection->destination_name == name)
                         {
                             int index = connection->index_in_all_mods;
                             selected_modulation_sliders_[index]->my0to1amt = connection->getScaling();
                             showModulationAmountOverlay(selected_modulation_sliders_[index].get());
                         }
                     }

                     setVisibleMeterBounds();
                     makeModulationsVisible(ptr, true);
                 }
                 else
                 {
                     modulationsChanged(name);
                 }
             }
             else if constexpr (std::is_same_v<T, ButtonPtr>)
             {
                 if (!source_name.contains("state"))
                     return;
                 if (getStateConnection(source_name, name) == nullptr)
                 {
                     temporarily_set_destination_ = destination;
                     temporarily_set_button_      = button_model_lookup_[name];
                     connectStateModulation(source_name, name);
                     makeModulationsVisible(ptr, true);
                 }
                 else
                 {
                     modulationsChanged(name);
                 }
             }
             else if constexpr (std::is_same_v<T, StatePtr> )
             {
                 if (!source_name.contains("state"))
                     return;
                 if (getStateConnection(source_name, name) == nullptr)
                 {
                     temporarily_set_destination_      = destination;
                     temporarily_set_state_component_  = state_model_lookup_[name];
                     connectStateModulation(source_name, name);
                     setStateModulationValues(source_name, name, ptr->modulationState);
                     makeModulationsVisible(ptr, true);
                 }
                 else
                 {
                     modulationsChanged(name);
                 }
             }
             else if constexpr (std::is_same_v<T, ComboPtr>)
             {
                 if (!source_name.contains("state"))
                     return;
                 if (getStateConnection(source_name, name) == nullptr)
                 {
                     temporarily_set_destination_     = destination;
                     temporarily_set_state_combo_box_ = combo_box_lookup[name];
                     connectStateModulation(source_name, name);
                     // setStateModulationValues(source_name, name, ptr->modulationState);
                     makeModulationsVisible(ptr, true);
                 }
                 else
                 {
                     modulationsChanged(name);
                 }
             }
         }, destination->destination);
     }
}

void ModulationManager::setTemporaryModulationBipolar(juce::Component *component, bool bipolar) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (current_modulator_ == nullptr || component != temporarily_set_destination_ || component == nullptr)
        return;

    std::string source_name = current_modulator_->getComponentID().toStdString();
    std::string name = component->getComponentID().toStdString();
    ModulationDestination *destination = destination_lookup_[name];
    // SynthSlider *slider =std::get<SynthSlider*>( destination->destination);
    if (auto sp = std::get_if<SliderPtr>(&destination->destination)) {
        if (sp->getComponent() == nullptr) return; // component gone
        auto* slider = sp->getComponent();
        const float percent = slider->valueToProportionOfLength(slider->getValue());
        float modulation_amount = 1.0f - percent;
        if (bipolar)
            modulation_amount = std::min(modulation_amount, percent) * 2.0f;
        modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);
        int index = getModulationIndex(source_name, name);
        setModulationValues(source_name, name, modulation_amount, bipolar, false, false);
        temporarily_set_bipolar_ = bipolar;
        showModulationAmountOverlay(selected_modulation_sliders_[index].get());
    }
}

void ModulationManager::clearTemporaryModulation() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (temporarily_set_destination_ && current_modulator_) {
        temporarily_set_destination_->setActive(false);
        setDestinationQuadBounds(temporarily_set_destination_);
        temporarily_set_destination_ = nullptr;
        std::string source_name = current_modulator_->getComponentID().toStdString();
        if (temporarily_set_synth_slider_ != nullptr) {
            removeModulation(source_name, temporarily_set_synth_slider_->getComponentID().toStdString());
        } else if (temporarily_set_button_ != nullptr) {
            removeModulation(source_name, temporarily_set_button_->getComponentID().toStdString());
        } else if (temporarily_set_state_component_ != nullptr) {
            removeModulation(source_name, temporarily_set_state_component_->getComponentID().toStdString());
        }
        temporarily_set_button_ = nullptr;
        temporarily_set_synth_slider_ = nullptr;
        temporarily_set_state_component_ = nullptr;
        hideModulationAmountOverlay();
    }
}

void ModulationManager::clearTemporaryHoverModulation() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (temporarily_set_hover_slider_ && current_modulator_) {
        std::string name = temporarily_set_hover_slider_->getOriginalName().toStdString();

        std::string source_name = current_modulator_->getComponentID().toStdString();
        removeModulation(source_name, temporarily_set_hover_slider_->getOriginalName().toStdString());
        temporarily_set_hover_slider_ = nullptr;
    }
}

void ModulationManager::modulationDragged(const juce::MouseEvent &e) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (!dragging_)
        return;

    mouse_drag_position_ = getLocalPoint(current_source_, e.getPosition());
    juce::Component *component = getComponentAt(mouse_drag_position_.x, mouse_drag_position_.y);
       bool bipolar = e.mods.isAnyModifierKeyDown();
    // if you've set the destination on one parameter that isn't the current one
    // (if you dragged quickly between two parameters) clear the temporary mod
    // before connecting
    if (temporarily_set_destination_ && temporarily_set_destination_ != component)
        clearTemporaryModulation();
    if (temporarily_set_hover_slider_ && temporarily_set_hover_slider_ != component)
        clearTemporaryHoverModulation();
    else if (temporarily_set_synth_slider_ && temporarily_set_bipolar_ != bipolar)
        setTemporaryModulationBipolar(component, bipolar);

    // if (hover_knob)
    //   return;// modulationDraggedToHoverSlider(hover_knob);
    // else
    modulationDraggedToComponent(component, bipolar);
}

void ModulationManager::modulationWheelMoved(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (!dragging_ || current_modulator_ == nullptr || temporarily_set_destination_ == nullptr)
        return;

    juce::MouseEvent new_event(e.source, e.position, juce::ModifierKeys(), e.pressure, e.orientation, e.rotation,
                               e.tiltX, e.tiltY, e.eventComponent, e.originalComponent, e.eventTime,
                               e.mouseDownPosition,
                               e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
    std::string source_name = current_modulator_->getComponentID().toStdString();
    std::string destination_name = temporarily_set_destination_->getComponentID().toStdString();
    int index = getModulationIndex(source_name, destination_name);
    if (index >= 0)
        selected_modulation_sliders_[index]->mouseWheelMove(new_event, wheel);
}

void ModulationManager::endModulationMap() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_hover_slider_ = nullptr;
    dragging_ = false;

    setModulationAmounts();
    positionModulationAmountSliders();
    current_source_ = nullptr;
    for (auto &rotary_destination_group: rotary_destinations_)
        rotary_destination_group.second->setAlpha(0.0f);

    for (auto &linear_destination_group: linear_destinations_)
        linear_destination_group.second->setAlpha(0.0f);

    for (auto &button_destination_group: button_destinations_)
        button_destination_group.second->setAlpha(0.0f);
    modulation_destinations_->setVisible(false);
    drag_quad_.setThickness(0.0f, true);
    hideModulationAmountOverlay();
}

void ModulationManager::modulationLostFocus(ModulationButton *source) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    source->setActiveModulation(false);
    clearModulationSource();
}

void ModulationManager::clearModulationSource() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (current_modulator_) {
        for (auto &selected_slider: selected_modulation_sliders_)
            selected_slider->makeVisible(false);
        for (auto &selected_indicator: selected_modulation_indicators_)
            selected_indicator->makeVisible(false);
    }
    for (auto &hover_slider: modulation_hover_sliders_) {
        hover_slider->makeVisible(false);
        hover_slider->redoImage();
    }
    for (auto &hover_sliders: modulation_hover_indicators_)
        hover_sliders->makeVisible(false);
    current_modulator_ = nullptr;
    setModulationAmounts();
}

void ModulationManager::disconnectModulation(ModulationIndicator *indicator) {
    bitklavier::StateConnection *connection = getConnectionForModulationIndicator(indicator);
    while (connection && !connection->source_name.empty() && !connection->destination_name.empty()) {
        removeStateModulation(connection->source_name, connection->destination_name);
        connection = getConnectionForModulationIndicator(indicator);
    }

    setModulationAmounts();
}

void ModulationManager::disconnectModulation(ModulationAmountKnob *modulation_knob) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);

    bitklavier::ModulationConnection *connection = getConnectionForModulationSlider(modulation_knob);
    while (connection && !connection->source_name.empty() && !connection->destination_name.empty()) {
        removeModulation(connection->source_name, connection->destination_name);
        connection = getConnectionForModulationSlider(modulation_knob);
    }

    setModulationAmounts();
}

void ModulationManager::setModulationSettings(ModulationAmountKnob *modulation_knob) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    bitklavier::ModulationConnection *connection = getConnectionForModulationSlider(modulation_knob);
    float value = modulation_knob->getValue();
    bool bipolar = modulation_knob->isBipolar();
    bool stereo = modulation_knob->isStereo();
    bool bypass = modulation_knob->isBypass();

    int index = modulation_knob->index();
    modulation_amount_sliders_[index]->setBipolar(bipolar);
    modulation_amount_sliders_[index]->setStereo(stereo);
    modulation_amount_sliders_[index]->setBypass(bypass);
    modulation_hover_sliders_[index]->setBipolar(bipolar);
    modulation_hover_sliders_[index]->setStereo(stereo);
    modulation_hover_sliders_[index]->setBypass(bypass);
    selected_modulation_sliders_[index]->setBipolar(bipolar);
    selected_modulation_sliders_[index]->setStereo(stereo);
    selected_modulation_sliders_[index]->setBypass(bypass);

    setModulationValues(connection->source_name, connection->destination_name, value, bipolar, stereo, bypass);
}

void ModulationManager::setModulationBypass(ModulationAmountKnob *modulation_knob, bool bypass) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    setModulationSettings(modulation_knob);
}

void ModulationManager::setModulationBipolar(ModulationAmountKnob *modulation_knob, bool bipolar) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    setModulationSettings(modulation_knob);
}

void ModulationManager::setModulationStereo(ModulationAmountKnob *modulation_knob, bool stereo) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    setModulationSettings(modulation_knob);
}

void ModulationManager::initOpenGlComponents(OpenGlWrapper &open_gl) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    drag_quad_.init(open_gl);
    modulation_expansion_box_->init(open_gl);
    // modulation_source_meters_->init(open_gl);
    for (auto &rotary_destination_group: rotary_destinations_)
        rotary_destination_group.second->init(open_gl);

    for (auto &linear_destination_group: linear_destinations_)
        linear_destination_group.second->init(open_gl);

    for (auto &rotary_meter_group: rotary_meters_)
        rotary_meter_group.second->init(open_gl);

    for (auto &linear_meter_group: linear_meters_)
        linear_meter_group.second->init(open_gl);

    SynthSection::initOpenGlComponents(open_gl);
}

void ModulationManager::drawModulationDestinations(OpenGlWrapper &open_gl) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    for (auto &rotary_destination_group: rotary_destinations_)
        rotary_destination_group.second->render(open_gl, true);

    for (auto &linear_destination_group: linear_destinations_)
        linear_destination_group.second->render(open_gl, true);
    for (auto &button_destination_group: button_destinations_) {
        button_destination_group.second->render(open_gl, true);
    }
}

void ModulationManager::drawCurrentModulator(OpenGlWrapper &open_gl) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    juce::Component *component = current_modulator_;
    if (component) {
        current_modulator_quad_.setTargetComponent(component);
        current_modulator_quad_.setAlpha(1.0f);
    } else {
        current_modulator_quad_.setAlpha(0.0f);
        current_modulator_quad_.setTargetComponent(nullptr);
    }

    current_modulator_quad_.setThickness(dragging_ ? 2.6f : 1.0f);
    current_modulator_quad_.render(open_gl, true);
}

void ModulationManager::drawDraggingModulation(OpenGlWrapper &open_gl) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    static constexpr float kRadiusWidthRatio = 0.022f;
    static constexpr float kThicknessWidthRatio = 0.003f;
    if (current_source_ == nullptr || temporarily_set_destination_ || temporarily_set_hover_slider_)
        return;

    float mod_percent = 50.f; //modulation_source_readouts_[current_source_->getComponentID().toStdString()]->value();
    float draw_radius = kRadiusWidthRatio * getWidth();
    float radius_x = draw_radius / getWidth();
    float radius_y = draw_radius / getHeight();
    float x = mouse_drag_position_.x * 2.0f / getWidth() - 1.0f;
    float y = -mouse_drag_position_.y * 2.0f / getHeight() + 1.0f;

    juce::Colour widget_background = findColour(Skin::kWidgetBackground, true);
    juce::Colour control = findColour(Skin::kModulationMeterControl, true);
    drag_quad_.setAltColor(widget_background.interpolatedWith(control, mod_percent));
    drag_quad_.setQuad(0, x - radius_x, y - radius_y, 2.0f * radius_x, 2.0f * radius_y);
    drag_quad_.setThickness(getWidth() * kThicknessWidthRatio);
    drag_quad_.render(open_gl, true);
}

void ModulationManager::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
    if (!animate)
        return;
    // juce::ScopedLock lock(open_gl_critical_section_);
    drawCurrentModulator(open_gl);
    for (auto &callout_button: modulation_callout_buttons_) {
        if (callout_button.second->isVisible() && !callout_button.second->isInit())
            callout_button.second->renderSliderQuads(open_gl, animate);
    }

    OpenGlComponent::setViewPort(this, open_gl);
    drawModulationDestinations(open_gl);
    //
    //  juce::Colour first_color = findColour(Skin::kWidgetPrimary1, true);
    //  juce::Colour second_color = findColour(Skin::kWidgetPrimary2, true);
    //
    //  modulation_source_meters_->setAdditiveBlending(second_color.getBrightness() > 0.5f);
    //  modulation_source_meters_->setColor(second_color);
    //  renderSourceMeters(open_gl, 1);
    //  modulation_source_meters_->setAdditiveBlending(first_color.getBrightness() > 0.5f);
    //  modulation_source_meters_->setColor(first_color);
    //  renderSourceMeters(open_gl, 0);
    //  updateSmoothModValues();
    //

    SynthSection::renderOpenGlComponents(open_gl, animate);
    for (auto &ind: modulation_indicators_) {
        if (ind->isVisible())
            ind->render(open_gl, animate);
    }
    for (auto &ind: modulation_hover_indicators_) {
        if (ind->isVisible())
            ind->render(open_gl, animate);
    }
    for (auto &ind: selected_modulation_indicators_) {
        if (ind->isVisible())
            ind->render(open_gl, animate);
    }
    editing_rotary_amount_quad_.render(open_gl, animate);
    editing_linear_amount_quad_.render(open_gl, animate);
    drawDraggingModulation(open_gl);
}

void ModulationManager::renderMeters(OpenGlWrapper &open_gl, bool animate) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (!animate)
        return;
    // juce::ScopedLock lock(open_gl_critical_section_);
    int num_voices = 1;
    //  if (num_voices_readout_)
    //    num_voices = std::max<float>(0.0f, num_voices_readout_->value()[0]);
    for (auto &meter: meter_lookup_) {
        SynthSlider *slider = slider_model_lookup_[meter.first];
        bool show = meter.second->isModulated() && showingInParents(slider) && slider->isActive();
        meter.second->setActive(show);
        if (show)
            meter.second->updateDrawing(num_voices);
    }

    OpenGlComponent::setViewPort(this, open_gl);
    for (auto &rotary_meter_group: rotary_meters_)
        rotary_meter_group.second->render(open_gl, animate);

    for (auto &linear_meter_group: linear_meters_)
        linear_meter_group.second->render(open_gl, animate);
}

//void ModulationManager::beginModulationEdit(ModulationIndicator * ind)
//{
//
//}
void ModulationManager::preparationClosed(bool isModulation) {
    for (auto &ind: modulation_indicators_) {
        ind->setVisible(false);
    }
    for (auto &ind: modulation_hover_indicators_) {
        ind->setVisible(false);
    }
    for (auto &ind: selected_modulation_indicators_) {
        ind->setVisible(false);
    }
    editing_rotary_amount_quad_.setVisible(false);
    editing_linear_amount_quad_.setVisible(false);
    if (editing_state_component_)
        editing_state_component_->setVisible(false);
    if (isModulation) {
        // for (auto& slider : modulation_amount_sliders_) {
        //     slider->makeVisible(false);
        //     slider->redoImage();
        // }
    } else {
        // for (auto& selected_slider : selected_modulation_sliders_) {
        //     selected_slider->makeVisible(false);
        //     selected_slider->redoImage();
        // }
        for (auto &hover_slider: modulation_hover_sliders_) {
            hover_slider->makeVisible(false);
        }
    }

    componentAdded();
}

void ModulationManager::renderSourceMeters(OpenGlWrapper &open_gl, int index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    int i = 0;
    float width = getWidth();
    float height = getHeight();
    ////  for (auto& mod_readout : modulation_source_readouts_) {
    ////    ModulationButton* button = modulation_buttons_[mod_readout.first];
    ////    float readout_value = mod_readout.second->value()[index];
    ////
    ////    float clamped_value = bitklavier::utils::clamp(readout_value, 0.0f, 1.0f);
    ////    if (!active_mod_values_[mod_readout.first] && !mod_readout.second->isClearValue(readout_value))
    ////      smooth_mod_values_[mod_readout.first].set(index, clamped_value);
    ////    float smooth_value = smooth_mod_values_[mod_readout.first][index];
    ////
    ////    juce::Rectangle<int> bounds = getLocalArea(button, button->getMeterBounds());
    ////    float left = 2.0f * ((bounds.getX() - 1.0f) / width) - 1.0f + kModSourceMeterBuffer;
    ////    float w = 2.0f * bounds.getWidth() / width - 2.0f * kModSourceMeterBuffer;
    ////    float h = 2.0f * bounds.getHeight() / height - 2.0f * kModSourceMeterBuffer;
    ////    float y = 1.0f - 2.0f * bounds.getY() / height - kModSourceMeterBuffer;
    ////    float y_center = y - h * (1.0f - clamped_value);
    ////    float smooth_y_center = y - h * (1.0f - smooth_value);
    ////
    ////    float top = std::min(y_center, smooth_y_center) - kModSourceMinRadius;
    ////    float bottom = std::max(y_center, smooth_y_center) + kModSourceMinRadius;
    ////
    ////    bool active = button->isActiveModulation() || button->hasAnyModulation();
    ////    if (w <= 0.0f || mod_readout.second->isClearValue(readout_value) || !showingInParents(button) || !active) {
    ////      left = -2.0f;
    ////      top = -2.0f;
    ////      bottom = -2.0f;
    ////    }
    //
    ////    modulation_source_meters_->positionBar(i, left, top, w, bottom - top);
    //
    //    i++;
    //  }

    modulation_source_meters_->render(open_gl, true);
}

void ModulationManager::updateSmoothModValues() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    static constexpr float kTimeDecayScale = 60.0f;
    long long current_milliseconds = juce::Time::currentTimeMillis();
    long long delta_milliseconds = current_milliseconds - last_milliseconds_;
    last_milliseconds_ = current_milliseconds;

    float seconds = delta_milliseconds / 1000.0f;
    float decay = std::max(std::min(kModSmoothDecay * seconds * kTimeDecayScale, 1.0f), 0.0f);

    //  for (auto& mod_readout : modulation_source_readouts_) {
    //    float readout_value = mod_readout.second->value();
    //    float clamped_value = bitklavier::utils::clamp(readout_value, 0.0f, 1.0f);
    //    float smooth_value = smooth_mod_values_[mod_readout.first];
    //    bool active = !mod_readout.second->isClearValue(readout_value);
    //    active_mod_values_[mod_readout.first] = active;
    //    if (active)
    //      smooth_mod_values_[mod_readout.first] = bitklavier::utils::interpolate(smooth_value, clamped_value, decay);
    //  }
}

void ModulationManager::destroyOpenGlComponents(OpenGlWrapper &open_gl) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthSection::destroyOpenGlComponents(open_gl);

    drag_quad_.destroy(open_gl);
    modulation_expansion_box_->destroy(open_gl);
    //  modulation_source_meters_->destroy(open_gl);
    for (auto &rotary_destination_group: rotary_destinations_)
        rotary_destination_group.second->destroy(open_gl);

    for (auto &linear_destination_group: linear_destinations_)
        linear_destination_group.second->destroy(open_gl);

    for (auto &rotary_meter_group: rotary_meters_)
        rotary_meter_group.second->destroy(open_gl);

    for (auto &linear_meter_group: linear_meters_)
        linear_meter_group.second->destroy(open_gl);
}

void ModulationManager::showModulationAmountOverlay(ModulationAmountKnob *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    bitklavier::ModulationConnection *connection = getConnection(slider->index());
    if (connection == nullptr || meter_lookup_.count(connection->destination_name) == 0)
        return;

    ModulationMeter *meter = meter_lookup_[connection->destination_name].get();
    if (!meter->destination()->isShowing())
        return;

    if (meter->isRotary()) {
        editing_rotary_amount_quad_.setTargetComponent(meter);
        editing_rotary_amount_quad_.setAdditive(false);
        meter->setAmountQuadVertices(editing_rotary_amount_quad_);
        meter->setModulationAmountQuad(editing_rotary_amount_quad_, slider->get0to1value(), slider->isBipolar());

        editing_rotary_amount_quad_.setThickness(2.0f);
        editing_rotary_amount_quad_.setAlpha(1.0f);
        editing_rotary_amount_quad_.setActive(true);
    } else {
        editing_linear_amount_quad_.setTargetComponent(meter);
        editing_linear_amount_quad_.setAdditive(false);
        meter->setAmountQuadVertices(editing_linear_amount_quad_);
        meter->setModulationAmountQuad(editing_linear_amount_quad_, slider->get0to1value(), slider->isBipolar());

        editing_linear_amount_quad_.setAlpha(1.0f);
        editing_linear_amount_quad_.setActive(true);
    }
}

void ModulationManager::hideModulationAmountOverlay() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (changing_hover_modulation_)
        return;

    editing_rotary_amount_quad_.setAlpha(0.0f);
    editing_linear_amount_quad_.setAlpha(0.0f);
}
void ModulationManager::hoverStarted(OpenGLComboBox *component) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (changing_hover_modulation_)
        return;

    if (!enteringHoverValue())
        makeModulationsVisible(component, true);
    if(editing_state_component_)
        editing_state_component_->setVisible(false);
}

void ModulationManager::hoverEnded(OpenGLComboBox* component) {
    // if (component->isModulation_) {
    // if (editing_comboBox_component_ != nullptr && component->isModulation_)
        // editing_comboBox_component_->setVisible(false);
        //        editing_state_component_->setVisible(false);
    // }
}

void ModulationManager::hoverStarted(StateModulatedComponent *component) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (changing_hover_modulation_)
        return;

    if (!enteringHoverValue())
        makeModulationsVisible(component, true);
    // if(!component->isModulation_) {
    //     if(editing_state_component_)
    //         editing_state_component_->setVisible(false);
    // }
}

void ModulationManager::hoverEnded(StateModulatedComponent *component) {
    if (component->isModulation_) {
        // editing_state_component_->setVisible(false);
        //        editing_state_component_->setVisible(false);
    }
}

void ModulationManager::hoverStarted(SynthButton *button) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (changing_hover_modulation_)
        return;

    if (!enteringHoverValue())
        makeModulationsVisible(button, true);
    if(editing_state_component_)
        editing_state_component_->setVisible(false);
    //    ModulationAmountKnob* amount_knob = dynamic_cast<ModulationIndicator*>(slider);
    //    if (amount_knob)
    //    {
    //        //DBG(amount_knob->getName() + juce::String((juce::uint64)(void*)amount_knob));
    //        showModulationAmountOverlay (amount_knob);
    //    }
    //    else
    //        hideModulationAmountOverlay();
}

void ModulationManager::hoverEnded(SynthButton *button) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
}

void ModulationManager::hoverStarted(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (changing_hover_modulation_)
        return;

    if (!enteringHoverValue())
        makeModulationsVisible(slider, true);
    for (auto &hover_indicator: modulation_hover_indicators_) {
        hover_indicator->makeVisible(false);
    }

    ModulationAmountKnob *amount_knob = dynamic_cast<ModulationAmountKnob *>(slider);
    if (amount_knob) {
        //DBG(amount_knob->getName() + juce::String((juce::uint64)(void*)amount_knob));
        showModulationAmountOverlay(amount_knob);
    } else
        hideModulationAmountOverlay();
    if(editing_state_component_)
        editing_state_component_->setVisible(false);
}

void ModulationManager::hoverEnded(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    hideModulationAmountOverlay();
    //cant make the modulation go away on destinatino hover end becuase then you can't ever get to the modualtion
    //could iomplement with some sort of short timer ?
    //  if (changing_hover_modulation_)
    //      return;
    //  makeModulationsVisible(slider, false);
}

void ModulationManager::menuFinished(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (current_modulator_ && current_modulator_->isVisible())
        current_modulator_->grabKeyboardFocus();
}

void ModulationManager::modulationsChanged(const std::string &destination_str) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    hideUnusedHoverModulations();

    if (destination_lookup_.count(destination_str) == 0)
        return;

    ModulationDestination* destination = destination_lookup_[destination_str];

    if (current_modulator_)
    {
        makeCurrentModulatorAmountsVisible();
        makeCurrentStateModulatorsVisible();
    }
    else
    {
        std::visit([this](const auto& ptr)
        {
            if (ptr != nullptr)
                makeModulationsVisible(ptr, ptr->isShowing());
        }, destination->destination);
    }
    if (parent == nullptr)
        return;

    if (meter_lookup_.count(destination_str) == 0)
        return;

    int num_modulations = parent->getSynth()->getNumModulations(destination_str);
    meter_lookup_[destination_str]->setModulated(num_modulations);
    meter_lookup_[destination_str]->setVisible(num_modulations);
}

int ModulationManager::getModulationIndex(std::string source, std::string destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->
            getDestinationConnections(destination);

    for (bitklavier::ModulationConnection *connection: connections) {
        if (connection->source_name == source)
            return connection->index_in_all_mods;
    }

    return -1;
}

int ModulationManager::getStateModulationIndex(std::string source, std::string destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->
            getDestinationStateConnections(destination);

    for (auto *connection: connections) {
        if (connection->source_name == source)
            return connection->index_in_all_mods;
    }

    return -1;
}

int ModulationManager::getIndexForModulationIndicator(juce::Component *component) {
    ModulationIndicator *indicator = dynamic_cast<ModulationIndicator *>(component);
    if (indicator)
        return indicator->index();
    return -1;
}

int ModulationManager::getIndexForModulationSlider(juce::Slider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    ModulationAmountKnob *amount_knob = dynamic_cast<ModulationAmountKnob *>(slider);
    if (amount_knob)
        return amount_knob->index();
    return -1;
}

bitklavier::StateConnection *ModulationManager::getConnectionForModulationIndicator(ModulationIndicator *comp) {
    int index = getIndexForModulationIndicator(comp);
    if (index < 0)
        return nullptr;
    //    while()
    return getStateConnection(index);
}

bitklavier::ModulationConnection *ModulationManager::getConnectionForModulationSlider(juce::Slider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    int index = getIndexForModulationSlider(slider);
    if (index < 0)
        return nullptr;

    while (aux_connections_to_from_.count(index))
        index = aux_connections_to_from_[index];

    return getConnection(index);
}

bitklavier::ModulationConnection *ModulationManager::getConnection(int index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return nullptr;

    return parent->getSynth()->getModulationBank().atIndex(index);
}

bitklavier::StateConnection *ModulationManager::getStateConnection(int index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return nullptr;

    return parent->getSynth()->getStateBank().atIndex(index);
}

bitklavier::ModulationConnection *ModulationManager::getConnection(const std::string &source, const std::string &dest) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return nullptr;

    std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->getSourceConnections(source);
    for (bitklavier::ModulationConnection *connection: connections) {
        if (connection->destination_name == dest)
            return connection;
    }

    return nullptr;
}

bitklavier::StateConnection *ModulationManager::getStateConnection(const std::string &source, const std::string &dest) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return nullptr;

    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->getSourceStateConnections(source);
    for (bitklavier::StateConnection *connection: connections) {
        if (connection->destination_name == dest)
            return connection;
    }

    return nullptr;
}

void ModulationManager::mouseDown(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    for (auto &amount_knob: modulation_hover_sliders_) {
        if (slider == amount_knob.get())
            return;
    }
    //
    if (modulation_expansion_box_->isVisible())
        return;

    bitklavier::ModulationConnection *connection = getConnectionForModulationSlider(slider);
    if (connection && !connection->source_name.empty() && !connection->destination_name.empty())
        modulationSelected(modulation_buttons_[connection->source_name]);
    else {
        clearModulationSource();
        hideModulationAmountOverlay();
        makeModulationsVisible(slider, true);
    }
}

void ModulationManager::mouseUp(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (current_modulator_ && current_modulator_->isVisible())
        current_modulator_->grabKeyboardFocus();
}

void ModulationManager::doubleClick(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    changing_hover_modulation_ = false;
    bitklavier::ModulationConnection *connection = getConnectionForModulationSlider(slider);
    //  if (connection)
    //    removeModulation(connection->source_name, connection->destination_name);
    //  setModulationAmounts();
    //
    //  if (current_modulator_ && current_modulator_->isVisible())
    //    current_modulator_->grabKeyboardFocus();
}

void ModulationManager::beginModulationEdit(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    changing_hover_modulation_ = true;
}


void ModulationManager::endModulationEdit(SynthSlider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    changing_hover_modulation_ = false;
}

void ModulationManager::sliderValueChanged(juce::Slider *slider) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    ModulationAmountKnob *amount_knob = dynamic_cast<ModulationAmountKnob *>(slider);
    auto *destination = dynamic_cast<SynthSlider *>(slider);

    if (amount_knob == nullptr && destination != nullptr) {
        std::string name = destination->getComponentID().toStdString();
        if (slider_model_lookup_[name] != destination)
            return;

        auto parent = findParentComponentOfClass<SynthGuiInterface>();
        std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->
                getDestinationConnections(name);

        for (bitklavier::ModulationConnection *connection: connections) {
            connection->setScalingValue(connection->getCurrentBaseValue(),destination->getValue());
        }

        changing_hover_modulation_ = false;
        hideModulationAmountOverlay();
    }

    if (amount_knob == nullptr)
        return;

    float value = slider->getValue();
    int index = getIndexForModulationSlider(slider);
    float scale = getAuxMultiplier(index);
    float scaled_value = value * scale;
    while (aux_connections_to_from_.count(index))
        index = aux_connections_to_from_[index];

    bitklavier::ModulationConnection *connection = getConnection(index);
    bool bipolar = connection->isBipolar();
    bool stereo = connection->isStereo();
    bool bypass = connection->isBypass();
    if (amount_knob->destination)
        connection->setScalingValue(value,amount_knob->destination->getValue());
    else
        connection->setScalingValue(value, connection->currentDestinationSliderVal);

    //DBG("ModulationManager::sliderValueChanged " + juce::String(value) +", " + juce::String(connection->getScaling()));
    setModulationValues(connection->source_name, connection->destination_name, value, bipolar, stereo, bypass);
    amount_knob->my0to1amt = connection->getScaling();
    showModulationAmountOverlay(amount_knob);

    SynthSection::sliderValueChanged(modulation_amount_sliders_[index].get());
}

void ModulationManager::buttonClicked(juce::Button *button) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    for (auto &callout_button: modulation_callout_buttons_) {
        if (button == callout_button.second.get()) {
            bool new_button = button != current_expanded_modulation_;
            hideModulationAmountCallout();
            if (new_button)
                showModulationAmountCallout(callout_button.first);
            return;
        }
    }

    SynthSection::buttonClicked(button);
}

void ModulationManager::connectStateModulation(std::string source, std::string destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return;

    modifying_ = true;
    parent->connectStateModulation(source, destination);
    modifying_ = false;
}


void ModulationManager::connectModulation(std::string source, std::string destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return;

    modifying_ = true;
    parent->connectModulation(source, destination);
    modifying_ = false;
}

void ModulationManager::removeStateModulation(std::string source, std::string destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return;

    bitklavier::StateConnection *connection = getStateConnection(source, destination);
    if (connection == nullptr) {
        positionModulationAmountSliders();
        return;
    }
    //
    //    int index = connection->index_in_all_mods;
    //    if (aux_connections_from_to_.count(index)) {
    //        float current_value = connection->getCurrentBaseValue();
    //        int dest_index = aux_connections_from_to_[index];
    //        ModulationAmountKnob* modulation_amount = modulation_amount_sliders_[dest_index].get();
    //        removeAuxSourceConnection(index);
    //        float reset_value = current_value == 0.0f ? 1.0f : -current_value;
    //        modulation_amount->setValue(reset_value, juce::dontSendNotification);
    //        modulation_amount->setValue(current_value * 2.0f, juce::sendNotificationSync);
    //    }
    //    else
    //        removeAuxSourceConnection(index);
    //
    modifying_ = true;
    parent->disconnectStateModulation(source, destination);
    modulationsChanged(destination);
    modifying_ = false;
    positionModulationAmountSliders();
}

void ModulationManager::removeModulation(std::string source, std::string destination) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return;
    bitklavier::StateConnection *connection = getStateConnection(source,destination);
    if(connection == nullptr) {
        bitklavier::ModulationConnection *connection = getConnection(source, destination);
        if (connection == nullptr) {
            positionModulationAmountSliders();
            return;
        }

        int index = connection->index_in_all_mods;
        if (aux_connections_from_to_.count(index)) {
            float current_value = connection->getCurrentBaseValue();
            int dest_index = aux_connections_from_to_[index];
            ModulationAmountKnob *modulation_amount = modulation_amount_sliders_[dest_index].get();
            removeAuxSourceConnection(index);
            float reset_value = current_value == 0.0f ? 1.0f : -current_value;
            modulation_amount->setValue(reset_value, juce::dontSendNotification);
            modulation_amount->setValue(current_value * 2.0f, juce::sendNotificationSync);
        } else
            removeAuxSourceConnection(index);

        modifying_ = true;
        parent->disconnectModulation(source, destination);
        modulationsChanged(destination);
        modifying_ = false;
        positionModulationAmountSliders();
    } else {
        modifying_ = true;
        parent->disconnectStateModulation(source,destination);
        modulationsChanged(destination);
        modifying_ = false;
    }
}

void ModulationManager::setModulationSliderValue(int index, float value) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    modulation_amount_sliders_[index]->setValue(value, juce::dontSendNotification);
    modulation_hover_sliders_[index]->setValue(value, juce::dontSendNotification);
    selected_modulation_sliders_[index]->setValue(value, juce::dontSendNotification);
    modulation_amount_sliders_[index]->redoImage();
    modulation_hover_sliders_[index]->redoImage();
    selected_modulation_sliders_[index]->redoImage();
}

void ModulationManager::setModulationSliderBipolar(int index, bool bipolar) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    modulation_amount_sliders_[index]->setBipolar(bipolar);
    modulation_hover_sliders_[index]->setBipolar(bipolar);
    selected_modulation_sliders_[index]->setBipolar(bipolar);
}

void ModulationManager::setModulationSliderValues(int index, float value) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    setModulationSliderValue(index, value);
    float from_value = value;
    int from_index = index;
    while (aux_connections_from_to_.count(from_index)) {
        from_index = aux_connections_from_to_[from_index];
        from_value *= 2.0f;
        setModulationSliderValue(from_index, from_value);
    }

    float to_value = value;
    int to_index = index;
    while (aux_connections_to_from_.count(to_index)) {
        to_index = aux_connections_to_from_[to_index];
        to_value *= 0.5f;
        setModulationSliderValue(to_index, to_value);
    }

    setModulationSliderScale(index);
}

void ModulationManager::setModulationSliderScale(int index) {
    //    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    int end_index = index;
    float scale = 1.0f;
    while (aux_connections_from_to_.count(end_index)) {
        end_index = aux_connections_from_to_[end_index];
        scale *= 2.0f;
    }

    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    //  bitklavier::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
    //  bitklavier::ModulationConnection* connection = bank.atIndex(end_index);
    //  if (!connection->destination_name.empty()) {
    //    bitklavier::ValueDetails details = bitklavier::juce::Parameters::getDetails(connection->destination_name);
    //    if (details.value_scale == bitklavier::ValueDetails::kLinear || details.value_scale == bitklavier::ValueDetails::kIndexed) {
    //      float display_multiply = scale * (details.max - details.min);
    //      modulation_amount_sliders_[index]->setDisplayMultiply(display_multiply);
    //      modulation_hover_sliders_[index]->setDisplayMultiply(display_multiply);
    //      selected_modulation_sliders_[index]->setDisplayMultiply(display_multiply);
    //      return;
    //    }
    //  }
    modulation_amount_sliders_[index]->setDisplayMultiply(1.0f);
    modulation_hover_sliders_[index]->setDisplayMultiply(1.0f);
    selected_modulation_sliders_[index]->setDisplayMultiply(1.0f);
}

void ModulationManager::setStateModulationValues(std::string source, std::string destination,
                                                 const juce::ValueTree &values) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return;

    modifying_ = true;
    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->
            getDestinationStateConnections(destination);

    for (auto *connection: connections) {
        if (connection->source_name == source)
            connection->setChange(values);
    }

    setStateModulationAmounts();
    modifying_ = false;
}

void ModulationManager::setStateModulationAmounts() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || modifying_)
        return;
}

void ModulationManager::setModulationValues(std::string source, std::string destination,
                                            float amount, bool bipolar, bool stereo, bool bypass) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return;

    modifying_ = true;
    //  parent->setModulationValues(source, destination, amount, bipolar, stereo, bypass);
    int index = getModulationIndex(source, destination);
    //  notifyModulationValueChanged(index);
    setModulationAmounts();
    setModulationSliderValues(index, amount);
    setModulationSliderBipolar(index, bipolar);

    modifying_ = false;
}


void ModulationManager::initAuxConnections() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        modulation_amount_sliders_[i]->removeAux();
        modulation_hover_sliders_[i]->removeAux();
        selected_modulation_sliders_[i]->removeAux();
    }

    aux_connections_from_to_.clear();
    aux_connections_to_from_.clear();
    //
    bitklavier::ModulationConnectionBank &bank = parent->getSynth()->getModulationBank();
    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        bitklavier::ModulationConnection *connection = bank.atIndex(i);
        int index = connection->index_in_all_mods;

        if (modulation_amount_lookup_.count(connection->destination_name)) {
            int modulation_index = modulation_amount_lookup_[connection->destination_name]->index();
            addAuxConnection(index, modulation_index);
        }
    }
}

void ModulationManager::reset() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || modifying_)
        return;

    //reset mod manager to point to current piano


    componentAdded();

    for (auto &meter: meter_lookup_) {
        int num_modulations = parent->getSynth()->getNumModulations(meter.first);
        meter.second->setModulated(num_modulations);
        meter.second->setVisible(num_modulations);
    }

    for (auto &button: modulation_buttons_)
        button.second->setActiveModulation(button.second->isActiveModulation());

    setModulationAmounts();
    if (getWidth() > 0)
        positionModulationAmountSliders();
    initAuxConnections();
}

void ModulationManager::hideUnusedHoverModulations() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || changing_hover_modulation_)
        return;

    bitklavier::ModulationConnectionBank &bank = parent->getSynth()->getModulationBank();
    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        bitklavier::ModulationConnection *connection = bank.atIndex(i);
        int index = connection->index_in_all_mods;
        if (connection->source_name.empty() || connection->destination_name.empty())
            modulation_hover_sliders_[index]->hideImmediately();
        else {
            SynthSlider *slider = slider_model_lookup_[connection->destination_name];
            if (slider == nullptr || !slider->isShowing())
                modulation_hover_sliders_[index]->hideImmediately();
        }
    }
}

float ModulationManager::getAuxMultiplier(int index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    float mult = 1.0f;
    while (aux_connections_to_from_.count(index)) {
        index = aux_connections_to_from_[index];
        mult *= 0.5f;
    }

    return mult;
}

void ModulationManager::addAuxConnection(int from_index, int to_index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (from_index == to_index)
        return;

    aux_connections_to_from_[to_index] = from_index;
    aux_connections_from_to_[from_index] = to_index;
    std::string aux_name = "modulation_" + std::to_string(from_index + 1) + "_amount";
    modulation_hover_sliders_[to_index]->setAux(aux_name);
    modulation_amount_sliders_[to_index]->setAux(aux_name);
}

void ModulationManager::removeAuxSourceConnection(int from_index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (aux_connections_from_to_.count(from_index) == 0)
        return;

    int to_index = aux_connections_from_to_[from_index];
    modulation_hover_sliders_[to_index]->removeAux();
    modulation_amount_sliders_[to_index]->removeAux();
    aux_connections_from_to_.erase(from_index);
    aux_connections_to_from_.erase(to_index);
}

void ModulationManager::removeAuxDestinationConnection(int to_index) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    if (aux_connections_to_from_.count(to_index) == 0)
        return;

    modulation_hover_sliders_[to_index]->removeAux();
    modulation_amount_sliders_[to_index]->removeAux();
    aux_connections_from_to_.erase(aux_connections_to_from_[to_index]);
    aux_connections_to_from_.erase(to_index);
}

void ModulationManager::makeCurrentModulatorAmountsVisible() {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();

    if (current_modulator_ == nullptr || parent == nullptr)
        return;

    std::string source_name = current_modulator_->getComponentID().toStdString();
    std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->getSourceConnections(source_name);
    std::set<ModulationAmountKnob *> selected_modulation_sliders;

    int width = size_ratio_ * 24.0f;
    for (bitklavier::ModulationConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        ModulationAmountKnob *selected_slider = selected_modulation_sliders_[index].get();
        selected_slider->setCurrentModulator(true);
        selected_modulation_sliders.insert(selected_slider);
        if (!selected_slider->hasAux()) {
            selected_slider->setValue(connection->getCurrentBaseValue(), juce::dontSendNotification);
            selected_slider->redoImage();
        }
        selected_slider->setSource(connection->source_name);
        selected_slider->setCurrentModulator(connection->source_name == source_name);
        if (slider_model_lookup_.count(connection->destination_name) == 0)
            continue;

        SynthSlider *destination_slider = slider_model_lookup_[connection->destination_name];
        if (slider_model_lookup_[connection->destination_name] == nullptr)
            return;

        juce::Rectangle<int> destination_bounds =
                getLocalArea(destination_slider, destination_slider->getLocalBounds());

        int center_x = destination_bounds.getCentreX();
        int left = destination_bounds.getX();
        int right = destination_bounds.getRight();

        int bottom = destination_bounds.getBottom();
        int top = destination_bounds.getY();
        int center_y = destination_bounds.getCentreY();

        juce::BubbleComponent::BubblePlacement placement = destination_slider->getModulationPlacement();
        selected_slider->setPopupPlacement(placement);
        if (placement == juce::BubbleComponent::below)
            selected_slider->setBounds(center_x - width / 2, bottom, width, width);
        else if (placement == juce::BubbleComponent::above)
            selected_slider->setBounds(center_x - width / 2, center_y - width * 2, width, width);
            //selected_slider->setBounds(center_x - width / 2, top - width, width, width);
        else if (placement == juce::BubbleComponent::left)
            selected_slider->setBounds(left - width, center_y - width / 2, width, width);
        else
            selected_slider->setBounds(right, center_y - width / 2, width, width);

        selected_slider->makeVisible(destination_slider->isShowing());
        selected_slider->setDestinationSlider(destination_slider);
    }

    for (auto &selected_slider: selected_modulation_sliders_) {
        if (selected_modulation_sliders.count(selected_slider.get()) == 0)
            selected_slider->makeVisible(false);
    }
}

void ModulationManager::makeCurrentStateModulatorsVisible() {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();

    if (current_modulator_ == nullptr || parent == nullptr)
        return;

    std::string source_name = current_modulator_->getComponentID().toStdString();
    //DBG("source_name" + source_name);
    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->getSourceStateConnections(source_name);
    std::set<ModulationIndicator *> selected_modulation_indicators;
    int width = size_ratio_ * 24.0f;
    for (bitklavier::StateConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        ModulationIndicator *selected_indicator = selected_modulation_indicators_[index].get();
        selected_indicator->setCurrentModulator(true);
        selected_modulation_indicators.insert(selected_indicator);
        //        if (!selected_indicator->hasAux()) {
        //            selected_indicator->setValue(connection->getCurrentBaseValue(), juce::dontSendNotification);
        //            selected_indicator->redoImage();
        //        }
        selected_indicator->setSource(connection->source_name);
        selected_indicator->setCurrentModulator(connection->source_name == source_name);
        if (button_model_lookup_.count(connection->destination_name) == 0 && state_model_lookup_.count(
                connection->destination_name) == 0 && combo_box_lookup.count(connection->destination_name))
            continue;


        juce::Component *destination = button_model_lookup_[connection->destination_name];
        if (destination == nullptr)
            destination = state_model_lookup_[connection->destination_name];
        if (destination == nullptr)
            destination = combo_box_lookup[connection->destination_name];
        if (destination == nullptr)
            return;
        juce::Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());

        int center_x = destination_bounds.getCentreX();
        int left = destination_bounds.getX();
        int right = destination_bounds.getRight();

        int bottom = destination_bounds.getBottom();
        int top = destination_bounds.getY();
        int center_y = destination_bounds.getCentreY();

        juce::BubbleComponent::BubblePlacement placement = juce::BubbleComponent::below;
        //destination_indicator->getModulationPlacement();
        //        selected_indicator->setPopupPlacement(placement);
        if (placement == juce::BubbleComponent::below)
            selected_indicator->setBounds(center_x - width / 2, bottom, width, width);
        else if (placement == juce::BubbleComponent::above)
            selected_indicator->setBounds(center_x - width / 2, top - width, width, width);
        else if (placement == juce::BubbleComponent::left)
            selected_indicator->setBounds(left - width, center_y - width / 2, width, width);
        else
            selected_indicator->setBounds(right, center_y - width / 2, width, width);

        selected_indicator->makeVisible(destination->isShowing());
    }
    for (auto &selected: selected_modulation_indicators_) {
        if (selected_modulation_indicators.count(selected.get()) == 0)
            selected->makeVisible(false);
    }
}
void ModulationManager::makeModulationsVisible(OpenGLComboBox* destination, bool visible) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (destination == nullptr || parent == nullptr || changing_hover_modulation_) {
        return;
    }
    std::string name = destination->getComponentID().toStdString();
    if (combo_box_lookup[name] != destination)
        return;

    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->getDestinationStateConnections(name);
    std::vector<ModulationIndicator *> modulation_hover_indicators;
    bool current_modulation_showing = false;
    for (bitklavier::StateConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        ModulationIndicator *indicator = modulation_hover_indicators_[index].get();
        if (current_modulator_ && current_modulator_->getComponentID() == juce::String(connection->source_name))
            current_modulation_showing = true;
        else
            modulation_hover_indicators.push_back(indicator);
        indicator->setSource(connection->source_name);
    }

    int hover_indicator_width = size_ratio_ * 24.0f;
    if (current_modulation_showing) {
        auto position = modulation_hover_indicators.begin() + (modulation_hover_indicators.size() + 1) / 2;
        modulation_hover_indicators.insert(position, nullptr);
        if (modulation_hover_indicators.size() % 2 == 0)
            modulation_hover_indicators.insert(modulation_hover_indicators.end(), nullptr);
    }
    int num_sliders = (int) modulation_hover_indicators.size();

    juce::Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());
    int x = destination_bounds.getRight();
    int y = destination_bounds.getBottom();
    int beginning_offset = hover_indicator_width * num_sliders / 2;
    int delta_x = 0;
    int delta_y = 0;

    juce::BubbleComponent::BubblePlacement placement = juce::BubbleComponent::below;
    //; destination->getModulationPlacement();
    if (placement == juce::BubbleComponent::below) {
        x = destination_bounds.getCentreX() - beginning_offset;
        delta_x = hover_indicator_width;
    }

    std::unordered_set<ModulationIndicator *> lookup(modulation_hover_indicators.begin(),
                                                     modulation_hover_indicators.end());
    for (auto &hover_indicator: modulation_hover_indicators_) {
        if (lookup.count(hover_indicator.get()) == 0)
            hover_indicator->makeVisible(false);
    }

    for (ModulationIndicator *hover_indicator: modulation_hover_indicators) {
        if (hover_indicator) {
            //            hover_indicator->setPopupPlacement(placement);
            hover_indicator->setBounds(x, y, hover_indicator_width, hover_indicator_width);
            hover_indicator->makeVisible(visible);
            //            hover_indicator->redoImage();
            DBG("make visible combox");
        }
        x += delta_x;
        y += delta_y;
    }
}
void ModulationManager::makeModulationsVisible(StateModulatedComponent *destination, bool visible) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (destination == nullptr || parent == nullptr || changing_hover_modulation_) {
        return;
    }
    std::string name = destination->getComponentID().toStdString();
    if (state_model_lookup_[name] != destination)
        return;

    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->getDestinationStateConnections(name);
    std::vector<ModulationIndicator *> modulation_hover_indicators;
    bool current_modulation_showing = false;
    for (bitklavier::StateConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        ModulationIndicator *indicator = modulation_hover_indicators_[index].get();
        if (current_modulator_ && current_modulator_->getComponentID() == juce::String(connection->source_name))
            current_modulation_showing = true;
        else
            modulation_hover_indicators.push_back(indicator);
        indicator->setSource(connection->source_name);
    }

    int hover_indicator_width = size_ratio_ * 24.0f;
    if (current_modulation_showing) {
        auto position = modulation_hover_indicators.begin() + (modulation_hover_indicators.size() + 1) / 2;
        modulation_hover_indicators.insert(position, nullptr);
        if (modulation_hover_indicators.size() % 2 == 0)
            modulation_hover_indicators.insert(modulation_hover_indicators.end(), nullptr);
    }
    int num_sliders = (int) modulation_hover_indicators.size();

    juce::Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());
    int x = destination_bounds.getRight();
    int y = destination_bounds.getBottom();
    int beginning_offset = hover_indicator_width * num_sliders / 2;
    int delta_x = 0;
    int delta_y = 0;

    juce::BubbleComponent::BubblePlacement placement = juce::BubbleComponent::below;
    //; destination->getModulationPlacement();
    if (placement == juce::BubbleComponent::below) {
        x = destination_bounds.getCentreX() - beginning_offset;
        delta_x = hover_indicator_width;
    }
    //    else if (placement == juce::BubbleComponent::above) {
    //        x = destination_bounds.getCentreX() - beginning_offset;
    //        y = destination_bounds.getY() - hover_slider_width;
    //        delta_x = hover_slider_width;
    //    }
    //    else if (placement == juce::BubbleComponent::left) {
    //        x = destination_bounds.getX() - hover_slider_width;
    //        y = destination_bounds.getCentreY() - beginning_offset;
    //        delta_y = hover_slider_width;
    //    }
    //    else {
    //        y = destination_bounds.getCentreY() - beginning_offset;
    //        delta_y = hover_slider_width;
    //    }

    std::unordered_set<ModulationIndicator *> lookup(modulation_hover_indicators.begin(),
                                                     modulation_hover_indicators.end());
    for (auto &hover_indicator: modulation_hover_indicators_) {
        if (lookup.count(hover_indicator.get()) == 0)
            hover_indicator->makeVisible(false);
    }

    for (ModulationIndicator *hover_indicator: modulation_hover_indicators) {
        if (hover_indicator) {
            //            hover_indicator->setPopupPlacement(placement);
            hover_indicator->setBounds(x, y, hover_indicator_width, hover_indicator_width);
            hover_indicator->makeVisible(visible);
            // hover_indicator->redoImage();
            //DBG("make visible statemodcomp");
        }
        x += delta_x;
        y += delta_y;
    }
}

void ModulationManager::makeModulationsVisible(SynthButton *destination, bool visible) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (destination == nullptr || parent == nullptr || changing_hover_modulation_) {
        return;
    }
    std::string name = destination->getComponentID().toStdString();
    if (button_model_lookup_[name] != destination)
        return;

    std::vector<bitklavier::StateConnection *> connections = parent->getSynth()->getDestinationStateConnections(name);
    std::vector<ModulationIndicator *> modulation_hover_indicators;
    bool current_modulation_showing = false;
    for (bitklavier::StateConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        ModulationIndicator *indicator = modulation_hover_indicators_[index].get();
        if (current_modulator_ && current_modulator_->getComponentID() == juce::String(connection->source_name))
            current_modulation_showing = true;
        else
            modulation_hover_indicators.push_back(indicator);
        indicator->setSource(connection->source_name);
    }

    int hover_indicator_width = size_ratio_ * 24.0f;

    int num_sliders = (int) modulation_hover_indicators.size();

    juce::Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());
    int x = destination_bounds.getRight();
    int y = destination_bounds.getBottom();
    int beginning_offset = hover_indicator_width * num_sliders / 2;
    int delta_x = 0;
    int delta_y = 0;

    juce::BubbleComponent::BubblePlacement placement = juce::BubbleComponent::below;
    //; destination->getModulationPlacement();
    if (placement == juce::BubbleComponent::below) {
        x = destination_bounds.getCentreX() - beginning_offset;
        delta_x = hover_indicator_width;
    }
    std::unordered_set<ModulationIndicator *> lookup(modulation_hover_indicators.begin(),
                                                     modulation_hover_indicators.end());
    for (auto &hover_indicator: modulation_hover_indicators_) {
        if (lookup.count(hover_indicator.get()) == 0)
            hover_indicator->makeVisible(false);
    }

    for (ModulationIndicator *hover_indicator: modulation_hover_indicators) {
        if (hover_indicator) {
            //            hover_indicator->setPopupPlacement(placement);
            hover_indicator->setBounds(x, y, hover_indicator_width, hover_indicator_width);
            hover_indicator->makeVisible(visible);
            //            hover_indicator->redoImage();
        }
        x += delta_x;
        y += delta_y;
    }
}

void ModulationManager::makeModulationsVisible(SynthSlider *destination, bool visible) {
    //DBG("DBG: Function: " << __func__ << " | File: " << __FILE__ << " | Line: " << __LINE__);
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (destination == nullptr || parent == nullptr || changing_hover_modulation_)
        return;

    std::string name = destination->getComponentID().toStdString();
    if (slider_model_lookup_[name] != destination)
        return;

    std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->getDestinationConnections(name);
    std::vector<ModulationAmountKnob *> modulation_hover_sliders;

    bool current_modulation_showing = false;
    for (bitklavier::ModulationConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        ModulationAmountKnob *hover_slider = modulation_hover_sliders_[index].get();
        if (current_modulator_ && current_modulator_->getComponentID() == juce::String(connection->source_name))
            current_modulation_showing = true;
        else
            modulation_hover_sliders.push_back(hover_slider);
        if (!hover_slider->hasAux()) {
            hover_slider->setValue(connection->getCurrentBaseValue(), juce::dontSendNotification);
            //sould be an actualy value
            hover_slider->redoImage();
        }
        hover_slider->my0to1amt = connection->getScaling();
        hover_slider->setSource(connection->source_name);
        // hover_slider->setBipolar(false);
        // hover_slider->setStereo(false);
        // hover_slider->setBypass(false);
        hover_slider->setBipolar(connection->isBipolar());
        //    hover_slider->setStereo(connection->modulation_processor->isStereo());
        //    hover_slider->setBypass(connection->modulation_processor->isBypassed());
    }

    int hover_slider_width = size_ratio_ * 24.0f;
    if (current_modulation_showing) {
        auto position = modulation_hover_sliders.begin() + (modulation_hover_sliders.size() + 1) / 2;
        modulation_hover_sliders.insert(position, nullptr);
        if (modulation_hover_sliders.size() % 2 == 0)
            modulation_hover_sliders.insert(modulation_hover_sliders.end(), nullptr);
    }
    int num_sliders = (int) modulation_hover_sliders.size();

    juce::Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());
    int x = destination_bounds.getRight();
    int y = destination_bounds.getBottom();
    int beginning_offset = hover_slider_width * num_sliders / 2;
    int delta_x = 0;
    int delta_y = 0;

    juce::BubbleComponent::BubblePlacement placement = destination->getModulationPlacement();
    if (placement == juce::BubbleComponent::below) {
        x = destination_bounds.getCentreX() - beginning_offset;
        delta_x = hover_slider_width;
    } else if (placement == juce::BubbleComponent::above) {
        x = destination_bounds.getCentreX() - beginning_offset;
        //y = destination_bounds.getY() - hover_slider_width;
        y = destination_bounds.getCentreY() - hover_slider_width * 2;
        delta_x = hover_slider_width;
    } else if (placement == juce::BubbleComponent::left) {
        x = destination_bounds.getX() - hover_slider_width;
        y = destination_bounds.getCentreY() - beginning_offset;
        delta_y = hover_slider_width;
    } else {
        y = destination_bounds.getCentreY() - beginning_offset;
        delta_y = hover_slider_width;
    }

    std::unordered_set<ModulationAmountKnob *> lookup(modulation_hover_sliders.begin(), modulation_hover_sliders.end());
    for (auto &hover_slider: modulation_hover_sliders_) {
        if (lookup.count(hover_slider.get()) == 0)
            hover_slider->makeVisible(false);
    }
    //    for (auto& hover_indicator : modulation_hover_indicators_) {
    //        hover_indicator->makeVisible(false);
    //    }

    for (ModulationAmountKnob *hover_slider: modulation_hover_sliders) {
        if (hover_slider) {
            hover_slider->setPopupPlacement(placement);
            hover_slider->setBounds(x, y, hover_slider_width, hover_slider_width);
            hover_slider->makeVisible(visible);
            hover_slider->setDestinationSlider(destination);
            hover_slider->redoImage();
        }
        x += delta_x;
        y += delta_y;
    }
}

void ModulationManager::positionModulationAmountSlidersInside(const std::string &source,
                                                              std::vector<bitklavier::ModulationConnection *>
                                                              connections) {
    static constexpr float kRightPopupPositionX = 150;
    int total_connections = static_cast<int>(connections.size());
    ModulationButton *modulation_button = modulation_buttons_[source];
    ExpandModulationButton *expand_button = modulation_callout_buttons_[source].get();
    expand_button->setVisible(false);

    if (expand_button == current_expanded_modulation_)
        hideModulationAmountCallout();

    for (int i = 0; i < total_connections; ++i) {
        bitklavier::ModulationConnection *connection = connections[i];
        int index = connection->index_in_all_mods;
        ModulationAmountKnob *slider = modulation_amount_sliders_[index].get();
        slider->setVisible(showingInParents(modulation_button));
        juce::Point<int> point = getLocalPoint(modulation_button, juce::Point<int>(0, 0));
        slider->setBounds(modulation_button->getModulationAmountBounds(i, total_connections) + point);

        juce::BubbleComponent::BubblePlacement popup_position = juce::BubbleComponent::below;
        if (slider->getX() < kRightPopupPositionX)
            popup_position = juce::BubbleComponent::right;
        if (getWidth() - slider->getRight() < kRightPopupPositionX)
            popup_position = juce::BubbleComponent::left;
        slider->setPopupPlacement(popup_position);

        std::string name = connection->destination_name;
        if (slider_model_lookup_.count(name)) {
            slider->setDestinationComponent(slider_model_lookup_[name], name);
            slider->setDestinationSlider(slider_model_lookup_[name]);
        } else {
            slider->setDestinationComponent(nullptr, name);
            // slider->setDestinationSlider(nullptr);
        }

        slider->setMouseClickGrabsKeyboardFocus(true);
        slider->redoImage();
    }
}

void ModulationManager::positionModulationAmountSlidersCallout(const std::string &source,
                                                               std::vector<bitklavier::ModulationConnection *>
                                                               connections) {
    ModulationButton *modulation_button = modulation_buttons_[source];
    ExpandModulationButton *expand_button = modulation_callout_buttons_[source].get();
    expand_button->setBounds(getLocalArea(modulation_button, modulation_button->getModulationAreaBounds()));
    expand_button->setVisible(showingInParents(modulation_button));

    std::vector<ModulationAmountKnob *> amount_controls;
    for (bitklavier::ModulationConnection *connection: connections) {
        int index = connection->index_in_all_mods;
        amount_controls.push_back(modulation_amount_sliders_[index].get());
        ModulationAmountKnob *slider = modulation_amount_sliders_[index].get();

        std::string name = connection->destination_name;
        if (slider_model_lookup_.count(name)) {
            slider->setDestinationComponent(slider_model_lookup_[name], name);
            slider->setDestinationSlider(slider_model_lookup_[name]);
        } else {
            slider->setDestinationComponent(nullptr, name);
            slider->setDestinationSlider(nullptr);
        }
        slider->setVisible(false);
    }

    expand_button->setSliders(amount_controls);
    if (expand_button == current_expanded_modulation_)
        showModulationAmountCallout(source);
}

void ModulationManager::showModulationAmountCallout(const std::string &source) {
    static constexpr int kSliderWidth = 30;
    static constexpr int kPadding = 5;

    ModulationButton *modulation_button = modulation_buttons_[source];
    current_expanded_modulation_ = modulation_callout_buttons_[source].get();
    std::vector<ModulationAmountKnob *> amount_controls = current_expanded_modulation_->getSliders();

    int num_sliders = static_cast<int>(amount_controls.size());
    int columns = current_expanded_modulation_->getNumColumns(num_sliders);
    int rows = (num_sliders + columns - 1) / columns;
    int width = kSliderWidth * columns + 2 * kPadding;
    int height = kSliderWidth * rows + 2 * kPadding;
    juce::Rectangle<int> top_level_modulation_bounds = getLocalArea(modulation_button,
                                                                    modulation_button->getLocalBounds());
    int start_x = top_level_modulation_bounds.getX() + (modulation_button->getWidth() - width) / 2;
    start_x = std::min(getWidth() - width, std::max(0, start_x));
    int start_y = top_level_modulation_bounds.getBottom();
    start_y = std::min(getHeight() - height, start_y);

    modulation_expansion_box_->setVisible(true);
    modulation_expansion_box_->setAmountControls(amount_controls);
    modulation_expansion_box_->setBounds(start_x, start_y, width, height);
    modulation_expansion_box_->setRounding(findValue(Skin::kBodyRounding));
    modulation_expansion_box_->grabKeyboardFocus();

    int row = 0;
    int column = 0;
    for (ModulationAmountKnob *slider: amount_controls) {
        int x = column * kSliderWidth + kPadding;
        int y = height - (row + 1) * kSliderWidth - kPadding;
        slider->setBounds(start_x + x, start_y + y, kSliderWidth, kSliderWidth);
        slider->setVisible(true);
        slider->setMouseClickGrabsKeyboardFocus(false);
        slider->redoImage();
        slider->getQuadComponent()->setAlwaysOnTop(true);

        column++;
        if (column >= columns) {
            column = 0;
            row++;
        }
    }
}

void ModulationManager::hideModulationAmountCallout() {
    if (current_expanded_modulation_ == nullptr)
        return;

    std::vector<ModulationAmountKnob *> amount_controls = current_expanded_modulation_->getSliders();
    for (ModulationAmountKnob *slider: amount_controls) {
        slider->setVisible(false);
        slider->getQuadComponent()->setAlwaysOnTop(false);
    }

    modulation_expansion_box_->setVisible(false);
    current_expanded_modulation_ = nullptr;
}

//void ModulationManager::positionModulationIndicators()
//{
//
//}
//void ModulationManager::positionModulationIndicators(const std::string& source)
//{
//
//}
void ModulationManager::positionModulationAmountSliders(const std::string &source) {
    static constexpr int kMaxModulationsAcross = 3;
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    auto itButton = modulation_buttons_.find(source);
    if (itButton == modulation_buttons_.end() || itButton->second == nullptr)
    {
        // Source key not found or button was removed; nothing to position.
        return;
    }
    ModulationButton *modulation_button = itButton->second;
    juce::Rectangle<int> modulation_area = modulation_button->getModulationAreaBounds();
    int area_width = std::max(1, modulation_area.getWidth());
    int max_modulation_height = (kMaxModulationsAcross * modulation_area.getHeight()) / area_width;
    int max_modulations_inside = kMaxModulationsAcross * max_modulation_height;

    std::vector<bitklavier::ModulationConnection *> connections = parent->getSynth()->getSourceConnections(source);
    int total_connections = static_cast<int>(connections.size());
    if (total_connections) {
        if (total_connections && total_connections > max_modulations_inside)
            positionModulationAmountSlidersCallout(source, connections);
        else
            positionModulationAmountSlidersInside(source, connections);
    } else {
        auto itCallout = modulation_callout_buttons_.find(source);
        if (itCallout != modulation_callout_buttons_.end() && itCallout->second)
            itCallout->second->setVisible(false);
    }
}

void ModulationManager::positionModulationAmountSliders() {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    for (auto &modulation_slider: modulation_amount_sliders_)
        modulation_slider->setVisible(false);

    for (auto &entry: modulation_buttons_) {
        const std::string& name = entry.first;
        positionModulationAmountSliders(name);
    }
}

bool ModulationManager::enteringHoverValue() {
    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        if (modulation_amount_sliders_[i] && modulation_amount_sliders_[i]->enteringValue())
            return true;
        if (modulation_hover_sliders_[i] && modulation_hover_sliders_[i]->enteringValue())
            return true;
        if (selected_modulation_sliders_[i] && selected_modulation_sliders_[i]->enteringValue())
            return true;
    }
    return false;
}

void ModulationManager::setModulationAmounts() {
    SynthGuiInterface *parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || modifying_)
        return;

    bitklavier::ModulationConnectionBank &bank = parent->getSynth()->getModulationBank();
    for (int i = 0; i < bitklavier::kMaxModulationConnections; ++i) {
        bitklavier::ModulationConnection *connection = bank.atIndex(i);
        if (aux_connections_to_from_.count(i) == 0)
            setModulationSliderValues(i, connection->getCurrentBaseValue());

        bool bipolar = connection->isBipolar();
        bool stereo = connection->isStereo();
        bool bypass = connection->isBypass();
        modulation_amount_sliders_[i]->setBipolar(bipolar);
        modulation_amount_sliders_[i]->setStereo(stereo);
        modulation_amount_sliders_[i]->setBypass(bypass);

        modulation_hover_sliders_[i]->setBipolar(bipolar);
        modulation_hover_sliders_[i]->setStereo(stereo);
        modulation_hover_sliders_[i]->setBypass(bypass);
    }
}

void ModulationManager::setVisibleMeterBounds() {
    for (auto &meter: meter_lookup_) {
        SynthSlider *slider = slider_model_lookup_[meter.first];
        if (slider && slider->isShowing()) {
            juce::Rectangle<int> local_bounds = getLocalArea(slider, slider->getModulationMeterBounds());
            meter.second->setBounds(local_bounds);
        }
    }
}

