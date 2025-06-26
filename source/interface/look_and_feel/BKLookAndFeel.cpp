/*
  ==============================================================================

    BKLookAndFeel.cpp
    Created: 20 Jun 2017 4:48:16pm
    Author:  Daniel Trueman

  ==============================================================================
*/

#include "BKLookAndFeel.h"
#include "BKGraphicsConstants.h"

class BKDocumentWindowButton   : public juce::Button
{
public:
    BKDocumentWindowButton (const juce::String& name, juce::Colour c, const juce::Path& normal, const juce::Path& toggled)
    : juce::Button (name), colour (c), normalShape (normal), toggledShape (toggled)
    {
    }
    
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto background = juce::Colours::grey;
        
        if (auto* rw = findParentComponentOfClass<juce::ResizableWindow>())
            if (auto lf = dynamic_cast<juce::LookAndFeel_V4*> (&rw->getLookAndFeel()))
                background = juce::Colours::black;
        
        g.fillAll (background);
        
        g.setColour ((! isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha (0.6f)
                     : colour);
        
        if (shouldDrawButtonAsHighlighted)
        {
            g.fillAll();
            g.setColour (background);
        }
        
        auto& p = getToggleState() ? toggledShape : normalShape;
        
        auto reducedRect = juce::Justification (juce::Justification::centred)
        .appliedToRectangle (juce::Rectangle<int> (getHeight(), getHeight()), getLocalBounds())
        .toFloat()
        .reduced (getHeight() * 0.3f);
        
        g.fillPath (p, p.getTransformToScaleToFit (reducedRect, true));
    }
    
private:
    juce::Colour colour;
    juce::Path normalShape, toggledShape;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BKDocumentWindowButton)
};

juce::Button* BKWindowLAF::createDocumentWindowButton (int buttonType)
{
    juce::Path shape;
    auto crossThickness = 0.15f;
    
    if (buttonType == juce::DocumentWindow::closeButton)
    {
        shape.addLineSegment ({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
        shape.addLineSegment ({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);
        
        return new BKDocumentWindowButton ("close", juce::Colour (0xffb11621), shape, shape);
    }
    
    if (buttonType == juce::DocumentWindow::minimiseButton)
    {
        shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
        
        return new BKDocumentWindowButton ("minimise", juce::Colour (0xffc19b13), shape, shape);
    }
    
    if (buttonType == juce::DocumentWindow::maximiseButton)
    {
        shape.addLineSegment ({ 0.5f, 0.0f, 0.5f, 1.0f }, crossThickness);
        shape.addLineSegment ({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);
        
        juce::Path fullscreenShape;
        fullscreenShape.startNewSubPath (45.0f, 100.0f);
        fullscreenShape.lineTo (0.0f, 100.0f);
        fullscreenShape.lineTo (0.0f, 0.0f);
        fullscreenShape.lineTo (100.0f, 0.0f);
        fullscreenShape.lineTo (100.0f, 45.0f);
        fullscreenShape.addRectangle (45.0f, 45.0f, 100.0f, 100.0f);
        juce::PathStrokeType (30.0f).createStrokedPath (fullscreenShape, fullscreenShape);
        
        return new BKDocumentWindowButton ("maximise", juce::Colour (0xff0A830A), shape, fullscreenShape);
    }
    
    jassertfalse;
    return nullptr;
}

void BKWindowLAF::drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g,
                                              int w, int h, int titleSpaceX, int titleSpaceW,
                                              const juce::Image* icon, bool drawTitleTextOnLeft)
{
    if (w * h == 0)
        return;
    
    auto isActive = window.isActiveWindow();
    
    g.setColour (juce::Colours::black);
    g.fillAll();
    
    juce::Font font (h * 0.65f, juce::Font::plain);
    g.setFont (font);
    
    auto textW = font.getStringWidth (window.getName());
    auto iconW = 0;
    auto iconH = 0;
    
    if (icon != nullptr)
    {
        iconH = static_cast<int> (font.getHeight());
        iconW = icon->getWidth() * iconH / icon->getHeight() + 4;
    }
    
    textW = juce::jmin (titleSpaceW, textW + iconW);
    auto textX = drawTitleTextOnLeft ? titleSpaceX
    : juce::jmax (titleSpaceX, (w - textW) / 2);
    
    if (textX + textW > titleSpaceX + titleSpaceW)
        textX = titleSpaceX + titleSpaceW - textW;
    
    if (icon != nullptr)
    {
        g.setOpacity (isActive ? 1.0f : 0.6f);
        g.drawImageWithin (*icon, textX, (h - iconH) / 2, iconW, iconH,
                           juce::RectanglePlacement::centred, false);
        textX += iconW;
        textW -= iconW;
    }
    
    if (window.isColourSpecified (juce::DocumentWindow::textColourId) || isColourSpecified (juce::DocumentWindow::textColourId))
        g.setColour (window.findColour (juce::DocumentWindow::textColourId));
    else
        g.setColour (getCurrentColourScheme().getUIColour (ColourScheme::defaultText));
    
    g.drawText (window.getName(), textX, 0, textW, h, juce::Justification::centredLeft, true);
}

juce::Font BKWindowLAF::getComboBoxFont (juce::ComboBox& box)
{
    return { juce::jmin (16.0f, box.getHeight() * 0.85f) };
}

juce::Font BKWindowLAF::getPopupMenuFont (void)
{
     return juce::Font (17.0f);
}

juce::Font BKWindowLAF::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return { juce::jmin (16.0f, buttonHeight * 0.6f) };
}

juce::Font BKWindowLAF::getLabelFont (juce::Label& label)
{
    return label.getFont();
}

void BKWindowLAF::drawCallOutBoxBackground (juce::CallOutBox& box, juce::Graphics& g,
                                               const juce::Path& path, juce::Image& cachedImage)
{
    if (cachedImage.isNull())
    {
        cachedImage = { juce::Image::ARGB, box.getWidth(), box.getHeight(), true };
        juce::Graphics g2 (cachedImage);
        
        juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 8, { 0, 2 }).drawForPath (g2, path);
    }
    
    g.setColour (juce::Colours::black);
    g.drawImageAt (cachedImage, 0, 0);
    
    g.setColour (juce::Colours::black);
    g.fillPath (path);
    
    g.setColour (juce::Colours::antiquewhite);
    g.strokePath (path, juce::PathStrokeType (1.0f));
}

void BKButtonAndMenuLAF::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setFont (getComboBoxFont (box));
    
    if(comboBoxJustification == juce::Justification::centredRight)
    {
        label.setBounds (30, 1,
                         box.getWidth() - 30,
                         box.getHeight() - 2);
        
        label.setJustificationType (juce::Justification::centredRight);
    }
    else
    {
        label.setBounds (1, 1,
                         box.getWidth() - 30,
                         box.getHeight() - 2);
        
        label.setJustificationType (juce::Justification::centredLeft);
    }
}

#define HEIGHT_IOS 22
#define HEIGHT 15
#define HORIZONTAL 0.75f
#define KERNING 0.25f
#define STYLE 0
#define IOS_RATIO 1.5f

juce::Font BKButtonAndMenuLAF::getComboBoxFont (juce::ComboBox& cb)
{
    juce::Font font;

    font.setSizeAndStyle(fontHeight, STYLE, HORIZONTAL, KERNING);
    
    return font;
}

juce::Font BKButtonAndMenuLAF::getPopupMenuFont (void)
{
    juce::Font font;

    font.setSizeAndStyle(fontHeight, STYLE, HORIZONTAL, KERNING);
    
    return font;
}

juce::Font BKButtonAndMenuLAF::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    juce::Font font;

    font.setSizeAndStyle(fontHeight, STYLE, HORIZONTAL, KERNING);
    
    return font;
}

juce::Font BKButtonAndMenuLAF::getLabelFont (juce::Label& label)
{
    juce::Font font;

    font.setSizeAndStyle(fontHeight, STYLE, HORIZONTAL, KERNING);
    
    return font;
}

void BKButtonAndMenuLAF::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    juce::Rectangle<int> bounds (width, height);
    auto cornerSize = 0.0f;
    
    g.setColour (findColour (juce::TooltipWindow::backgroundColourId));
    g.setOpacity(0.0f);
    g.fillRoundedRectangle (bounds.toFloat(), cornerSize);
    
    g.setColour (findColour (juce::TooltipWindow::outlineColourId));
    g.setOpacity(0.0f);
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);
    
    const float tooltipFontSize = 13.0f;
    const int maxToolTipWidth = 400;
    
    juce::AttributedString s;
    s.setJustification (juce::Justification::centred);
    s.append (text, juce::Font (tooltipFontSize, juce::Font::bold), findColour (juce::TooltipWindow::textColourId));
    
    juce::TextLayout tl;
    tl.createLayoutWithBalancedLineLengths (s, (float) maxToolTipWidth);
    tl.draw (g, { static_cast<float> (width), static_cast<float> (height) });
}


#define MENUBARHEIGHT_IOS 40
#define MENUBARHEIGHT  25

int BKButtonAndMenuLAF::getDefaultMenuBarHeight()
{
#if JUCE_IOS
    return MENUBARHEIGHT_IOS;
#else
    return MENUBARHEIGHT;
#endif
}

#define CORNER_SIZE 2.0f

void BKButtonAndMenuLAF::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox& box)
{
    //const auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 0.0f;
    const auto cornerSize = CORNER_SIZE;
    const juce::Rectangle<int> boxBounds (0, 0, width, height);
    
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);
    
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 0.5f);
    
    juce::Rectangle<int> arrowZone (width - 30, 0, 20, height);
    if(comboBoxJustification == juce::Justification::centredRight)
        arrowZone = juce::Rectangle<int> (gXSpacing, 0, 20, height);
    
    juce::Path path;
    path.startNewSubPath (arrowZone.getX() + 3.0f, arrowZone.getCentreY() - 2.0f);
    path.lineTo (static_cast<float> (arrowZone.getCentreX()), arrowZone.getCentreY() + 3.0f);
    path.lineTo (arrowZone.getRight() - 3.0f, arrowZone.getCentreY() - 2.0f);
    
    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha ((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

void BKButtonAndMenuLAF::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                        const bool isSeparator, const bool isActive,
                                        const bool isHighlighted, const bool isTicked,
                                        const bool hasSubMenu, const juce::String& text,
                                        const juce::String& shortcutKeyText,
                                        const juce::Drawable* icon, const juce::Colour* const textColourToUse)
{
    if (isSeparator)
    {
        auto r  = area.reduced (5, 0);
        r.removeFromTop (juce::roundToInt ((r.getHeight() * 0.5f) - 0.5f));
        
        g.setColour (findColour (juce::PopupMenu::textColourId).withAlpha (0.3f));
        g.fillRect (r.removeFromTop (1));
    }
    else
    {
        auto textColour = (textColourToUse == nullptr ? findColour (juce::PopupMenu::textColourId)
                           : *textColourToUse);
        
        auto r  = area.reduced (1);
        
        if (isHighlighted && isActive)
        {
            g.setColour (findColour (juce::PopupMenu::highlightedBackgroundColourId));
            g.fillRect (r);
            
            g.setColour (findColour (juce::PopupMenu::highlightedTextColourId));
        }
        else
        {
            g.setColour (textColour.withMultipliedAlpha (isActive ? 1.0f : 0.5f));
        }
        
        r.reduce (juce::jmin (5, area.getWidth() / 20), 0);
        
        auto font = getPopupMenuFont();
        
        const auto maxFontHeight = r.getHeight() / 1.3f;
        
        if (font.getHeight() > maxFontHeight)
            font.setHeight (maxFontHeight);
        
        g.setFont (font);
        
        auto iconArea = r.removeFromLeft (juce::roundToInt (maxFontHeight)).toFloat();
        
        if (icon != nullptr)
        {
            icon->drawWithin (g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
        }
        else if (isTicked)
        {
            const auto tick = getTickShape (1.0f);
            g.fillPath (tick, tick.getTransformToScaleToFit (iconArea.reduced (iconArea.getWidth() / 5, 0).toFloat(), true));
        }
        
        if (hasSubMenu)
        {
            const auto arrowH = 0.6f * getPopupMenuFont().getAscent();
            
            const auto x = (float) r.removeFromRight ((int) arrowH).getX();
            const auto halfH = (float) r.getCentreY();
            
            juce::Path path;
            path.startNewSubPath (x, halfH - arrowH * 0.5f);
            path.lineTo (x + arrowH * 0.6f, halfH);
            path.lineTo (x, halfH + arrowH * 0.5f);
            
            g.strokePath (path, juce::PathStrokeType (2.0f));
        }
        
        r.removeFromRight (3);
        if(comboBoxJustification == juce::Justification::centredRight)
            g.drawFittedText (text, r, juce::Justification::centredRight, 1);
        else
            g.drawFittedText (text, r, juce::Justification::centredLeft, 1);
        
        if (shortcutKeyText.isNotEmpty())
        {
            auto f2 = font;
            f2.setHeight (f2.getHeight() * 0.75f);
            f2.setHorizontalScale (0.95f);
            g.setFont (f2);
            
            if(comboBoxJustification == juce::Justification::centredRight)
                g.drawText (shortcutKeyText, r, juce::Justification::centredRight, true);
            else
                g.drawText (shortcutKeyText, r, juce::Justification::centredLeft, true);
        }
    }
}

void BKButtonAndMenuLAF::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                               bool isMouseOverButton, bool isButtonDown)
{
    const auto cornerSize = CORNER_SIZE;
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f, 0.5f);
    
    auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
    .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);
    
    if (isButtonDown || isMouseOverButton)
        baseColour = baseColour.contrasting (isButtonDown ? 0.2f : 0.05f);
    
    g.setColour (baseColour);
    
    if (button.isConnectedOnLeft() || button.isConnectedOnRight())
    {
        juce::Path path;
        path.addRoundedRectangle (bounds.getX(), bounds.getY(),
                                  bounds.getWidth(), bounds.getHeight(),
                                  cornerSize, cornerSize,
                                  ! button.isConnectedOnLeft(),
                                  ! button.isConnectedOnRight(),
                                  ! button.isConnectedOnLeft(),
                                  ! button.isConnectedOnRight());
        
        g.fillPath (path);
        
        g.setColour (button.findColour (juce::ComboBox::outlineColourId));
        g.strokePath (path, juce::PathStrokeType (0.5f));
    }
    else
    {
        g.fillRoundedRectangle (bounds, cornerSize);
        
        g.setColour (button.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, cornerSize, 0.5f);
    }
}

void BKButtonAndMenuLAF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool isMouseOverButton, bool isButtonDown)
{
    if(toggleTextToRight)
    {
        const auto fontSize = juce::jmin (15.0f, button.getHeight() * 0.75f);
        const auto tickWidth = fontSize * 1.1f;
        
        drawTickBox (g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f,
                     tickWidth, tickWidth,
                     button.getToggleState(),
                     button.isEnabled(),
                     isMouseOverButton,
                     isButtonDown);
        
        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (fontSize);
        juce::Font toggleFont;
        toggleFont.setSizeAndStyle(fontHeight, STYLE, HORIZONTAL, KERNING);
        g.setFont (toggleFont);
        
        if (! button.isEnabled())
            g.setOpacity (0.5f);
        
        const auto textX = juce::roundToInt (tickWidth) + 10;
        
        g.drawFittedText (button.getButtonText(),
                          textX, 0,
                          button.getWidth() - textX - 2, button.getHeight(),
                          juce::Justification::centredLeft, 10);
    }
    else
    {
        const auto fontSize = juce::jmin (15.0f, button.getHeight() * 0.75f);
        const auto tickWidth = fontSize * 1.1f;
        
        drawTickBox (g, button,
                     (button.getWidth() - tickWidth - 2),
                     (button.getHeight() - tickWidth) * 0.5f,
                     tickWidth, tickWidth,
                     button.getToggleState(),
                     button.isEnabled(),
                     isMouseOverButton,
                     isButtonDown);
        
        g.setColour (button.findColour (juce::ToggleButton::textColourId));
        g.setFont (fontSize);
        juce::Font toggleFont;
        toggleFont.setSizeAndStyle(fontHeight, STYLE, HORIZONTAL, KERNING);
        g.setFont (toggleFont);
        
        if (! button.isEnabled())
            g.setOpacity (0.5f);
        
        const auto textX = juce::roundToInt (tickWidth) + 10;
        
        g.drawFittedText (button.getButtonText(),
                          0,
                          0,
                          button.getWidth() - textX - 2,
                          button.getHeight(),
                          juce::Justification::centredRight, 10);
        
    }
}

void BKButtonAndMenuLAF::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.isBar())
    {
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRect (slider.isHorizontal() ? juce::Rectangle<float> (static_cast<float> (x), y + 0.5f, sliderPos - x, height - 1.0f)
                    : juce::Rectangle<float> (x + 0.5f, sliderPos, width - 1.0f, y + (height - sliderPos)));
    }
    else
    {
        const auto isTwoVal   = (style == juce::Slider::SliderStyle::TwoValueVertical   || style == juce::Slider::SliderStyle::TwoValueHorizontal);
        const auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);
        
        const auto trackWidth = juce::jmin (6.0f, slider.isHorizontal() ? height * 0.25f : width * 0.25f);
        
        const juce::Point<float> startPoint (slider.isHorizontal() ? x : x + width * 0.5f,
                                       slider.isHorizontal() ? y + height * 0.5f : height + y);
        
        const juce::Point<float> endPoint (slider.isHorizontal() ? width + x : startPoint.x,
                                     slider.isHorizontal() ? startPoint.y : y);
        
        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.strokePath (backgroundTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint, thumbPoint;
        
        if (isTwoVal || isThreeVal)
        {
            minPoint = { slider.isHorizontal() ? minSliderPos : width * 0.5f,
                slider.isHorizontal() ? height * 0.5f : minSliderPos };
            
            if (isThreeVal)
                thumbPoint = { slider.isHorizontal() ? sliderPos : width * 0.5f,
                    slider.isHorizontal() ? height * 0.5f : sliderPos };
            
            maxPoint = { slider.isHorizontal() ? maxSliderPos : width * 0.5f,
                slider.isHorizontal() ? height * 0.5f : maxSliderPos };
        }
        else
        {
            const auto kx = slider.isHorizontal() ? sliderPos : (x + width * 0.5f);
            const auto ky = slider.isHorizontal() ? (y + height * 0.5f) : sliderPos;
            
            minPoint = startPoint;
            maxPoint = { kx, ky };
        }
        
        const auto thumbWidth = trackWidth * 2.0f;
        
        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.strokePath (valueTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        if (! isTwoVal)
        {
            g.setColour (slider.findColour (juce::Slider::thumbColourId));
            g.fillEllipse (juce::Rectangle<float> (thumbWidth, thumbWidth).withCentre (isThreeVal ? thumbPoint : maxPoint));
        }
        
        if (isTwoVal || isThreeVal)
        {
            const auto sr = juce::jmin (trackWidth, (slider.isHorizontal() ? height : width) * 0.4f);
            const auto pointerColour = slider.findColour (juce::Slider::thumbColourId);
            
            if (slider.isHorizontal())
            {
                drawPointer (g, minSliderPos - sr,
                             juce::jmax (0.0f, y + height * 0.5f - trackWidth * 2.0f),
                             trackWidth * 2.0f, pointerColour, 2);
                
                drawPointer (g, maxSliderPos - trackWidth,
                             juce::jmin (y + height - trackWidth * 2.0f, y + height * 0.5f),
                             trackWidth * 2.0f, pointerColour, 4);
            }
            else
            {
                drawPointer (g, juce::jmax (0.0f, x + width * 0.5f - trackWidth * 2.0f),
                             minSliderPos - trackWidth,
                             trackWidth * 2.0f, pointerColour, 1);
                
                drawPointer (g, juce::jmin (x + width - trackWidth * 2.0f, x + width * 0.5f), maxSliderPos - sr,
                             trackWidth * 2.0f, pointerColour, 3);
            }
        }
    }
}

void BKMultiSliderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                 float sliderPos, float minSliderPos, float maxSliderPos,
                                                 const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    //g.fillAll (slider.findColour (juce::Slider::backgroundColourId)); // Transposition slider works better without this line
    
    if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
    {
        const float fx = (float) x, fy = (float) y, fw = (float) width, fh = (float) height;
        
        juce::Path p;
        
        if (style == juce::Slider::LinearBarVertical)
            p.addRectangle (fx, sliderPos - 2, fw, 4); //band instead of bar, width of 4
        else
            //p.addRectangle (fx, fy, sliderPos - fx, fh);
            p.addRectangle (sliderPos - 2, fy, 4 , fh);
        
        juce::Colour baseColour (slider.findColour (juce::Slider::thumbColourId)
                           .withMultipliedSaturation (slider.isEnabled() ? 1.0f : 0.5f)
                           .withMultipliedAlpha (0.8f));
        
        g.setGradientFill (juce::ColourGradient (baseColour.brighter (0.08f), 0.0f, 0.0f,
                                           baseColour.darker (0.08f), 0.0f, (float) height, false));
        g.fillPath (p);
        
        g.setColour (baseColour.darker (0.2f));

        if (style == juce::Slider::LinearBarVertical)
            g.fillRect (fx, sliderPos, fw, 1.0f);
        else
            g.fillRect (sliderPos, fy, 1.0f, fh);
    }
    else
    {
        drawLinearSliderBackground (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}


void BKRangeMinSliderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                    float sliderPos,
                                                    float minSliderPos,
                                                    float maxSliderPos,
                                                    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    {
        
        const auto trackWidth = juce::jmin (6.0f, slider.isHorizontal() ? height * 0.25f : width * 0.25f);
        
        const juce::Point<float> startPoint (slider.isHorizontal() ? x : width * 0.5f,
                                       slider.isHorizontal() ? height * 0.5f : height + y);
        
        const juce::Point<float> endPoint (slider.isHorizontal() ? width + x : startPoint.x,
                                     slider.isHorizontal() ? startPoint.y : y);
        
        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.strokePath (backgroundTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint;
        
        const auto kx = slider.isHorizontal() ? sliderPos : (x + width * 0.5f);
        const auto ky = slider.isHorizontal() ? (y + height * 0.5f) : sliderPos;
        
        //minPoint = startPoint;
        //maxPoint = { kx, ky };
        
        minPoint = { kx, ky };
        maxPoint = endPoint;
        
        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (maxPoint);
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.strokePath (valueTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        const auto pointerColour = slider.findColour (juce::Slider::thumbColourId);
        drawPointer (g, sliderPos - trackWidth,
                     //juce::jmin (y + height - trackWidth * 2.0f, y + height * 0.5f),
                     //juce::jmin (y + height - trackWidth * 1.0f, y + height * 0.5f),
                     y + height + 1 - trackWidth * 3.0f,
                     trackWidth * 2.0f, pointerColour, 4);
        
    }
}

void BKRangeMaxSliderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                    float sliderPos,
                                                    float minSliderPos,
                                                    float maxSliderPos,
                                                    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    {
        
        const auto trackWidth = juce::jmin (6.0f, slider.isHorizontal() ? height * 0.25f : width * 0.25f);
        
        const juce::Point<float> startPoint (slider.isHorizontal() ? x : width * 0.5f,
                                       slider.isHorizontal() ? height * 0.5f : height + y);
        
        const juce::Point<float> endPoint (slider.isHorizontal() ? width + x : startPoint.x,
                                     slider.isHorizontal() ? startPoint.y : y);
        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint;
        
        const auto kx = slider.isHorizontal() ? sliderPos : (x + width * 0.5f);
        const auto ky = slider.isHorizontal() ? (y + height * 0.5f) : sliderPos;
        
        //maxPoint = endPoint;
        //minPoint = { kx, ky };
        maxPoint = { kx, ky };
        minPoint = startPoint;
        
        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (maxPoint);
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        //g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.strokePath (valueTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        
        const auto pointerColour = slider.findColour (juce::Slider::thumbColourId);
        const auto sr = juce::jmin (trackWidth, (slider.isHorizontal() ? height : width) * 0.4f);
        drawPointer (g, sliderPos - sr,
                     //juce::jmax (0.0f, y + height * 0.5f - trackWidth * 2.0f),
                     //juce::jmax (0.0f, y + height * 0.5f - trackWidth * 4.0f),
                     //y + height - trackWidth *6.0f,
                     y + 4,
                     trackWidth * 2.0f, pointerColour, 2);
        
        
        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        //g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        //g.strokePath (backgroundTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}


