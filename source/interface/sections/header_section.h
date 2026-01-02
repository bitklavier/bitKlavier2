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


#include "synth_section.h"
#include "open_gl_background.h"

class LogoButton : public juce::Button {
public:
    LogoButton(const juce::String& name) : juce::Button(name), image_component_(new OpenGlImageComponent())
    {
        image_component_->setComponent(this);
        auto paths = Paths::logoPaths();
        layer_1_ = paths.getUnchecked(0);
        layer_2_ = paths.getUnchecked(1);
        layer_3_ = paths.getUnchecked(2);
        layer_4_ = paths.getUnchecked(3);
        layer_5_ = paths.getUnchecked(4);
    }

    void resized() override {

        const juce::DropShadow shadow(juce::Colours::white, 5,juce::Point<int>(0, 0));

        if (shadow_.getWidth() == getWidth() && shadow_.getHeight() == getHeight())
            return;

        juce::Rectangle<float> bounds = getLocalBounds().toFloat();
        //letter_.applyTransform(letter_.getTransformToScaleToFit(bounds, true));
        //ring_.applyTransform(ring_.getTransformToScaleToFit(bounds, true));
        layer_1_.applyTransform(layer_1_.getTransformToScaleToFit(bounds,true));

        shadow_ = juce::Image(juce::Image::SingleChannel, getWidth(), getHeight(), true);

        juce::Graphics shadow_g(shadow_);
        //shadow.drawForPath(shadow_g, letter_);
        //shadow.drawForPath(shadow_g, ring_);
        shadow.drawForPath(shadow_g, layer_1_);
        redoImage();
    }

    void paintButton(juce::Graphics& g, bool hover, bool down) override {

        // Ascertains the current section size
        juce::Rectangle<float> bounds = getLocalBounds().toFloat();

        // Ascertains the appropriate location for layer_1 based on the section
        // size
        float layer_1_x = bounds.getX();
        float layer_1_y = bounds.getY();
        float layer_1_width = bounds.getWidth();
        float layer_1_height = bounds.getHeight();

        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_1_x,
                                                                  layer_1_y,
                                                                  layer_1_width,
                                                                  layer_1_height,
                                                                  true));

        // Ascertains the appropriate location for layer_2 based on the section
        // size
        float layer_2_x = bounds.getX();
        float layer_2_y = bounds.getY() + (bounds.getHeight() / 5);
        float layer_2_width = bounds.getWidth();
        float layer_2_height = bounds.getHeight() * 4 / 5;

        layer_2_.applyTransform(layer_2_.getTransformToScaleToFit(layer_2_x,
                                                                  layer_2_y,
                                                                  layer_2_width,
                                                                  layer_2_height,
                                                                  false));

        // Ascertains the appropriate location for layer_3 based on the section
        // size
        float layer_3_x = bounds.getX() + (bounds.getWidth() / 20);
        float layer_3_y = bounds.getY() + (bounds.getHeight() * 5 / 6);
        float layer_3_width = bounds.getWidth() * 24 / 30;
        float layer_3_height = bounds.getHeight() / 6;

        layer_3_.applyTransform(layer_3_.getTransformToScaleToFit(layer_3_x,
                                                                  layer_3_y,
                                                                  layer_3_width,
                                                                  layer_3_height,
                                                                  false));

        // Ascertains the appropriate location for layer_4 based on the section
        // size
        float layer_4_x = bounds.getX();
        float layer_4_y = bounds.getY() + (bounds.getHeight() / 9);
        float layer_4_width = bounds.getWidth();
        float layer_4_height = bounds.getHeight() / 6;

        layer_4_.applyTransform(layer_4_.getTransformToScaleToFit(layer_4_x,
                                                                  layer_4_y,
                                                                  layer_4_width,
                                                                  layer_4_height,
                                                                  false));

        // Ascertains the appropriate location for layer_5 based on the section
        // size
        float layer_5_x = bounds.getX() + (bounds.getWidth() * 5 / 30);
        float layer_5_y = bounds.getY() + (bounds.getHeight() * 10 / 30);
        float layer_5_width = bounds.getWidth() * 20 / 30;
        float layer_5_height = bounds.getHeight() * 14 / 30;

        layer_5_.applyTransform(layer_5_.getTransformToScaleToFit(layer_5_x,
                                                                  layer_5_y,
                                                                  layer_5_width,
                                                                  layer_5_height,
                                                                  false));

        // Retrieves and sets the color of each layer

        g.setColour(findColour(Skin::kShadow, true));
        g.drawImageAt(shadow_, 0, 0, true);

        juce::ColourGradient letter_gradient(letter_top_color_, 0.0f, 0.0f, letter_bottom_color_, 0.0f, getHeight(), false);
        juce::ColourGradient ring_gradient(ring_top_color_, 0.0f, 0.0f, ring_bottom_color_, 0.0f, getHeight(), false);

        g.setColour(juce::Colours::brown);
        g.fillPath(layer_1_);

        g.setColour(juce::Colours::black);
        g.fillPath(layer_2_);

        g.setColour(juce::Colours::white);
        g.fillPath(layer_3_);

        g.setColour(juce::Colours::goldenrod);
        g.fillPath(layer_4_);
        g.fillPath(layer_5_);

        if (hover) {
            g.setColour(findColour(Skin::kLightenScreen, true));
            g.fillEllipse(getLocalBounds().toFloat());
        }
        else if (down) {
            g.setColour(findColour(Skin::kOverlayScreen, true));
            g.fillEllipse(getLocalBounds().toFloat());
        }
    }

    void setLetterColors(juce::Colour top, juce::Colour bottom) {
        letter_top_color_ = top;
        letter_bottom_color_ = bottom;
        redoImage();
    }

    void setRingColors(juce::Colour top, juce::Colour bottom) {
        ring_top_color_ = top;
        ring_bottom_color_ = bottom;
        redoImage();
    }

    void mouseEnter(const juce::MouseEvent& e) override {
        juce::Button::mouseEnter(e);
        image_component_->setColor(juce::Colour(0xffdddddd));
    }

    void mouseExit(const juce::MouseEvent& e) override {
        juce::Button::mouseExit(e);
        image_component_->setColor(juce::Colours::white);
    }

    std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
    void redoImage() { image_component_->redrawImage(true); }

private:
    std::shared_ptr<OpenGlImageComponent> image_component_;

    juce::Path layer_1_;
    juce::Path layer_2_;
    juce::Path layer_3_;
    juce::Path layer_4_;
    juce::Path layer_5_;

    juce::Image shadow_;

    juce::Colour letter_top_color_;
    juce::Colour letter_bottom_color_;

    juce::Colour ring_top_color_;
    juce::Colour ring_bottom_color_;
};

class LogoSection : public SynthSection {
  public:
    static constexpr float kLogoPaddingY = 6.0f;

    class Listener {
      public:
        virtual ~Listener() { }

        virtual void showAboutSection() = 0;
    };

    LogoSection();

    void resized() override;
    void paintBackground(juce::Graphics& g) override;
    void buttonClicked(juce::Button* clicked_button) override;
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<Listener*> listeners_;
    std::unique_ptr<LogoButton> logo_button_;
};

class HeaderSection : public SynthSection, public LogoSection::Listener{
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void showAboutSection() = 0;
    };

    HeaderSection(const juce::ValueTree&);

    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void reset() override;

    void buttonClicked(juce::Button* clicked_button) override;
    void sliderValueChanged(juce::Slider* slider) override;

    void setTemporaryTab(juce::String name);

//    void showAboutSection() override {
//      for (Listener* listener : listeners_)
//        listener->showAboutSection();
//    }
    void setSampleSelectText(std::string text) {
        sampleSelectText->setText(text);
    }
    void setTabOffset(int offset) { tab_offset_ = offset; repaint(); }
    void notifyChange();
    void notifyFresh();
    void showAboutSection() override {

        for (Listener* listener : listeners_)
            listener->showAboutSection();
    }
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    const juce::ValueTree& getActivePiano();
    std::vector<std::string> getAllPianoNames();
    std::vector<Listener*> listeners_;

    int tab_offset_;
    std::unique_ptr<PlainTextComponent> temporary_tab_;
    std::unique_ptr<OpenGlShapeButton> exit_temporary_button_;
    std::shared_ptr<OpenGlQuad> body_;
    std::unique_ptr<LogoSection> logo_section_;

    int currentSampleType;
    std::shared_ptr<PlainTextComponent> sampleSelectText;
    std::unique_ptr<juce::ShapeButton> sampleSelector ;
    int currentSoundfontPreset;
    std::shared_ptr<PlainTextComponent> soundfontPresetSelectText;
    std::unique_ptr<juce::ShapeButton> soundfontPresetSelector ;

    int currentPianoIndex;
    std::shared_ptr<PlainTextComponent> pianoSelectText;
    std::unique_ptr<juce::ShapeButton> pianoSelector ;

    std::unique_ptr<OpenGlTextButton> saveButton;
    std::unique_ptr<OpenGlTextButton> loadButton;
    std::unique_ptr<juce::FileChooser> filechooser;
     std::unique_ptr<OpenGlTextButton> addPianoButton;

    std::shared_ptr<PlainTextComponent> globalSoundset_label;
    std::shared_ptr<PlainTextComponent> soundfontPreset_label;

    //juce::Image background_image_;
    //std::unique_ptr<SynthButton> click_me;
   // juce::TextButton inspectButton { "Inspect the UI" };
    //std::unique_ptr<melatonin::Inspector> inspector;
    //std::unique_ptr<OpenGlToggleButton> inspectButton;

    juce::ValueTree gallery;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderSection)
};

