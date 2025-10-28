//
// Created by Myra Norton on 9/12/25.
//

#ifndef OPENGL_WAVEDISTUNDERTOWSLIDER_H
#define OPENGL_WAVEDISTUNDERTOWSLIDER_H
#include "../BKComponents/BKSliders.h"
#include "WaveDistUndertowParams.h"
#include "open_gl_component.h"
#include "synth_slider.h"
#include "valuetree_utils/VariantConverters.h"
#include "juce_data_structures/juce_data_structures.h"

class OpenGL_WaveDistUndertowSlider : public OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>, BKWaveDistanceUndertowSlider::Listener
{
public:
    OpenGL_WaveDistUndertowSlider (WaveDistUndertowParams *_params, chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>(
                                                                              "WAVE", // slider name
                                                                              0.f, // min
                                                                              20000.f, // max
                                                                              0.f, // default min
                                                                              20000.f, // default max
                                                                              1 ,//increment
                                                                             _params->stateChanges.defaultState
                                                                          ),
                                                                          params(_params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulated_ = true;
        addMyListener(this);

        /*
         * these SliderAttachments contain the listeners for each slider, and so will be notified when the slider is manipulated
         * the attachmentVec is just a place to put them, since some objects have might have many or variable number of them.
         * these attach the sliders changes to the chowdsp params, so they can be managed appropriately, with the callbacks
         * in the "sliderChangedCallback" list called
         */
        auto wavedistanceSliderPtr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[0].get(),
                                                                        listeners,
                                                                        wavedistanceSlider, nullptr);
        attachmentVec.emplace_back(std::move(wavedistanceSliderPtr));
        auto undertowSliderPtr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[1].get(),
                                                                listeners,
                                                                undertowSlider, nullptr);
        attachmentVec.emplace_back(std::move(undertowSliderPtr));
        wavedistanceValueTF.setText(juce::String(wavedistanceSlider.getValue()),juce::dontSendNotification);
        undertowValueTF.setText(juce::String(undertowSlider.getValue()),juce::dontSendNotification);
        /**
         * this list of callbacks is handled by the chowdsp system.
         * each of these will be called, and the lambda executed, when the parameters are changed anywhere in the code
         */


    }

    virtual void resized() override {
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::resized();
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::mouseDrag(e);
        redoImage();
    }

    virtual void mouseMove(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::mouseMove(e);
        redoImage();
    }

    void mouseDown(const juce::MouseEvent &e) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::mouseDown(e);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        mouseInteraction = true;
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::textEditorReturnKeyPressed(textEditor);
        redoImage();
        mouseInteraction = false;
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        // mouseInteraction = true;
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::textEditorFocusLost(textEditor);
        redoImage();
        // mouseInteraction = false;
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        // mouseInteraction = true;
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::textEditorEscapeKeyPressed(textEditor);
        redoImage();
        // mouseInteraction = false;
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        // mouseInteraction = true;
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::textEditorTextChanged(textEditor);
        redoImage();
        // mouseInteraction = false;
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>::mouseUp(event);
        redoImage();
        mouseInteraction = false;
    }

    OpenGL_WaveDistUndertowSlider* clone() {
        return new OpenGL_WaveDistUndertowSlider();
    }

    /*
     * this is called whenever the transposition slider "isModulated_" is edited by the user
     * OR when the modulation transposition slider "isModulation_" is edited by the user
     * this is NOT called when an actual state modulation is executed
     */
    void BKWaveDistanceUndertowSliderValueChanged(juce::String name, double wavedist, double undertow) override {

        /*
         * we don't want these to be called when a modulation or reset is triggered
         */
        if (!mouseInteraction)
            return;

        if (isModulation_) {
            modulationState.setProperty("wavedist", wavedist, nullptr);
            modulationState.setProperty("undertow", undertow, nullptr);
        }

        else if (isModulated_) {
            defaultState.setProperty("wavedist", wavedist, nullptr);
            defaultState.setProperty("undertow", undertow, nullptr);
        }
    }

    void updateSliderPositionsGL(juce::Array<int> newpositions)
    {
        updateSliderPositions (newpositions);
        redoImage();
    }

    /**
     * syncToValueTree() is called in ModulationManager::modulationClicked and
     * is used to set the mod view of the parameter to the most recent mod values
     */
    void syncToValueTree() override {
        // juce::Array<float> vals;
        // static juce::var nullVar;
        // for (int i = 0; i < 12; i++) {
        //     auto str = "t" + juce::String(i);
        //     auto val = modulationState.getProperty(str);
        //     if (val == nullVar)
        //         break;
        //     vals.add(val);
        // }

        // setTo(vals, juce::sendNotification);
    }

    void mouseExit(const juce::MouseEvent &e) override {
        for (auto *listener: listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    WaveDistUndertowParams *params;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

private :
    /**
     * we use this constructor to create a generic clone for the mods
     * so "isModulation" is set to true here,
     * as this will be used to set the modulation target values
     */
    OpenGL_WaveDistUndertowSlider() : OpenGlAutoImageComponent<BKWaveDistanceUndertowSlider>(
                                                                              "WAVE", // slider name
                                                                              0.f, // min
                                                                              20000.f, // max
                                                                              0.f, // default min
                                                                              20000.f, // default max
                                                                              1 ,//increment
                                                                              juce::ValueTree{}
                                                                          )

    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);

        isModulation_ = true;
        addMyListener(this);
    }

    bool mouseInteraction = false;
    chowdsp::ScopedCallbackList sliderChangedCallback;
};

#endif //OPENGL_WAVEDISTUNDERTOWSLIDER_H
