//
// Created by Davis Polito on 11/8/23.
//

#ifndef BITKLAVIER2_FULLINTERFACE_H
#define BITKLAVIER2_FULLINTERFACE_H


//#include "open_gl_background.h"
#include "sections/header_section.h"
#include "sections/main_section.h"
#include "sections/synth_section.h"
#include "sections/popup_browser.h"
#include "valuetree_utils/value_tree_debugger.h"
//#include "buffer_debugger.h"

class TestSection;
class AboutSection;
struct SynthGuiData;
class HeaderSection;
class ModulationPreparation;
class ModulationManager;
namespace bitklavier{
    constexpr int kMinWindowWidth = 350;
    constexpr int kMinWindowHeight = 205;
    constexpr int kDefaultWindowWidth = 1400;
    constexpr int kDefaultWindowHeight = 820;
}
class FullInterface : public SynthSection, public juce::OpenGLRenderer, public HeaderSection::Listener,
                   juce::DragAndDropContainer, private juce::Timer
{

public :
    static constexpr double kMinOpenGlVersion = 1.4;
    FullInterface(SynthGuiData *synth_gui_data, juce::ApplicationCommandManager& _manager);
    ~FullInterface() override;
    void paintBackground(juce::Graphics& g) override;
    void parentHierarchyChanged() override {
        SynthSection::parentHierarchyChanged();
        checkShouldReposition();

    }

    void timerCallback()   // to avoid flickering when resizing the window
    {
//        open_gl_context_.attachTo(*this);
//        stopTimer();
    }

    int getPixelMultiple() const override { return pixel_multiple_; }
    void copySkinValues(const Skin& skin);
    void reloadSkin(const Skin& skin);
    void showAboutSection() override;
    void repaintChildBackground(SynthSection* child);
    void repaintSynthesisSection();
    void repaintOpenGlBackground(OpenGlComponent* component);
    void redoBackground();
    void checkShouldReposition(bool resize = true);
    void resized() override;
    void animate(bool animate) override;
    void reset() override;
    void removeAllGuiListeners();
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void setFocus();
    void notifyChange();
    void notifyFresh();
    float getResizingScale() const { return width_ * 1.0f / resized_width_; }
    void showFullScreenSection(SynthSection* full_screen);
    void enableRedoBackground(bool enable) {
        enable_redo_background_ = enable;
        if (enable)
            resized();
    }
    void hideDisplay(bool primary);
    void popupSelector(juce::Component* source, juce::Point<int> position, const PopupItems& options,
        std::function<void(int,int)> callback, std::function<void()> cancel);

    void popupDisplay(juce::Component* source, const std::string& text,
        juce::BubbleComponent::BubblePlacement placement, bool primary);

    void prepDisplay(std::unique_ptr<SynthSection> synth_section, const juce::ValueTree& v );
    void modDisplay(std::unique_ptr<SynthSection> synth_section, const juce::ValueTree& v);
    std::unique_ptr<SinglePopupSelector> popup_selector_;
    std::unique_ptr<PreparationPopup> prep_popup;
    std::unique_ptr<PreparationPopup> mod_popup;
    std::unique_ptr<PopupDisplay> popup_display_1_;
    std::unique_ptr<PopupDisplay> popup_display_2_;
        SynthSection* full_screen_section_;
    std::unique_ptr<AboutSection> about_section_;
    std::unique_ptr<MainSection> main_;
    std::unique_ptr<ModulationManager> modulation_manager;
    std::unique_ptr<HeaderSection> header_;
    //juce::OpenGLContext open_gl_context_;
    std::unique_ptr<Shaders> shaders_;

    juce::Image background_image_;
    OpenGlBackground background_;
    //juce::TextButton inspectButton { "Inspect the UI" };
    std::unique_ptr<OpenGlToggleButton> inspectButton;
    juce::OpenGLContext open_gl_context_;
    juce::CriticalSection open_gl_critical_section_;
    OpenGlWrapper open_gl_;
    juce::ScopedPointer<ValueTreeDebugger> valueTreeDebugger;
    std::map<std::string, OpenGLComboBox*> getAllComboBox() override;
    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::map<std::string, ModulationButton*> getAllModulationButtons() override;
    std::map<std::string, SynthButton*> getAllButtons() override;
    std::map<std::string, StateModulatedComponent*> getAllStateModulatedComponents() override;
    void modulationChanged();
    //is currently just the first piano. should be whateber is currentpiano
    juce::ValueTree vt;
private :


//std::unique_ptr<TestSection> test_;
    juce::ApplicationCommandManager& commandManager;
    int width_;
    int resized_width_;
    bool animate_;
    bool unsupported_{};
    float last_render_scale_;
    bool enable_redo_background_{};
    float display_scale_;
    int pixel_multiple_;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullInterface)
};

#endif //BITKLAVIER2_FULLINTERFACE_H
