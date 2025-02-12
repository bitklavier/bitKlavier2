//
// Created by Dan Trueman on 11/5/24.
//

#ifndef BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#define BITKLAVIER2_DIRECTPARAMETERSVIEW_H
#include "ParametersView.h"
#include "envelope_section.h"
#include "TransposeParams.h"
#include "OpenGlTranspositionSlider.h"

class DirectParametersView : public bitklavier::ParametersView
{
public:
    DirectParametersView(chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params,juce::String name, OpenGlWrapper *open_gl) :
                                                                                                                     bitklavier::ParametersView(pluginState,params,open_gl,false)
    {
        //envelope = std::make_unique<EnvelopeSection>("ENV", "err");
        setName("");
        //addSubSection(envelope.get());
        setComponentID(name);
        auto& listeners = pluginState.getParameterListeners();
        params.doForAllParameterContainers(
                [this,&listeners](auto &paramVec) {
                    for (auto &param: paramVec) {
                        comps.push_back(
                                bitklavier::parameters_view_detail::createParameterComp(listeners, param, *this));
                    }
                },
                [this, &listeners, &pluginState](auto &paramHolder) {
                    if(auto *transposeParam = dynamic_cast<TransposeParams*>(&paramHolder)) {
                        transpositionSlider = std::make_unique<OpenGlTranspositionSlider>(transposeParam, listeners);
                       transposeParam->doForAllParameters([this, &pluginState] (auto& param, size_t indexInParamHolder)
                        {
                            transposeCallbacks += { pluginState.addParameterListener(param,
                                   chowdsp::ParameterListenerThread::MessageThread,
                                   [this] {

                                   })};
                        });
                    }
                    else
                    {
                        auto section = bitklavier::parameters_view_detail::createEditorSection(paramHolder, listeners, *this);
                        addSubSection(section.get());
                        paramHolderComps.push_back(std::move(section));
                    }
                });


        addAndMakeVisible(*transpositionSlider);
        addOpenGlComponent(transpositionSlider->getImageComponent());
        // Using std::find_if
        auto it = std::find_if(comps.begin(), comps.end(),
                               [](const std::unique_ptr<juce::Component>& p) { return p->getName() == "UseTuning"; });

        _ASSERT(it != comps.end());
        transpose_uses_tuning = std::move(*it);
        comps.erase(it);

    }
//    std::unique_ptr<EnvelopeSection> envelope;
    std::unique_ptr<juce::Component> transpose_uses_tuning;
    std::unique_ptr<OpenGlTranspositionSlider> transpositionSlider;
    void resized() override;
    chowdsp::ScopedCallbackList transposeCallbacks;


};

#endif //BITKLAVIER2_DIRECTPARAMETERSVIEW_H
