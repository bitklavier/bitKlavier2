//
// Created by Davis Polito on 11/13/24.
//

#ifndef BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
#define BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
#include "BKSliders.h"
#include "open_gl_component.h"
#include "juce_data_structures/juce_data_structures.h"
/************************************************************************************/
/*                              CLASS: OpenGlSlider                                 */
/************************************************************************************/

class OpenGlTranspositionSlider : public OpenGlAutoImageComponent<BKStackedSlider>, BKStackedSlider::Listener {
public:
    OpenGlTranspositionSlider(TransposeParams *params,
                              chowdsp::ParameterListeners &listeners) : OpenGlAutoImageComponent<BKStackedSlider>(
                                                                            "Transpositions", // slider name
                                                                            -12, // min
                                                                            12, // max
                                                                            -12, // default min
                                                                            12, // default max
                                                                            0, // default val
                                                                            0.01), // increment
                                                                        params(params)
    //-12, 12, -12, 12, 0, 0.01
    {
        // increment
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        int i = 0;
        for (auto slider: dataSliders) {
            //chowdsp::SemitonesParameter::Ptr param = params->addNewSliderParam();
            auto ptr = std::make_unique<chowdsp::SliderAttachment>(*(*params->getFloatParams())[i++].get(), listeners,
                                                                   *slider, nullptr);
            attachmentVec.emplace_back(std::move(ptr));
        }
    }


    virtual void resized() override {
        OpenGlAutoImageComponent<BKStackedSlider>::resized();
        // if (isShowing())
        redoImage();
    }

    virtual void mouseDrag(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseDrag(e);
        redoImage();
    }

    void mouseDown(const juce::MouseEvent &e) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseDown(e);
        redoImage();
    }

    void textEditorReturnKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorReturnKeyPressed(textEditor);
        redoImage();
    }

    void textEditorFocusLost(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorFocusLost(textEditor);
        redoImage();
    }

    void textEditorEscapeKeyPressed(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorEscapeKeyPressed(textEditor);
        redoImage();
    }

    void textEditorTextChanged(juce::TextEditor &textEditor) override {
        OpenGlAutoImageComponent<BKStackedSlider>::textEditorTextChanged(textEditor);
        redoImage();
    }

    void mouseUp(const juce::MouseEvent &event) override {
        OpenGlAutoImageComponent<BKStackedSlider>::mouseUp(event);
        redoImage();
    }

    OpenGlTranspositionSlider *clone() {
        return new OpenGlTranspositionSlider();
    }

    void BKStackedSliderValueChanged(juce::String name, juce::Array<float> val) override {
        if (isModulation_) {
            for (int i = 0; i < val.size(); i++) {
                auto str = "t" + juce::String(i);
                modulationState.setProperty(str, val[i], nullptr);
            }
            //           modulationState.setProperty("t0",val[0],nullptr);
            //           modulationState.setProperty("t1",val[1],nullptr);
            //           modulationState.setProperty("t2",val[2],nullptr);
            //           modulationState.setProperty("t3",val[3],nullptr);
            //           modulationState.setProperty("t4",val[4],nullptr);
            //           modulationState.setProperty("t5",val[5],nullptr);
            //           modulationState.setProperty("t6",val[6],nullptr);
            //           modulationState.setProperty("t7",val[7],nullptr);
            //           modulationState.setProperty("t8",val[8],nullptr);
            //           modulationState.setProperty("t9",val[9],nullptr);
            //           modulationState.setProperty("t10",val[10],nullptr);
            //           modulationState.setProperty("t11",val[11],nullptr);
        }
    }

    std::vector<std::unique_ptr<chowdsp::SliderAttachment> > attachmentVec;

    void syncToValueTree() override {
        juce::Array<float> vals;
        static juce::var nullVar;
        for (int i = 0; i < 11; i++) {
            auto str = "t" + juce::String(i);
            auto val = modulationState.getProperty(str);
            vals.add(val);
            if (val == nullVar)
                break;
        }

        setTo(vals, juce::sendNotification);
    }

    void mouseExit(const juce::MouseEvent &e) override {
        if (getTextEditor()->isVisible())
            return;
        for (auto *listener: listeners_)
            listener->hoverEnded(this);
        hovering_ = false;
    }

    TransposeParams *params;

private :
    OpenGlTranspositionSlider() : OpenGlAutoImageComponent<BKStackedSlider>(
        "Transpositions", // slider name
        -12, // min
        12, // max
        -12, // default min
        12, // default max
        0, // default val
        0.01) // increment

    {
        // increment
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        isModulation_ = true;
        addMyListener(this);
    }
};

#endif //BITKLAVIER2_OPENGLTRANSPOSITIONSLIDER_H
