/*
  ==============================================================================

    BKLookAndFeel.h
    Created: 22 Nov 2016 10:43:57pm
    Author:  Michael R Mulshine

  ==============================================================================
*/

#ifndef BKLOOKANDFEEL_H_INCLUDED
#define BKLOOKANDFEEL_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>

class BKTextFieldLAF : public juce::LookAndFeel_V3
{
public:
    
    BKTextFieldLAF()
    {
        setColour(juce::TextEditor::backgroundColourId,   juce::Colours::goldenrod);
        setColour(juce::TextEditor::textColourId,         juce::Colours::black);
        
        
    }
};

class BKButtonAndMenuLAF : public juce::LookAndFeel_V4, public juce::DeletedAtShutdown
{
public:

    BKButtonAndMenuLAF() :
    comboBoxJustification (juce::Justification::centredLeft)
    {

        setColour(juce::ComboBox::backgroundColourId, juce::Colours::black);
        setColour(juce::ComboBox::textColourId, juce::Colours::antiquewhite);
        setColour(juce::ComboBox::buttonColourId, juce::Colours::black);
        setColour(juce::ComboBox::outlineColourId, juce::Colours::antiquewhite);
        setColour(juce::ComboBox::arrowColourId, juce::Colours::antiquewhite);

        setColour(juce::PopupMenu::backgroundColourId, juce::Colours::black);
        setColour(juce::PopupMenu::textColourId, juce::Colours::antiquewhite);

        setColour(juce::TextButton::buttonColourId, juce::Colours::black);
        setColour(juce::TextButton::textColourOffId, juce::Colours::antiquewhite);
        setColour(juce::TextButton::textColourOnId, juce::Colours::antiquewhite);
        setColour(juce::TextEditor::backgroundColourId, juce::Colours::black);
        setColour(juce::TextEditor::textColourId, juce::Colours::antiquewhite);
        setColour(juce::TextEditor::outlineColourId, juce::Colours::antiquewhite);

        setColour(juce::Label::textColourId, juce::Colours::antiquewhite);

        setColour(juce::Slider::trackColourId, juce::Colours::burlywood.withMultipliedBrightness(0.4));
        setColour(juce::Slider::thumbColourId, juce::Colours::lightgrey);
        setColour(juce::Slider::backgroundColourId, juce::Colours::black);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);

        setColour(juce::ToggleButton::textColourId, juce::Colours::antiquewhite);

        toggleTextToRight = true;

        //setColour(juce::LookAndFeel_V4::ColourScheme::outline, juce::Colours::black);
        //getCurrentColourScheme().setUIColour(juce::LookAndFeel_V4::ColourScheme::widgetBackground, juce::Colours::yellow);

    }

    ~BKButtonAndMenuLAF()
    {
//         setDefaultLookAndFeel(nullptr);
    }

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox& box) override;

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                            const bool isSeparator, const bool isActive,
                                            const bool isHighlighted, const bool isTicked,
                                            const bool hasSubMenu, const juce::String& text,
                                            const juce::String& shortcutKeyText,
                                            const juce::Drawable* icon, const juce::Colour* const textColourToUse) override;

    void setComboBoxJustificationType (juce::Justification justification)    { comboBoxJustification = justification; }
    void setToggleBoxTextToRightBool (bool ttr)    { toggleTextToRight = ttr; }
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos,
                                               float minSliderPos,
                                               float maxSliderPos,
                                                   const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                               bool isMouseOverButton, bool isButtonDown) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont (void) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight)  override;
    juce::Font getLabelFont (juce::Label&) override;
    int getDefaultMenuBarHeight() override;
    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;

private:
    juce::Justification comboBoxJustification;
    bool toggleTextToRight;
};

class BKWindowLAF : public BKButtonAndMenuLAF
{
public:

    BKWindowLAF()
    {
        setColour(juce::ResizableWindow::backgroundColourId,   juce::Colours::black);

        setColour(juce::ListBox::textColourId, juce::Colours::antiquewhite);
        setColour(juce::ListBox::backgroundColourId, juce::Colours::black);
        setColour(juce::ListBox::outlineColourId, juce::Colours::antiquewhite.withAlpha(0.6f));

    }

    juce::Button* createDocumentWindowButton (int buttonType) override;
    void drawDocumentWindowTitleBar (juce::DocumentWindow& window, juce::Graphics& g,
                                     int w, int h, int titleSpaceX, int titleSpaceW,
                                     const juce::Image* icon, bool drawTitleTextOnLeft) override;

    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont (void) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight)  override;
    juce::Font getLabelFont (juce::Label&) override;

    void drawCallOutBoxBackground (juce::CallOutBox& box, juce::Graphics& g,
                                   const juce::Path& path, juce::Image& cachedImage) override;
};


class BKMultiSliderLookAndFeel : public BKButtonAndMenuLAF
{
    
public:
    
    BKMultiSliderLookAndFeel()
    {
        setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey.withMultipliedAlpha(0.25));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::darkgrey.withMultipliedAlpha(0.1));
    }
    ~BKMultiSliderLookAndFeel() {}
    
    
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

class BKDisplaySliderLookAndFeel :  public BKButtonAndMenuLAF
{
    
public:
    
    BKDisplaySliderLookAndFeel()
    {
        setColour(juce::Slider::trackColourId, juce::Colours::deepskyblue.withMultipliedBrightness(0.75));
        setColour(juce::Slider::thumbColourId, juce::Colours::deepskyblue.withMultipliedAlpha(0.75));
        setColour(juce::Slider::backgroundColourId, juce::Colours::deepskyblue.withMultipliedAlpha(0.));
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::deepskyblue.withMultipliedAlpha(0.));
    }
    ~BKDisplaySliderLookAndFeel() {}
    
};

class BKRangeMinSliderLookAndFeel : public BKButtonAndMenuLAF
{
    
public:
    
    BKRangeMinSliderLookAndFeel()
    {
        //setColour (juce::TextButton::buttonColourId, juce::Colour::greyLevel (0.8f).contrasting().withAlpha (0.13f));
        setColour(juce::Slider::thumbColourId, juce::Colours::goldenrod);
        setColour(juce::Slider::backgroundColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.1));
        setColour(juce::Slider::trackColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.35));
    }
    ~BKRangeMinSliderLookAndFeel() {}
    
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

class BKRangeMaxSliderLookAndFeel : public BKButtonAndMenuLAF
{
    
public:
    
    BKRangeMaxSliderLookAndFeel()
    {
        //setColour (juce::TextButton::buttonColourId, juce::Colour::greyLevel (0.8f).contrasting().withAlpha (0.13f));
        setColour(juce::Slider::thumbColourId, juce::Colours::goldenrod);
        setColour(juce::Slider::backgroundColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.1));
        setColour(juce::Slider::trackColourId, juce::Colours::goldenrod.withMultipliedAlpha(0.35));
        //trackColourId
    }
    ~BKRangeMaxSliderLookAndFeel() {}
    
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

#endif  // BKLOOKANDFEEL_H_INCLUDED
