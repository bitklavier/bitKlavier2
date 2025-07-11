//
// Created by Davis Polito on 4/22/24.
//

#ifndef BITKLAVIER2_BKITEM_H
#define BITKLAVIER2_BKITEM_H

#include "open_gl_image_component.h"
#include "common.h"
#include "Paths.h"

class BKItem : /*public DraggableComponent,*/ public juce::Button{
public:

BKItem (bitklavier::BKPreparationType type);
//    void mouseDown (const juce::MouseEvent& e) override;
//    void mouseUp (const juce::MouseEvent& e) override;
//    void mouseDoubleClick (const juce::MouseEvent& e) override;
//    void mouseDrag(const juce::MouseEvent& e) override;

    // void paint(juce::Graphics& g) override;
    void setIcons (const juce::Path& layer_1, const juce::Path& layer_2, const juce::Path& layer_3)
    {
        layer_1_ = layer_1;
        layer_2_ = layer_2;
        layer_3_ = layer_3;

    }

    void resized() override
    {
        //redoImage();
        const juce::DropShadow shadow(juce::Colours::white, 5, juce::Point<int>(0, 0));

        if (shadow_.getWidth() == getWidth() && shadow_.getHeight() == getHeight())
            return;

        juce::Rectangle<float> bounds = getLocalBounds().toFloat();//.reduced(10).withX(0).withY(0);

        // juce::Rectangle<float> s = bounds;// bounds.getProportion<float>({0.0f, 0.0f, 1.f, 1.f});
        // s.setCentre(bounds.getCentre());
        // layer_1_.applyTransform(layer_1_.getTransformToScaleToFit(s,true));
        int width = getWidth();
        int height = getHeight();
        hit_test_bounds = bounds;
        shadow_ = juce::Image(juce::Image::SingleChannel,width, height, true);
        juce::Rectangle<float> fullBounds = getLocalBounds().toFloat(); // The full area you're drawing into
        juce::Rectangle<float> targetArea = fullBounds.reduced(kMeterPixel);   // 5px margin
        juce::Rectangle<float> pathBounds = layer_1_.getBounds();

        if (!pathBounds.isEmpty())
        {
            // Compute uniform scaling factor
            float scaleX = targetArea.getWidth()  / pathBounds.getWidth();
            float scaleY = targetArea.getHeight() / pathBounds.getHeight();
            float uniformScale = std::min(scaleX, scaleY); // Keep aspect ratio

            // Compute the center offset after scaling
            juce::Point<float> pathCentre = pathBounds.getCentre();
            juce::Point<float> targetCentre = targetArea.getCentre();

            juce::AffineTransform transform =
                juce::AffineTransform::translation(-pathCentre.x, -pathCentre.y)   // move to origin
                .scaled(uniformScale)                                              // scale
                .translated(targetCentre.x, targetCentre.y);                       // move to target center

            layer_1_.applyTransform(transform);
        }
        //juce::Graphics shadow_g(shadow_);
        //shadow.drawForPath(shadow_g, layer_1_);

        redoImage();
    };

    virtual void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {

    }

    class Listener
    {
    public:
        virtual ~Listener() = default;

    };

    std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
    void redoImage() { image_component_->redrawImage (true); } //image_component_.redrawImage(true); }

    void setColor (juce::Colour col)
    {
        prep_color_ = col;
    }
    bool hitTest(int x, int y) override {
        layer_1_.contains(x, y);
    }
//
        float size_ratio;

    juce::Path layer_1_;
    juce::Rectangle<float> hit_test_bounds;
    juce::Image shadow_;
    static constexpr float kMeterPixel = 5.0f;
protected:
    //void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;
    //    juce::ValueTree &state;
    //    juce::UndoManager &um;
    std::vector<Listener*> listeners_;
    std::shared_ptr<OpenGlImageComponent> image_component_;

    juce::Path layer_2_;
    juce::Path layer_3_;
    juce::Path layer_4_;
    juce::Colour prep_color_;
    bool wasJustDragged;

};

class KeymapItem : public BKItem
{
public:
    KeymapItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeKeymap)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() * 23 / 100);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 9);
        float layer_2_width = bounds.getWidth() * 54 / 100;
        float layer_2_height = bounds.getHeight() * 11 / 20;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() * 13 / 32);
        float layer_3_y = bounds.getY() + (bounds.getHeight() / 9);
        float layer_3_width = bounds.getWidth() * 1 / 10;
        float layer_3_height = bounds.getHeight() * 29 / 36;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  false));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary2, true);
        g.setColour(c);
        g.fillPath(layer_1_);
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);

        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_,juce::PathStrokeType (kMeterPixel, juce::PathStrokeType::mitered) );
    }

};

class DirectItem : public BKItem
{
public:
    DirectItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeDirect)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() / 11);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 5);
        float layer_2_width = bounds.getWidth() * 9 / 11;
        float layer_2_height = bounds.getHeight() * 3 / 5;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() * 17 / 30);
        float layer_3_y = bounds.getY() + (bounds.getHeight() * 8 / 30);
        float layer_3_width = bounds.getWidth() / 6;
        float layer_3_height = bounds.getHeight() * 11 / 20;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  true));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        //g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_, juce::PathStrokeType(kMeterPixel, juce::PathStrokeType::mitered));
    }
};

class NostalgicItem : public BKItem
{
public:
    NostalgicItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeNostalgic)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() / 11);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 8);
        float layer_2_width = bounds.getWidth() * 9 / 11;
        float layer_2_height = bounds.getHeight() * 3 / 4;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() * 18 / 30);
        float layer_3_y = bounds.getY() + (bounds.getHeight() * 9 / 30);
        float layer_3_width = bounds.getWidth() / 7;
        float layer_3_height = bounds.getHeight() * 11 / 20;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  true));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_,juce::PathStrokeType (kMeterPixel, juce::PathStrokeType::mitered) );
    }

};

class SynchronicItem : public BKItem
{
public:
    SynchronicItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeSynchronic)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() / 15);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 9);
        float layer_2_width = bounds.getWidth() * 13 / 15;
        float layer_2_height = bounds.getHeight() * 7 / 9;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() * 2 / 3);
        float layer_3_y = bounds.getY() + (bounds.getHeight() / 3);
        float layer_3_width = bounds.getWidth() / 7;
        float layer_3_height = bounds.getHeight() / 2;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  true));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_,juce::PathStrokeType (kMeterPixel, juce::PathStrokeType::mitered));
    }

};

class BlendronicItem : public BKItem
{
public:
    BlendronicItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeBlendronic)
    {
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() / 15);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 7);
        float layer_2_width = bounds.getWidth() * 23 / 30;
        float layer_2_height = bounds.getHeight() * 5 / 7;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() * 23 / 30);
        float layer_3_y = bounds.getY() + (bounds.getHeight() * 12 / 30);
        float layer_3_width = bounds.getWidth() / 7;
        float layer_3_height = bounds.getHeight() * 9 / 20;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  true));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_,juce::PathStrokeType (kMeterPixel, juce::PathStrokeType::mitered) );
    }

};

class ResonanceItem : public BKItem
{
public:
    ResonanceItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeResonance)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() * 9 / 30);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 7);
        float layer_2_width = bounds.getWidth() * 18 / 30;
        float layer_2_height = bounds.getHeight() * 5 / 7;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() / 10);
        float layer_3_y = bounds.getY() + (bounds.getHeight() * 10 / 30);
        float layer_3_width = bounds.getWidth() / 7;
        float layer_3_height = bounds.getHeight() / 2;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  true));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_, juce::PathStrokeType(kMeterPixel, juce::PathStrokeType::mitered));
    }
};

class TuningItem : public BKItem
{
public:
    TuningItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeTuning)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() * 3 / 16 );
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 10);
        float layer_2_width = bounds.getWidth() * 11 / 16;
        float layer_2_height = bounds.getHeight() * 4 / 5;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));


        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_, juce::PathStrokeType(kMeterPixel, juce::PathStrokeType::mitered));
    }

};

class TempoItem : public BKItem
{
public:
    TempoItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeTempo)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() / 2 );
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 8);
        float layer_2_width = bounds.getWidth() / 2;
        float layer_2_height = bounds.getHeight() * 19 / 30;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));


        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_, juce::PathStrokeType(kMeterPixel, juce::PathStrokeType::mitered));
    }
};


class VSTItem : public BKItem {
    public:
    VSTItem (): BKItem(bitklavier::BKPreparationType::PreparationTypeVST){}
     void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        // Ascertains the current window size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_2 based on the preparation
        // window size
        float layer_2_x = bounds.getX() + (bounds.getWidth() * 9 / 30);
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 7);
        float layer_2_width = bounds.getWidth() * 18 / 30;
        float layer_2_height = bounds.getHeight() * 5 / 7;

        // Transforms layer_2 to fit appropriately in the preparation window
        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the preparation
        // window size
        float layer_3_x = bounds.getX() + (bounds.getWidth() / 10);
        float layer_3_y = bounds.getY() + (bounds.getHeight() * 10 / 30);
        float layer_3_width = bounds.getWidth() / 7;
        float layer_3_height = bounds.getHeight() / 2;

        // Transforms layer_3 to fit appropriately in the preparation window
        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  true));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);
        g.fillPath(layer_1_);

        juce::Colour c;
        c = findColour(Skin::kWidgetPrimary1, true);
        g.setColour(c);
        g.fillPath(layer_2_);
        g.fillPath(layer_3_);
        g.setColour(prep_color_);
        g.strokePath(layer_1_, juce::PathStrokeType(kMeterPixel, juce::PathStrokeType::mitered));
    }
};


class ResetItem : public BKItem
{
public:
    ResetItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeReset)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto bounds = getLocalBounds().toFloat();
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.5f;

        juce::Path stopSign;

        const int numSides = 8;
        const float angleStep = juce::MathConstants<float>::twoPi / numSides;
        const float rotation = juce::MathConstants<float>::pi / 8.0f; // 22.5 degrees

        for (int i = 0; i < numSides; ++i)
        {
            float angle = angleStep * i + rotation; // Add rotation here
            float x = centerX + radius * std::cos(angle);
            float y = centerY + radius * std::sin(angle);

            if (i == 0)
                stopSign.startNewSubPath(x, y);
            else
                stopSign.lineTo(x, y);
        }

        stopSign.closeSubPath();

        // Red fill
        g.setColour(juce::Colours::red);
        g.fillPath(stopSign);

        // White border
        g.setColour(juce::Colours::white);
        g.strokePath(stopSign, juce::PathStrokeType(6.0f));
    }
};

class MidiFilterItem : public BKItem
{
public:
    MidiFilterItem() : BKItem(bitklavier::BKPreparationType::PreparationTypeMidiFilter)
    {

    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
    {
        auto bounds = getLocalBounds().toFloat();
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.5f;

        juce::Path stopSign;

        const float numSides = 5;
        const float angleStep = juce::MathConstants<float>::twoPi / numSides;
        const float rotation = (juce::MathConstants<float>::pi / numSides) * 3. + juce::MathConstants<float>::pi / 2.;

        for (int i = 0; i < numSides; ++i)
        {
            float angle = angleStep * i + rotation; // Add rotation here
            float x = centerX + radius * std::cos(angle);
            float y = centerY + radius * std::sin(angle);

            if (i == 0)
                stopSign.startNewSubPath(x, y);
            else
                stopSign.lineTo(x, y);
        }

        stopSign.closeSubPath();

        // fill
        g.setColour(juce::Colours::steelblue); // just to differentiate from reset for now
        g.fillPath(stopSign);

        // White border
        g.setColour(juce::Colours::white);
        g.strokePath(stopSign, juce::PathStrokeType(6.0f));
    }
};

#endif //BITKLAVIER2_BKITEM_H
