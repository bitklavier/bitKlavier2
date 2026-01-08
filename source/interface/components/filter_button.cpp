//
// Created by Myra Norton on 12/20/25.
//

#include "filter_button.h"
#include "BKItem.h"
#include "paths.h"

filter_button::filter_button(const juce::Image& image) :
    juce::Button("filterButton"), image_(image)
{

    setClickingTogglesState(true);

    image_component_ = std::make_shared<OpenGlImageComponent>();
    image_component_->setComponent(this);
    image_component_->setInterceptsMouseClicks(false, false);
}

void filter_button::paintButton (juce::Graphics& g,
                                 bool,
                                 bool)
{
    auto opacity = getToggleState() ? 1.0f : 0.5f;

    g.setOpacity(opacity);
    g.drawImageWithin(image_,
                      0, 0,
                      getWidth(), getHeight(),
                      juce::RectanglePlacement::centred);
    g.setOpacity(1.0f);
}


void filter_button::resized()
{
    image_component_->setBounds(getLocalBounds());
    image_component_->redrawImage(true);
}


// filter_button::filter_button () : juce::Button("filterButton")
// {
//     image_component_ = std::make_shared<OpenGlImageComponent>();
//     image_component_->setComponent(this);
//     image_component_->setInterceptsMouseClicks(false, false);
//
//     juce::Array<juce::Path> paths = Paths::keymapPaths();
//     layer_1_ = paths.getUnchecked(0);
//     layer_2_ = paths.getUnchecked(1);
//     if (paths.size() > 2)
//         layer_3_ = paths.getUnchecked(2);
// }
//
// void filter_button::resized()
// {
//     juce::Rectangle<float> bounds = getLocalBounds().toFloat();
//     int width = getWidth();
//     int height = getHeight();
//     juce::Rectangle<float> fullBounds = getLocalBounds().toFloat(); // The full area you're drawing into
//     juce::Rectangle<float> targetArea = fullBounds.reduced(kMeterPixel);   // 5px margin
//     juce::Rectangle<float> pathBounds = layer_1_.getBounds();
//
//     if (!pathBounds.isEmpty())
//     {
//         // Compute uniform scaling factor
//         float scaleX = targetArea.getWidth()  / pathBounds.getWidth();
//         float scaleY = targetArea.getHeight() / pathBounds.getHeight();
//         float uniformScale = std::min(scaleX, scaleY); // Keep aspect ratio
//
//         // Compute the center offset after scaling
//         juce::Point<float> pathCentre = pathBounds.getCentre();
//         juce::Point<float> targetCentre = targetArea.getCentre();
//
//         juce::AffineTransform transform =
//             juce::AffineTransform::translation(-pathCentre.x, -pathCentre.y)   // move to origin
//             .scaled(uniformScale)                                              // scale
//             .translated(targetCentre.x, targetCentre.y);                       // move to target center
//
//         layer_1_.applyTransform(transform);
//     }
//     redoImage();
// }
//
// void filter_button::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
// {
//     // Ascertains the current window size
//     juce::Rectangle<float> bounds = getLocalBounds().toFloat();
//
//     // Ascertains the appropriate location for layer_2 based on the preparation
//     // window size
//     float layer_2_x = bounds.getX() + (bounds.getWidth() * 23 / 100);
//     float layer_2_y = bounds.getY() + (bounds.getHeight() / 9);
//     float layer_2_width = bounds.getWidth() * 54 / 100;
//     float layer_2_height = bounds.getHeight() * 11 / 20;
//
//     // Transforms layer_2 to fit appropriately in the preparation window
//     layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
//                                                               layer_2_y,
//                                                               layer_2_width,
//                                                               layer_2_height,
//                                                               false));
//
//     // Ascertains the appropriate location for layer_3 based on the preparation
//     // window size
//     float layer_3_x = bounds.getX() + (bounds.getWidth() * 13 / 32);
//     float layer_3_y = bounds.getY() + (bounds.getHeight() / 9);
//     float layer_3_width = bounds.getWidth() * 1 / 10;
//     float layer_3_height = bounds.getHeight() * 29 / 36;
//
//     // Transforms layer_3 to fit appropriately in the preparation window
//     layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
//                                                               layer_3_y,
//                                                               layer_3_width,
//                                                               layer_3_height,
//                                                               false));
//
//     // Retrieves and sets the color of each layer
//
//     g.setColour(findColour(Skin::kShadow, true));
//     g.drawImageAt(shadow_, 0, 0, true);
//     juce::Colour c;
//     c = findColour(Skin::kWidgetPrimary2, true);
//     g.setColour(c);
//     g.fillPath(layer_1_);
//     c = findColour(Skin::kWidgetPrimary1, true);
//     g.setColour(c);
//
//     g.fillPath(layer_2_);
//     g.fillPath(layer_3_);
//     g.setColour(prep_color_);
//     g.strokePath(layer_1_,juce::PathStrokeType (kMeterPixel, juce::PathStrokeType::mitered) );
// }

