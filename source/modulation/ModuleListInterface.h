//
// Created by Davis Polito on 1/30/25.
//

#ifndef BITKLAVIER2_MODULELISTINTERFACE_H
#define BITKLAVIER2_MODULELISTINTERFACE_H

#include "synth_section.h"
#include "Identifiers.h"
#include "tracktion_ValueTreeUtilities.h"
#include <functional>
#include <map>
#include <string>
#include <iostream>
#include "ModulationList.h"

class OpenGL_LabeledBorder;
class ModulationSection
;
class ModulesContainer : public SynthSection {
public:
    ModulesContainer(juce::String name) : SynthSection(name) {
        setInterceptsMouseClicks(false,true);
    }

    ~ModulesContainer()
    {
        //sub sections get added here
        //but will not get deleted if this isn't called since it technically has
        //ownership
        sub_sections_.clear();
    }

    void paintBackground(juce::Graphics& g) override {
        g.fillAll(findColour(Skin::kBackground, true));
        paintChildrenShadows(g);
        paintChildrenBackgrounds(g);
    }
};

class EffectsViewport : public juce::Viewport {
public:
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void effectsScrolled(int position) = 0;
    };

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void visibleAreaChanged(const juce::Rectangle<int>& visible_area) override {
        for (Listener* listener : listeners_)
            listener->effectsScrolled(visible_area.getY());

        Viewport::visibleAreaChanged(visible_area);
    }

private:
    std::vector<Listener*> listeners_;
};

class ModulesInterface : public SynthSection,
                         public juce::ScrollBar::Listener, EffectsViewport::Listener,
public ModulationList::Listener

{
public:
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void effectsMoved() = 0;
        virtual void added() =0;
        virtual void removed() = 0;
    };

    ModulesInterface(juce::ValueTree &);
    virtual ~ModulesInterface();

    void paintBackground(juce::Graphics& g) override;
    void paintChildrenShadows(juce::Graphics& g) override { }
    void resized() override;
    virtual void redoBackgroundImage();
    void mouseDown (const juce::MouseEvent& e) override;

    void setFocus() { grabKeyboardFocus(); }
    virtual void setEffectPositions() = 0;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;

    void scrollBarMoved(juce::ScrollBar* scroll_bar, double range_start) override;
    virtual void setScrollBarRange();

    void buttonClicked(juce::Button* clicked_button) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void effectsScrolled(int position) override {
        setScrollBarRange();
        scroll_bar_->setCurrentRange(position, viewport_.getHeight());

        for (Listener* listener : listeners_)
            listener->effectsMoved();
    }

    virtual PopupItems createPopupMenu() = 0;
    virtual void handlePopupResult(int result) = 0;

protected:
    std::vector<Listener*> listeners_;
    EffectsViewport viewport_;
    std::unique_ptr<ModulesContainer> container_;
    OpenGlImage background_;
    juce::CriticalSection open_gl_critical_section_;
    std::vector<std::unique_ptr<ModulationSection>> modulation_sections_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    std::unique_ptr<OpenGlTextButton> addModButton;
    std::shared_ptr<PlainTextComponent> modListTitle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulesInterface)
};



#endif //BITKLAVIER2_MODULELISTINTERFACE_H
