//
// Created by Davis Polito on 2/11/25.
//

#include "ModuleListInterface.h"
#include "ModulationSection.h"
#include "OpenGL_LabeledBorder.h"

ModulesInterface::ModulesInterface(juce::ValueTree &v) : SynthSection("modules") {
    container_ = std::make_unique<ModulesContainer>("container");

    addAndMakeVisible(viewport_);
    viewport_.setViewedComponent(container_.get());
    viewport_.addListener(this);
    viewport_.setScrollBarsShown(false, false, true, false);
    viewport_.setInterceptsMouseClicks(false,true);
    //breaks sacling if true
    addSubSection(container_.get(), false);

    container_->toFront(true);
    container_->setInterceptsMouseClicks(false,true);

    addModButton = std::make_unique<OpenGlTextButton>("mod_add_mod");
    addOpenGlComponent(addModButton->getGlComponent());
    addAndMakeVisible(addModButton.get());
    addModButton->addListener(this);
    //addModButton->setLookAndFeel(TextLookAndFeel::instance());
    addModButton->setButtonText("add modification");

    modListTitle = std::make_shared<PlainTextComponent>(getName(), "modifications");
    addOpenGlComponent(modListTitle);
    modListTitle->setFontType (PlainTextComponent::kTitle);
    modListTitle->setJustification(juce::Justification::centred);

    setOpaque(false);

//    setInterceptsMouseClicks(false, true);
    ////    setSkinOverride(Skin::kAllEffects);
}
ModulesInterface::~ModulesInterface() {
    //freeObjects();
}

void ModulesInterface::paintBackground(juce::Graphics& g) {
//    juce::Colour background = findColour(Skin::kBackground, true);
//    g.setColour(background);
//    g.fillRect(getLocalBounds().withRight(getWidth() - findValue(Skin::kLargePadding) / 2));
    //paintChildBackground(g, effect_order_.get());

    redoBackgroundImage();
}

void ModulesInterface::redoBackgroundImage() {
    juce::Colour background = findColour(Skin::kBackground, true);

    int height = std::max(container_->getHeight(), getHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    juce::Image background_image = juce::Image(juce::Image::ARGB, container_->getWidth() * mult, height * mult, true);

    juce::Graphics background_graphics(background_image);
    background_graphics.addTransform(juce::AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}

void ModulesInterface::resized() {
    static constexpr float kEffectOrderWidthPercent = 0.2f;

    juce::ScopedLock lock(open_gl_critical_section_);

    int order_width = getWidth() * kEffectOrderWidthPercent;
    //    effect_order_->setBounds(0, 0, order_width, getHeight());
    //    effect_order_->setSizeRatio(size_ratio_);
    int large_padding = findValue(Skin::kLargePadding);
    int small_padding = findValue(Skin::kPadding);
    int labelsectionheight = findValue(Skin::kLabelHeight);
    int shadow_width = getComponentShadowWidth();

    juce::Rectangle<int> titleArea = getLocalBounds();
    titleArea.removeFromTop(small_padding);
    modListTitle->setBounds(titleArea.removeFromTop(labelsectionheight));
    modListTitle->setTextSize (findValue(Skin::kKnobLabelSizeMedium));

    int viewport_x = 0 + large_padding - shadow_width;
    int viewport_width = getWidth() - viewport_x - large_padding + 2 * shadow_width;
    viewport_.setBounds(viewport_x, 0, viewport_width, getHeight());
    setEffectPositions();

    scroll_bar_->setBounds(getWidth() - large_padding + 1, 0, large_padding - 2, getHeight());
    scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));

    //addModButton->setBounds(getLocalBounds().reduced(large_padding, small_padding).removeFromBottom(findValue(Skin::kComboMenuHeight)));
    addModButton->setBounds(getLocalBounds().reduced(large_padding, small_padding).removeFromTop(findValue(Skin::kComboMenuHeight)));

    SynthSection::resized();
}

void ModulesInterface::mouseDown (const juce::MouseEvent& e)
{
    if(e.mods.isPopupMenu())
    {
        PopupItems options = createPopupMenu();
        showPopupSelector(this, e.getPosition(), options, [=](int selection,int) { handlePopupResult(selection); });
    }
    juce::Component::mouseDown(e);
}

void ModulesInterface::buttonClicked(juce::Button* clicked_button)
{
    PopupItems options = createPopupMenu();
    showPopupSelector(this, getLocalBounds().getBottomRight(), options, [=](int selection,int) { handlePopupResult(selection); });
    SynthSection::buttonClicked(clicked_button);
}

void ModulesInterface::initOpenGlComponents(OpenGlWrapper& open_gl) {
    background_.init(open_gl);
    SynthSection::initOpenGlComponents(open_gl);
}

void ModulesInterface::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    juce::ScopedLock lock(open_gl_critical_section_);
    //    Component* top_level = getTopLevelComponent();
    //    Rectangle<int> global_bounds = top_level->getLocalArea(this, getLocalBounds());
    //    double display_scale = Desktop::getInstance().getDisplays().getDisplayForRect(top_level->getScreenBounds())->scale;
    //    return 1;//
    // display_scale;// * (1.0f * global_bounds.getWidth()) / getWidth();
    if (background_.shader() == nullptr)
        background_.init(open_gl);
    OpenGlComponent::setViewPort(&viewport_, open_gl);

    float image_width = background_.getImageWidth(); //bitklavier::utils::nextPowerOfTwo(background_.getImageWidth());
    float image_height =background_.getImageHeight(); // bitklavier::utils::nextPowerOfTwo(background_.getImageHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    float width_ratio = image_width / (container_->getWidth() * mult);
    float height_ratio = image_height / (viewport_.getHeight() * mult);
    float y_offset = (2.0f * viewport_.getViewPositionY()) / getHeight();

    background_.setTopLeft(-1.0f, 1.0f + y_offset);
    background_.setTopRight(-1.0 + 2.0f * width_ratio, 1.0f + y_offset);
    background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setBottomRight(-1.0 + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setColor(juce::Colours::white);
    background_.drawImage(open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate);
}

void ModulesInterface::destroyOpenGlComponents(OpenGlWrapper& open_gl) {

    if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {
        open_gl.context.executeOnGLThread([this, &open_gl](juce::OpenGLContext &openGLContext) {
            background_.destroy(open_gl);
        }, true);
    }
    else
        background_.destroy(open_gl);
    SynthSection::destroyOpenGlComponents(open_gl);
}

void ModulesInterface::scrollBarMoved(juce::ScrollBar* scroll_bar, double range_start) {
    viewport_.setViewPosition(juce::Point<int>(0, range_start));
}

void ModulesInterface::setScrollBarRange() {
    scroll_bar_->setRangeLimits(0.0, container_->getHeight());
    scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(),juce::dontSendNotification);
}
