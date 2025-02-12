#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "synth_section.h"
struct OpenGlWrapper;

namespace bitklavier {
/** Clone of juce::GenericAudioProcessorEditor, but usable as a generic component */
    class  ParametersView : public SynthSection {
    public:
        ParametersView (chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params, OpenGlWrapper *open_gl,bool isDefaultInit=true);
        ParametersView (chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params, juce::String name,bool isDefaultInit=true);
        ParametersView (chowdsp::ParameterListeners& paramListeners, chowdsp::ParamHolder& params, OpenGlWrapper *open_gl,bool isDefaultInit=true);
        ParametersView (chowdsp::ParameterListeners& paramListeners, chowdsp::ParamHolder& params, juce::String name,bool isDefaultInit=true);
        ~ParametersView() override;

        void paint(juce::Graphics &) override;

        void resized() override;
//        void initOpenGlComponents(OpenGlWrapper &open_gl) override;
//        void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
        void init_();
        /** Returns nullptr if no component is found for the given parameter */
        [[nodiscard]] juce::Component* getComponentForParameter (const juce::RangedAudioParameter&);
        void paintBackground(juce::Graphics& g) override
        {
            SynthSection::paintContainer(g);
            paintHeadingText(g);
            paintBorder(g);
            paintKnobShadows(g);
            for (auto slider : all_sliders_v) {
                drawLabelForComponent(g, slider->getName(), slider);
            }
            paintChildrenBackgrounds(g);
        }
    private:
    protected:
        //chowdsp::PluginState& pluginState;
        std::vector<std::unique_ptr<juce::Component>> comps;
        std::vector<std::unique_ptr<juce::Component>> paramHolderComps;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersView)
    };
namespace parameters_view_detail
{
    std::unique_ptr<juce::Component> createParameterComp(chowdsp::ParameterListeners& listeners, juce::RangedAudioParameter &parameter, SynthSection& parent);
    std::unique_ptr<SynthSection> createEditorSection(chowdsp::ParamHolder &params, chowdsp::ParameterListeners& listeners, SynthSection &parent);
}
///** Clone of juce::GenericAudioProcessorEditor. */
//    class ParametersViewEditor : public juce::AudioProcessorEditor {
//    public:
//        ParametersViewEditor(juce::AudioProcessor &proc, chowdsp::PluginState &pluginState,
//                             chowdsp::ParamHolder &params)
//                : juce::AudioProcessorEditor(proc),
//                  view(pluginState, params, ) {
//            setResizable(true, false);
//            setSize(view.getWidth(), view.getHeight());
//
//            addAndMakeVisible(view);
//        }
//
//        void resized() override {
//            view.setBounds(getLocalBounds());
//        }
//
//    private:
//        ParametersView view;
//
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersViewEditor)
//    };
}//namespace bitilavier
