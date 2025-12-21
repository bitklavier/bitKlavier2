//
// Created by Myra Norton on 12/20/25.
//

#ifndef FILTER_BUTTON_H
#define FILTER_BUTTON_H
#include "open_gl_image_component.h"
#include "common.h"
#include "Paths.h"

class filter_button : public juce::Button
{
public:
    filter_button(const juce::Image& image);
    void resized() override;
    void paintButton (juce::Graphics&, bool, bool) override;
    std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
    void clicked() override
    {
        image_component_->redrawImage(true);
    }

private:
    juce::Image image_;
    std::shared_ptr<OpenGlImageComponent> image_component_;
};

// class filter_button : public juce::Button {
// public:
//     filter_button();
//     void resized() override;
//     void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown);
//     std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
//     void redoImage() { image_component_->redrawImage (true); }
//     void setColor (juce::Colour col) {prep_color_ = col;}
//
//     class Listener
//     {
//     public:
//         virtual ~Listener() = default;
//     };
//
//
//     float size_ratio;
//     juce::Path layer_1_;
//     juce::Image shadow_;
//     static constexpr float kMeterPixel = 5.0f;
//
// protected:
//     std::vector<Listener*> listeners_;
//     std::shared_ptr<OpenGlImageComponent> image_component_;
//
//     juce::Path layer_2_;
//     juce::Path layer_3_;
//     juce::Path layer_4_;
//     juce::Colour prep_color_;
//     bool wasJustDragged;
// };

#endif //FILTER_BUTTON_H
