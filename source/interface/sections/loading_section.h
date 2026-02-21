//
// Created by Davis Polito on 10/20/25.
//

#ifndef LOADING_SECTION_H
#define LOADING_SECTION_H
#include "overlay.h"
class LoadingSection : public Overlay {
public:
    static constexpr int kInfoWidth = 430;
    static constexpr int kBasicInfoHeight = 250;
    static constexpr int kPaddingX = 25;
    static constexpr int kPaddingY = 15;
    static constexpr int kButtonHeight = 30;
    static constexpr int kLeftLogoBuffer = 95;
    static constexpr int kNameRightBuffer = 85;
    static constexpr int kLogoWidth = 96;

    LoadingSection(const juce::String& name): Overlay(name), body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)) {
        addOpenGlComponent (body_);
        setInterceptsMouseClicks(true,false);
        sample_loading_text_ = std::make_shared<PlainTextComponent> ("plugin name", "SAMPLES LOADING...");
        addOpenGlComponent (sample_loading_text_);
    }
        // logo_ = std::make_unique<AppLogo>("logo");
        //addOpenGlComponent(logo_.get());~LoadingSection();

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override {
        SynthSection::renderOpenGlComponents(open_gl, animate);
    }

    juce::Rectangle<int> getInfoRect() {
        int info_height = kBasicInfoHeight * size_ratio_;
        int info_width = kInfoWidth * size_ratio_;

        int x = (getWidth() - info_width) / 2;
        int y = (getHeight() - info_width) / 2;
        return juce::Rectangle<int>(x, y, info_width, info_height);
    }

    void mouseUp(const juce::MouseEvent& e) override{}
    void setVisible(bool should_be_visible) override {
        if (should_be_visible) {
            //setLogoBounds();
            juce::Image image(juce::Image::ARGB, 1, 1, false);
            juce::Graphics g(image);
            paintOpenGlChildrenBackgrounds(g);
        }

        Overlay::setVisible(should_be_visible);

        // Ensure we handle mouse interaction when visible
        setInterceptsMouseClicks(should_be_visible, should_be_visible);
    }
    void buttonClicked(juce::Button* clicked_button) override{}
    void resized() override {
        int padding_x = size_ratio_ * kPaddingX;
        int padding_y = size_ratio_ * kPaddingY;
        juce::Rectangle<int> info_rect = getInfoRect();
        body_->setBounds(info_rect);
        body_->setRounding(findValue(Skin::kBodyRounding));
        //body_->setColor(findColour(Skin::kBody, true));
        body_->setColor(juce::Colours::black);
        juce::Colour body_text = findColour(Skin::kBodyText, true);
        sample_loading_text_->setColor(body_text);
        int name_x = (kLogoWidth + kLeftLogoBuffer) * size_ratio_;
        info_rect.reduce(20, 20);
        sample_loading_text_->setBounds(info_rect);
        //sample_loading_text_->setBounds(info_rect.getX(), info_rect.getY() + padding_y + 40 * size_ratio_,
        //info_rect.getWidth()- kNameRightBuffer * size_ratio_, 40 * size_ratio_);
        sample_loading_text_->setTextSize(40.0f * size_ratio_);
        Overlay::resized();
    }
    void mouseDown(const juce::MouseEvent &event) override
    {}

private:
    void fullScreen() {
        if (juce::Desktop::getInstance().getKioskModeComponent())
            juce::Desktop::getInstance().setKioskModeComponent(nullptr);
        else
            juce::Desktop::getInstance().setKioskModeComponent(getTopLevelComponent());
    }


    std::shared_ptr<OpenGlQuad> body_;
    //std::unique_ptr<AppLogo> logo_;
    std::shared_ptr<PlainTextComponent> sample_loading_text_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoadingSection)
};
#endif //LOADING_SECTION_H
