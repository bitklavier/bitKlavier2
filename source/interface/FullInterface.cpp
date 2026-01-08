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

#include "FullInterface.h"
#include "Identifiers.h"
#include "ModulationPreparation.h"
#include "about_section.h"
#include "modulation_manager.h"
#include "sections/header_section.h"
#include "sections/main_section.h"
#include "synth_base.h"
#include "synth_slider.h"
#include "loading_section.h"

FullInterface::FullInterface (SynthGuiData* synth_data, juce::ApplicationCommandManager& _manager)
    : SynthSection ("full_interface"), width_ (0), resized_width_ (0), last_render_scale_ (0.0f), display_scale_ (1.0f), pixel_multiple_ (1), unsupported_ (false), animate_ (true), enable_redo_background_ (true), open_gl_ (open_gl_context_),
    commandManager (_manager)
{
    full_screen_section_ = nullptr;
    Skin default_skin;
    setSkinValues (default_skin, true);
    default_skin.copyValuesToLookAndFeel (DefaultLookAndFeel::instance());

    vt = synth_data->tree.getChildWithName(IDs::PIANO);
    main_ = std::make_unique<MainSection> (synth_data->tree, synth_data->um, open_gl_, synth_data, commandManager);
    addSubSection (main_.get());
    main_->addListener (this);
    // valueTreeDebugger = new ValueTreeDebugger (synth_data->tree);
    modulation_manager = std::make_unique<ModulationManager> (synth_data->tree.getChildWithName(IDs::PIANO), synth_data->synth);
    modulation_manager->setOpaque (false);
    modulation_manager->setAlwaysOnTop (true);
    modulation_manager->setModulationAmounts();
    modulation_manager->setVisibleMeterBounds();
    modulation_manager->hideUnusedHoverModulations();
    modulation_manager->toFront (false);

    header_ = std::make_unique<HeaderSection>(synth_data->tree);
    addSubSection (header_.get());
    header_->addListener (this);

    footer_ = std::make_unique<FooterSection>(synth_data);
    addSubSection (footer_.get());
    footer_->addListener (this);

    prep_popup = std::make_unique<PreparationPopup> (false);
    //prep_popup->setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
    addSubSection (prep_popup.get());
    prep_popup->setVisible (false);
    prep_popup->setAlwaysOnTop (true);
    prep_popup->setWantsKeyboardFocus (true);

    mod_popup = std::make_unique<PreparationPopup> (true);
    addSubSection (mod_popup.get());
    mod_popup->setVisible (false);
    mod_popup->setAlwaysOnTop (true);
    mod_popup->setWantsKeyboardFocus (true);

    popup_selector_ = std::make_unique<SinglePopupSelector>();
    addSubSection (popup_selector_.get());
    popup_selector_->setVisible (false);
    popup_selector_->setAlwaysOnTop (true);
    popup_selector_->setWantsKeyboardFocus (true);
    //popup_selector_->setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);

    popup_display_1_ = std::make_unique<PopupDisplay>();
    addSubSection (popup_display_1_.get());
    popup_display_1_->setVisible (false);
    popup_display_1_->setAlwaysOnTop (true);
    popup_display_1_->setWantsKeyboardFocus (false);
    //popup_display_1_->setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);

    popup_display_2_ = std::make_unique<PopupDisplay>();
    addSubSection (popup_display_2_.get());
    popup_display_2_->setVisible (false);
    popup_display_2_->setAlwaysOnTop (true);
    popup_display_2_->setWantsKeyboardFocus (false);
    //popup_display_2_->setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);

    about_section_ = std::make_unique<AboutSection> ("about");
    addSubSection (about_section_.get(), false);
    addChildComponent (about_section_.get());
    loading_section = std::make_unique<LoadingSection> ("load");
    addSubSection (loading_section.get(), false);
    loading_section->setAlwaysOnTop(true);
    addChildComponent (loading_section.get());

    addSubSection (modulation_manager.get());

    about_section_->toFront (true);
    loading_section->toFront (true);
    setOpaque (true);
    open_gl_context_.setContinuousRepainting (true);
    open_gl_context_.setOpenGLVersionRequired (juce::OpenGLContext::openGL3_2);
    open_gl_context_.setSwapInterval (0);
    open_gl_context_.setRenderer (this);
    //componentpaintingenabled fixes flickering
    open_gl_context_.setComponentPaintingEnabled (false); // set to true, and the non-OpenGL components will draw
    open_gl_context_.attachTo (*this);

    ///startTimer(100);
}

FullInterface::~FullInterface()
{
}

void FullInterface::paintBackground (juce::Graphics& g)
{
    g.fillAll (findColour (Skin::kBackground, true));
    paintChildrenShadows (g);

    paintKnobShadows (g);
    paintChildrenBackgrounds (g);
}

void FullInterface::copySkinValues (const Skin& skin)
{
}

void FullInterface::reloadSkin (const Skin& skin)
{
    copySkinValues (skin);
    juce::Rectangle<int> bounds = getBounds();
    setBounds (0, 0, bounds.getWidth() / 4, bounds.getHeight() / 4);
    setBounds (bounds);
}

void FullInterface::repaintChildBackground (SynthSection* child)
{
    if (!background_image_.isValid())
        return;
    background_.lock();
    juce::Graphics g (background_image_);
    paintChildBackground (g, child);
    background_.updateBackgroundImage (background_image_);
    background_.unlock();
}

void FullInterface::repaintSynthesisSection()
{
    if (!background_image_.isValid())
        return;

    background_.lock();
    juce::Graphics g (background_image_);

    g.setColour (findColour (Skin::kBackground, true));

    background_.updateBackgroundImage (background_image_);
    background_.unlock();
}

void FullInterface::repaintOpenGlBackground (OpenGlComponent* component)
{
    if (!background_image_.isValid())
        return;

    background_.lock();
    juce::Graphics g (background_image_);
    paintOpenGlBackground (g, component);
    background_.updateBackgroundImage (background_image_);
    background_.unlock();
}

void FullInterface::redoBackground()
{
    int width = std::ceil (display_scale_ * getWidth());
    int height = std::ceil (display_scale_ * getHeight());
    if (width < bitklavier::kMinWindowWidth || height < bitklavier::kMinWindowHeight)
        return;

    juce::ScopedLock open_gl_lock (open_gl_critical_section_);

    background_.lock();
    background_image_ = juce::Image (juce::Image::RGB, width, height, true);
    juce::Graphics g (background_image_);
    paintBackground (g);
    background_.updateBackgroundImage (background_image_);
    background_.unlock();
}

void FullInterface::checkShouldReposition (bool resize)
{
    float old_scale = display_scale_;
    int old_pixel_multiple = pixel_multiple_;
    display_scale_ = getDisplayScale();
    pixel_multiple_ = std::max<int> (1, display_scale_);

    if (resize && (old_scale != display_scale_ || old_pixel_multiple != pixel_multiple_))
        resized();
}

void FullInterface::resized()
{
    checkShouldReposition (false);

    if (!enable_redo_background_)
    {
        return;
    }
    width_ = getWidth();

    resized_width_ = width_;

    juce::ScopedLock lock (open_gl_critical_section_);
    static constexpr int kTopHeight = 48;

    int left = 0;
    int top = 0;
    int width = std::ceil (getWidth() * display_scale_);
    int height = std::ceil (getHeight() * display_scale_);
    juce::Rectangle<int> bounds (0, 0, width, height);
    modulation_manager->setBounds (bounds);

    /*
     * not sure we need this ratio stuff if we're usign findValue and the skin?
     */
    float width_ratio = getWidth() / (1.0f * bitklavier::kDefaultWindowWidth);
    float ratio = width_ratio * display_scale_;
    float height_ratio = getHeight() / (1.0f * bitklavier::kDefaultWindowHeight);
    if (width_ratio > height_ratio + 1.0f / bitklavier::kDefaultWindowHeight)
    {
        ratio = height_ratio;
        width = height_ratio * bitklavier::kDefaultWindowWidth * display_scale_;
        left = (getWidth() - width) / 2;
    }
    if (height_ratio > width_ratio + 1.0f / bitklavier::kDefaultWindowHeight)
    {
        ratio = width_ratio;
        height = ratio * bitklavier::kDefaultWindowHeight * display_scale_;
        top = (getHeight() - height) / 2;
    }

    setSizeRatio (ratio);
    int voice_padding = findValue (Skin::kLargePadding);
    int top_height = kTopHeight * ratio;

    header_->setTabOffset (2 * voice_padding);
    header_->setBounds (left, top, width, top_height);
    footer_->setBounds (left, height - 100, width, 100);
    juce::Rectangle<int> new_bounds (0, 0, width, height);
    main_->setBounds (new_bounds);
    prep_popup->setBounds (voice_padding, header_->getBottom() + voice_padding, new_bounds.getWidth() / (1.2 * display_scale_), new_bounds.getHeight() / (1.2 * display_scale_));
    mod_popup->setBounds (bounds.getRight() - 200 - voice_padding, header_->getBottom() + voice_padding, 200, new_bounds.getHeight() / (1.2 * display_scale_));
    about_section_->setBounds (new_bounds);
    loading_section->setBounds (new_bounds);
    if (getWidth() && getHeight())
        redoBackground();
}

void FullInterface::showAboutSection()
{
    juce::ScopedLock lock (open_gl_critical_section_);
    about_section_->setVisible (true);
}
void FullInterface::showLoadingSection()
{
    juce::ScopedLock lock (open_gl_critical_section_);
    loading_section->setVisible (true);
}

void FullInterface::hideLoadingSection() {
    juce::ScopedLock lock (open_gl_critical_section_);
    loading_section->setVisible (false);
}

void FullInterface::animate (bool animate)
{
    if (animate_ != animate)
        open_gl_context_.setContinuousRepainting (animate);

    animate_ = animate;
    SynthSection::animate (animate);
}

void FullInterface::reset()
{
    //object creation and destruction is thread safe internally should not need to put a scopedlock hereo
    //vital previously had a scoped lock on this whole section. but this only really makes sense whenever we have a static system
    //since individual components must be capable of being created and destroyed we want to lock or block in those
    //individual destruction calls -- 4/25/25 -- davis
    vt = findParentComponentOfClass<SynthGuiInterface>()->getSynth()->getValueTree().getChildWithName (IDs::PIANO);
    SynthSection::reset();
    repaintSynthesisSection();
    prep_popup->reset();
    mod_popup->reset();
}

void FullInterface::removeAllGuiListeners() {
    main_->removeAllGuiListeners();
}

void FullInterface::popupDisplay (juce::Component* source, const std::string& text, juce::BubbleComponent::BubblePlacement placement, bool primary)
{
    PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
    display->setContent (text, getLocalArea (source, source->getLocalBounds()), placement);
    display->setVisible (true);
}

void FullInterface::prepDisplay (std::unique_ptr<SynthSection> synth_section, const juce::ValueTree &v)
{
    prep_popup->setContent (std::move(synth_section),v);
    prep_popup->setVisible (true);
    modulation_manager->added();
}

void FullInterface::modDisplay (std::unique_ptr<SynthSection> synth_section,const juce::ValueTree &v)
{
    mod_popup->setContent (std::move(synth_section),v);
    mod_popup->setVisible (true);
    modulation_manager->added();
}

void FullInterface::hideDisplay (bool primary)
{
    PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
    if (display)
        display->setVisible (false);
}

void FullInterface::popupSelector (juce::Component* source, juce::Point<int> position, const PopupItems& options, std::function<void (int,int)> callback, std::function<void()> cancel)
{
    popup_selector_->setCallback (callback);
    popup_selector_->setCancelCallback (cancel);
    popup_selector_->showSelections (options);
    juce::Rectangle<int> bounds (0, 0, std::ceil (display_scale_ * getWidth()), std::ceil (display_scale_ * getHeight()));
    popup_selector_->setPosition (getLocalPoint (source, position), bounds);
    popup_selector_->setVisible (true);
}

void FullInterface::newOpenGLContextCreated()
{
    double version_supported = juce::OpenGLShaderProgram::getLanguageVersion();
    unsupported_ = version_supported < kMinOpenGlVersion;
    if (unsupported_)
    {
        juce::NativeMessageBox::showMessageBoxAsync (juce::AlertWindow::WarningIcon, "Unsupported OpenGl Version", juce::String ("Vial requires OpenGL version: ") + juce::String (kMinOpenGlVersion) + juce::String ("\nSupported version: ") + juce::String (version_supported));
        return;
    }

    shaders_ = std::make_unique<Shaders> (open_gl_context_);
    open_gl_.shaders = shaders_.get();
    open_gl_.display_scale = display_scale_;
    last_render_scale_ = display_scale_;

    background_.init (open_gl_);
    initOpenGlComponents (open_gl_);
    modulation_manager->renderMeters (open_gl_, animate_);
}

void FullInterface::renderOpenGL()
{
    if (unsupported_)
        return;

    float render_scale = open_gl_.context.getRenderingScale();
    if (render_scale != last_render_scale_)
    {
        last_render_scale_ = render_scale;
        juce::MessageManager::callAsync ([=] { checkShouldReposition (true); });
    }

    juce::ScopedLock lock (open_gl_critical_section_);
    open_gl_.display_scale = display_scale_;
    background_.render (open_gl_);

    renderOpenGlComponents (open_gl_, animate_);
    modulation_manager->renderMeters(open_gl_,animate_);
}

void FullInterface::openGLContextClosing()
{
    if (unsupported_)
        return;
    DBG ("closing");
    background_.destroy (open_gl_);
    prep_popup->destroyOpenGlComponents (open_gl_);
    mod_popup->destroyOpenGlComponents (open_gl_);
    destroyOpenGlComponents (open_gl_);
    removeSubSection(loading_section.get());
    removeSubSection (main_.get());
    removeSubSection (header_.get());
    removeSubSection (footer_.get());

    removeSubSection (prep_popup.get());
    removeSubSection (mod_popup.get());

    // main_.reset();
    // main_->destroyOpenGlComponents(open_gl);
    main_ = nullptr;
    header_ = nullptr;
    footer_ = nullptr;
    prep_popup = nullptr;
    mod_popup = nullptr;

    open_gl_.shaders = nullptr;
    shaders_ = nullptr;
}

void FullInterface::setFocus()
{
}

void FullInterface::notifyChange()
{
    if (header_)
        header_->notifyChange();
}

void FullInterface::notifyFresh()
{
    if (header_)
        header_->notifyFresh();
}

void FullInterface::showFullScreenSection (SynthSection* full_screen)
{
    juce::ScopedLock lock (open_gl_critical_section_);
    full_screen_section_ = full_screen;

    if (full_screen_section_)
    {
        addSubSection (full_screen_section_);
        full_screen_section_->setBounds (getLocalBounds());
    }

    //   for (int i = 0; i < bitklavier::kNumOscillators; ++i)
    //       wavetable_edits_[i]->setVisible(false);
    //

    bool show_rest = full_screen == nullptr;
    // header_->setVisible(show_rest);
    //   synthesis_interface_->setVisible(show_rest);
    //   modulation_interface_->setVisible(show_rest);
    //   keyboard_interface_->setVisible(show_rest);
    //   extra_mod_section_->setVisible(show_rest);
    //   modulation_manager_->setVisible(show_rest);
    //   voice_section_->setVisible(show_rest);
    //   bend_section_->setVisible(show_rest);
    //   portamento_section_->setVisible(show_rest);
    redoBackground();
}

//void FullInterface::showWavetableEditSection(int index) {
//   if (!wavetableEditorsInitialized())
//       return;
//
//   ScopedLock lock(open_gl_critical_section_);
//   for (int i = 0; i < bitklavier::kNumOscillators; ++i)
//       wavetable_edits_[i]->setVisible(i == index);
//
//   bool show_rest = index < 0;
//   header_->setVisible(show_rest);
//   synthesis_interface_->setVisible(show_rest);
//   modulation_interface_->setVisible(show_rest);
//   keyboard_interface_->setVisible(show_rest);
//   extra_mod_section_->setVisible(show_rest);
//   modulation_manager_->setVisible(show_rest);
//   voice_section_->setVisible(show_rest);
//   bend_section_->setVisible(show_rest);
//   portamento_section_->setVisible(show_rest);
//   redoBackground();
//}

std::map<std::string, SynthSlider*> FullInterface::getAllSliders()
{
    return prep_popup->getAllSliders();
}

std::map<std::string, SynthButton*> FullInterface::getAllButtons()
{
    return prep_popup->getAllButtons();
}

std::map<std::string, ModulationButton*> FullInterface::getAllModulationButtons()
{
    return mod_popup->getAllModulationButtons();
}
std::map<std::string, OpenGLComboBox*> FullInterface::getAllComboBox() {
    return prep_popup->getAllComboBox();
}

std::map<std::string, StateModulatedComponent*> FullInterface::getAllStateModulatedComponents()
{
    return prep_popup->getAllStateModulatedComponents();
}

void FullInterface::modulationChanged()
{
    if (modulation_manager)
        modulation_manager->reset();
}
