//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_TUNINGPARAMETERSVIEW_H
#define BITKLAVIER2_TUNINGPARAMETERSVIEW_H
#include "ParametersView.h"
#include "envelope_section.h"
#include "TransposeParams.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "../components/opengl/open_gl_combo_box.h"
#include "../components/opengl/OpenGL_AbsoluteKeyboardSlider.h"
#include "TuningProcessor.h"
#include "tuning_systems.h"
using SliderAttachmentTuple = std::tuple<std::shared_ptr<SynthSlider>, std::unique_ptr<chowdsp::SliderAttachment>>;
using BooleanAttachmentTuple = std::tuple<std::shared_ptr<SynthButton>, std::unique_ptr<chowdsp::ButtonAttachment>>;
class TuningParametersView : public SynthSection,BKTuningKeyboardSlider::Listener
{
public:
    TuningParametersView(chowdsp::PluginState& pluginState, TuningParams& param,juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
                                                                                                                     //bitklavier::ParametersView(pluginState,params,open_gl,false)
    {
        //envelope = std::make_unique<EnvelopeSection>("ENV", "err");
        setName("tuning");
        setLookAndFeel(DefaultLookAndFeel::instance());

        //addSubSection(envelope.get());
        setComponentID(name);
        auto& listeners = pluginState.getParameterListeners();
        for ( auto &param_ : *params.getFloatParams())
        {
            auto slider = std::make_unique<SynthSlider>(param_->paramID);
            auto attachment = std::make_unique<chowdsp::SliderAttachment>(*param_.get(), listeners, *slider.get(), nullptr);
            addSlider(slider.get());
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            floatAttachments.emplace_back(std::move(attachment));
            _sliders.emplace_back(std::move(slider));

        }
        keyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->keyboardState);
        addStateModulatedComponent(keyboard.get());
        circular_keyboard = std::make_unique<OpenGLCircularKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->keyboardState);
        addStateModulatedComponent(circular_keyboard.get());
      // for (auto &param_ : *params.getChoiceParams()) {
      //    auto box = std::make_unique<OpenGLComboBox>(param_->paramID.toStdString());
      //     auto attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*param_.get(), listeners,*box.get(), nullptr);
      //     addAndMakeVisible(box.get());
      //     addOpenGlComponent(box->getImageComponent());
      //     combo_box_attachments.emplace_back(std::move(attachment));
      //     _comboBoxes.emplace_back(std::move(box));
      // }
        if (auto* tuningParams = dynamic_cast<TuningParams*>(&params)) {
            ///tuninng systems
            auto index = tuningParams->tuningSystem->getIndex();
            tuning_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningSystem->paramID.toStdString());
            tuning_attachment= std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningSystem.get(), listeners,*tuning_combo_box, nullptr);
            addAndMakeVisible(tuning_combo_box.get());
            addOpenGlComponent(tuning_combo_box->getImageComponent());
            tuning_combo_box->clear(juce::sendNotificationSync);
            juce::OwnedArray<juce::PopupMenu> submenus;
            submenus.add(new juce::PopupMenu());
            submenus.add(new juce::PopupMenu());
            int i = 0;
            for (auto choice : tuningParams->tuningSystem->choices) {
                if (i <= 6)
                    tuning_combo_box->addItem(choice,i+1);
                if (i>6 && i <33) {
                    submenus.getUnchecked(0)->addItem(i+1,choice);
                }
                else if (i>=33) {
                    submenus.getUnchecked(1)->addItem(i+1,choice);
                }

                i++;
            }
            tuning_combo_box->addSeparator();
            auto* pop_up = tuning_combo_box->getRootMenu();
            pop_up->addSubMenu("Historical",*submenus.getUnchecked(0));
            pop_up->addSubMenu("Various",*submenus.getUnchecked(1));
            tuning_combo_box->setSelectedItemIndex(index,juce::sendNotificationSync);
            //fundamental

            fundamental_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->fundamental->paramID.toStdString());
            fundamental_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->fundamental.get(), listeners,*fundamental_combo_box, nullptr);
            addAndMakeVisible(fundamental_combo_box.get());
            addOpenGlComponent(fundamental_combo_box->getImageComponent());
            adaptive_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->adaptive->paramID.toStdString());
            adaptive_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->adaptive.get(), listeners,*adaptive_combo_box, nullptr);
            addAndMakeVisible(adaptive_combo_box.get());
            addOpenGlComponent(adaptive_combo_box->getImageComponent());
        }
        tuningComboBoxCallbacks += {listeners.addParameterListener(
            params.tuningSystem,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                if (!params.keyboardState.setFromAudioThread) {
                    TuningSystem t = params.tuningSystem->get();

                    //this->params.keyboardState.circularTuningOffset = tuningMap[t].second;
                    auto it = std::find_if(tuningMap.begin(), tuningMap.end(),
                           [t](const auto& pair) {
                               return pair.first == t;
                           });
                    if (it->first == TuningSystem::Custom) {

                    }
                    else if (it != tuningMap.end()) {
                        const auto& tuning = it->second;
                        const auto tuningArray = TuningKeyboardState::rotateValuesByFundamental(tuning, params.fundamental->getIndex());
                        int index  = 0;
                        for (const auto val :tuningArray) {
                            this->params.keyboardState.circularTuningOffset[index] = val * 100;
                            index++;
                        }
                    }

                }
                params.keyboardState.setFromAudioThread = false;
                circular_keyboard->redoImage();
            }
            )
        };
        tuningComboBoxCallbacks += {listeners.addParameterListener(
            params.fundamental,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                params.keyboardState.setFundamental(params.fundamental->getIndex());
                circular_keyboard->redoImage();
                DBG("rotat");
            }
        )};
    circular_keyboard->addMyListener(this);

    }
    void paintBackground(juce::Graphics& g) override
    {
        SynthSection::paintContainer(g);
        paintHeadingText(g);
        paintBorder(g);
        paintKnobShadows(g);
        for (auto& slider : _sliders) {
            drawLabelForComponent(g, slider->getName(), slider.get());
        }
        paintChildrenBackgrounds(g);
    }
    void keyboardSliderChanged(juce::String name) override;
//    std::unique_ptr<EnvelopeSection> envelope;
    //std::unique_ptr<juce::Component> transpose_uses_tuning;
    void resized() override;
    std::vector<std::unique_ptr<SynthSlider>> _sliders;
    std::vector<std::unique_ptr<chowdsp::SliderAttachment>> floatAttachments;
    std::unique_ptr<OpenGLComboBox> tuning_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> tuning_attachment;
    std::unique_ptr<OpenGLComboBox> fundamental_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> fundamental_attachment;
    std::unique_ptr<OpenGLComboBox> adaptive_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> adaptive_attachment;
    std::unique_ptr<OpenGLAbsoluteKeyboardSlider> keyboard;
    std::unique_ptr<OpenGLCircularKeyboardSlider> circular_keyboard;
    chowdsp::ScopedCallbackList tuningComboBoxCallbacks;
    //    std::vector<std::unique_ptr<SynthButton>> _buttons;
    TuningParams& params;
//    std::vector<std::unique_ptr<chowdsp::ButtonAttachment>> buttonAttachments;
};

#endif //BITKLAVIER2_TUNINGPARAMETERSVIEW_H
