//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_TUNINGPARAMETERSVIEW_H
#define BITKLAVIER2_TUNINGPARAMETERSVIEW_H
#include "envelope_section.h"
#include "TransposeParams.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_combo_box.h"
#include "OpenGL_AbsoluteKeyboardSlider.h"
#include "TuningProcessor.h"
#include "tuning_systems.h"
#include "SemitoneWidthSection.h"

class TuningParametersView : public SynthSection,BKTuningKeyboardSlider::Listener
{
public:
    TuningParametersView(chowdsp::PluginState& pluginState, TuningParams& param, juce::String name, OpenGlWrapper *open_gl) : SynthSection(""), params(param)
    {
        setName("tuning");
        setLookAndFeel(DefaultLookAndFeel::instance());
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

        keyboard = std::make_unique<OpenGLAbsoluteKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState);
        addStateModulatedComponent(keyboard.get());

        circular_keyboard = std::make_unique<OpenGLCircularKeyboardSlider>(dynamic_cast<TuningParams*>(&params)->tuningState);
        addStateModulatedComponent(circular_keyboard.get());

        //semitoneSection = std::make_unique<SemiToneWidthSection>("SEMITONESECTION", "SEMITONESECTION", params.semitoneWidthParams, listeners, *this);
        semitoneSection = std::make_unique<SemiToneWidthSection>(name, params.semitoneWidthParams, listeners, *this);
        addSubSection(semitoneSection.get());

      // for (auto &param_ : *params.getChoiceParams()) {
      //    auto box = std::make_unique<OpenGLComboBox>(param_->paramID.toStdString());
      //     auto attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*param_.get(), listeners,*box.get(), nullptr);
      //     addAndMakeVisible(box.get());
      //     addOpenGlComponent(box->getImageComponent());
      //     combo_box_attachments.emplace_back(std::move(attachment));
      //     _comboBoxes.emplace_back(std::move(box));
      // }

        if (auto* tuningParams = dynamic_cast<TuningParams*>(&params)) {
            ///tuning systems
            auto index = tuningParams->tuningSystem->getIndex();
            tuning_combo_box = std::make_unique<OpenGLComboBox>(tuningParams->tuningSystem->paramID.toStdString());
            tuning_attachment= std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningSystem.get(), listeners,*tuning_combo_box, nullptr);
            addAndMakeVisible(tuning_combo_box.get());
            addOpenGlComponent(tuning_combo_box->getImageComponent());

            // clear the default menu so we can make submenus
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
                if (!params.tuningState.setFromAudioThread) {
                    TuningSystem t = params.tuningSystem->get();

                    //this->params.tuningState.circularTuningOffset = tuningMap[t].second;
                    auto it = std::find_if(tuningMap.begin(), tuningMap.end(),
                           [t](const auto& pair) {
                               return pair.first == t;
                           });
                    if (it->first == TuningSystem::Custom) {

                    }
                    else if (it != tuningMap.end()) {
                        const auto& tuning = it->second;
                        const auto tuningArray = TuningState::rotateValuesByFundamental(tuning, params.fundamental->getIndex());
                        int index  = 0;
                        for (const auto val :tuningArray) {
                            this->params.tuningState.circularTuningOffset[index] = val * 100;
                            DBG("new tuning " + juce::String(index) + " " + juce::String(this->params.tuningState.circularTuningOffset[index]));
                            index++;
                        }
                    }
                }

                params.tuningState.setFromAudioThread = false;
                circular_keyboard->redoImage();
            }
            )
        };

        tuningComboBoxCallbacks += {listeners.addParameterListener(
            params.fundamental,
            chowdsp::ParameterListenerThread::MessageThread,
            [this]() {
                params.tuningState.setFundamental(params.fundamental->getIndex());
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

    TuningParams& params;
    std::unique_ptr<SemiToneWidthSection> semitoneSection;
};

#endif //BITKLAVIER2_TUNINGPARAMETERSVIEW_H
