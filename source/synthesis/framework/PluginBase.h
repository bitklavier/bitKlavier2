//
// Created by Davis Polito on 10/1/24.
//

#ifndef BITKLAVIER2_PLUGINBASE_H
#define BITKLAVIER2_PLUGINBASE_H
#include "Identifiers.h"
#include "SampleLoadManager.h"
#include "bk_XMLSerializer.h"
#include "synth_base.h"
#include <chowdsp_plugin_base/chowdsp_plugin_base.h>
#include "buffer_debugger.h"

class SynthSection;
class SynthBase;
class TuningState;
class TuningProcessor;
class TempoProcessor;
class SynchronicProcessor;
template<typename T>
class BKSamplerSound;

namespace bitklavier {
    class InternalProcessor : public juce::AudioProcessor {
    public:
        InternalProcessor(juce::AudioProcessor::BusesProperties layout,
                          const juce::ValueTree &_v) : juce::AudioProcessor(layout), v(_v) {
        }

        InternalProcessor() : juce::AudioProcessor() {
        }

        juce::ScopedPointer<BufferDebugger> bufferDebugger = new BufferDebugger();

        virtual void setTuning(TuningProcessor *tun) {
            tuning = tun;
        }

        virtual void setTempo(TempoProcessor *tem) {
            tempo = tem;
        }

        virtual void setSynchronic(SynchronicProcessor *syn) {
            synchronic = syn;
        }

        TuningProcessor* getTuning() const { return tuning; }
        TempoProcessor* getTempo() const { return tempo; }
        SynchronicProcessor* getSynchronic() const { return synchronic; }

        virtual void addSoundSet(
            juce::ReferenceCountedArray<BKSynthesiserSound> *s, // main samples
            juce::ReferenceCountedArray<BKSynthesiserSound> *h, // hammer samples
            juce::ReferenceCountedArray<BKSynthesiserSound> *r, // release samples
            juce::ReferenceCountedArray<BKSynthesiserSound> *p) {
        } // pedal samples;

        juce::ValueTree v;

    protected:
        TuningProcessor *tuning = nullptr;
        TempoProcessor *tempo = nullptr;
        SynchronicProcessor *synchronic = nullptr;
    };

    /**
      * Base class for plugin processors.
      *
      * Derived classes must override `prepareToPlay` and `releaseResources`
      * (from `juce::AudioProcessor`), as well as `processAudioBlock`, and
      * `addParameters`.
     */
#if JUCE_MODULE_AVAILABLE_chowdsp_plugin_state
    template<class PluginStateType>
#else
    template <class Processor>
#endif
    class PluginBase : public InternalProcessor
#if JUCE_MODULE_AVAILABLE_chowdsp_clap_extensions
        ,
                       public CLAPExtensions::CLAPInfoExtensions,
                       public clap_juce_extensions::clap_juce_audio_processor_capabilities
#endif
    {
    public:
        explicit PluginBase(SynthBase &parent, const juce::ValueTree &v, juce::UndoManager *um = nullptr,
                            const juce::AudioProcessor::BusesProperties &layout = getDefaultBusLayout());

        ~PluginBase() override = default;

#if defined JucePlugin_Name
        const juce::String getName() const override // NOLINT(readability-const-return-type): Needs to return a const juce::String for override compatibility
        {
            return JucePlugin_Name;
        }
#else
        const juce::String getName() const override // NOLINT(readability-const-return-type): Needs to return a const juce::String for override compatibility
        {
            return juce::String();
        }
#endif

        bool acceptsMidi() const override {
            return false;
        }

        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }

        double getTailLengthSeconds() const override { return 0.0; }

        int getNumPrograms() override;

        int getCurrentProgram() override;

        void setCurrentProgram(int) override;

        const juce::String getProgramName(int) override;

        void changeProgramName(int, const juce::String &) override;

#if JUCE_MODULE_AVAILABLE_chowdsp_presets_v2
        virtual presets::PresetManager& getPresetManager()
        {
            return *presetManager;
        }
#elif JUCE_MODULE_AVAILABLE_chowdsp_presets
        virtual PresetManager& getPresetManager()
        {
            return *presetManager;
        }

        void setUsePresetManagerForPluginInterface (bool shouldUse)
        {
            programAdaptor = shouldUse
                                 ? std::make_unique<ProgramAdapter::PresetsProgramAdapter> (presetManager)
                                 : std::make_unique<ProgramAdapter::BaseProgramAdapter>();
        }
#endif

        bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;

        void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

        void processBlock(juce::AudioBuffer<double> &, juce::MidiBuffer &) override {
        }

        virtual void processAudioBlock(juce::AudioBuffer<float> &) = 0;

        bool hasEditor() const override {
            return true;
        }
#if JUCE_MODULE_AVAILABLE_foleys_gui_magic
        juce::AudioProcessorEditor* createEditor() override
        {
            return new foleys::MagicPluginEditor (magicState);
        }
#endif

        void getStateInformation(juce::MemoryBlock &data) override;

        void setStateInformation(const void *data, int sizeInBytes) override;

#if JUCE_MODULE_AVAILABLE_chowdsp_plugin_state
        auto &getState() {
            return state;
        }

        const auto &getState() const {
            return state;
        }
#endif

        virtual juce::String getWrapperTypeString() const;

        bool supportsParameterModulation() const;

        void resetContinuousModulations() {
            DBG("PluginBase::resetContinuousModulations");
            parent.requestResetAllContinuousModsRT();
        }

        void processContinuousModulations(juce::AudioBuffer<float> &buffer) {

            // 1. Find the actual index of the "Modulation" input bus
            int modBusIdx = -1;
            for (int i = 0; i < getBusCount(true); ++i) {
                if (getBus(true, i)->getName() == "Modulation") {
                    modBusIdx = i;
                    break;
                }
            }

            if (modBusIdx == -1)
            {
                DBG("processContinuousModulations: no modulation bus found");
                return; // No modulation bus found
            }

            const auto &modBus = getBusBuffer(buffer, true, modBusIdx);

            int numInputChannels = modBus.getNumChannels();
            for (int channel = 0; channel < numInputChannels / 2; ++channel) {
            //for (int channel = 0; channel < juce::jmin (numInputChannels / 2, (int) state.params.modulatableParams.size()); ++channel) {
                const float *in = modBus.getReadPointer(channel);
                const float *in_continous = modBus.getReadPointer(channel + (numInputChannels/2));
                auto p = state.params.modulatableParams[channel];
                p->applyMonophonicModulation(*in);
                parent.getParamOffsetBank().setOffset(p->getParamOffsetIndex(), p->getCurrentValue());
                p->applyMonophonicModulation(*in + *in_continous);
                bufferDebugger->capture("m"+juce::String(channel) + "    " + p->getParameterID(), modBus.getReadPointer(channel), modBus.getNumSamples(), -1.f, 1.f);
                bufferDebugger->capture("mc"+juce::String(channel + (numInputChannels/2)) + " " + p->getParameterID(), modBus.getReadPointer(channel + (numInputChannels/2)), modBus.getNumSamples(), -1.f, 1.f);
            }
        }

        /**
         * generates mappings between audio-rate modulatable parameters and the audio channel the modulation comes in on
         *      from a modification preparation
         *      modulations like this come on an audio channel
         *      this is on a separate bus from the regular audio graph that carries audio between preparations
         */
        void setupModulationMappings() {
            auto mod_params = v.getOrCreateChildWithName(IDs::MODULATABLE_PARAMS, nullptr);

            // Create a temporary vector to store the children in the desired order
            juce::Array<juce::ValueTree> orderedChildren;

            for (auto param: state.params.modulatableParams) {
                juce::String name = param->paramID;
                auto vt = mod_params.getChildWithProperty(IDs::parameter, name);

                if (!vt.isValid()) {
                    vt = juce::ValueTree{IDs::MODULATABLE_PARAM};
                    vt.setProperty(IDs::parameter, name, nullptr);
                    const auto &a = param->getNormalisableRange();
                    vt.setProperty(IDs::start, a.start, nullptr);
                    vt.setProperty(IDs::end, a.end, nullptr);
                    vt.setProperty(IDs::skew, a.skew, nullptr);
                }

                // Update properties
                param->setRangeToValueTree(vt);
                if (v.hasProperty(name)) {
                    vt.setProperty(IDs::sliderval, v.getProperty(name), nullptr);
                    param->setParameterValue(v.getProperty(name));
                } else {
                    vt.setProperty(IDs::sliderval, param->get(), nullptr);
                }

                // Setup audio modulation offset
                const int offsetIdx = parent.getParamOffsetBank().addParam(v, name);
                param->setParamOffsetIndex(offsetIdx);

                orderedChildren.add(vt);
            }

            // Now rebuild mod_params children in the correct order
            mod_params.removeAllChildren(nullptr);
            for (auto& child : orderedChildren) {
                mod_params.appendChild(child, nullptr);
            }
        }

    protected:
        SynthBase &parent;
#if JUCE_MODULE_AVAILABLE_chowdsp_plugin_state
        PluginStateType state;
#else
        using juce::Parameters = chowdsp::Parameters;
        juce::AudioProcessorValueTreeState vts;

#if JUCE_MODULE_AVAILABLE_foleys_gui_magic
        foleys::MagicProcessorState magicState { *this, vts };
#endif
#endif

#if JUCE_MODULE_AVAILABLE_chowdsp_presets_v2
        std::unique_ptr<presets::PresetManager> presetManager {};
        std::unique_ptr<ProgramAdapter::BaseProgramAdapter> programAdaptor = std::make_unique<ProgramAdapter::BaseProgramAdapter>();
#elif JUCE_MODULE_AVAILABLE_chowdsp_presets
        std::unique_ptr<PresetManager> presetManager;
        std::unique_ptr<ProgramAdapter::BaseProgramAdapter> programAdaptor = std::make_unique<ProgramAdapter::PresetsProgramAdapter> (presetManager);
#else
        //std::unique_ptr<chowdsp::ProgramAdapter::BaseProgramAdapter> programAdaptor = std::make_unique<chowdsp::ProgramAdapter::BaseProgramAdapter>();
#endif

    private:
        static juce::AudioProcessor::BusesProperties getDefaultBusLayout();

#if !JUCE_MODULE_AVAILABLE_chowdsp_plugin_state
        juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

        CHOWDSP_CHECK_HAS_STATIC_METHOD (HasAddParameters, addParameters)
#endif

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBase)
    };

    template<class P>
    juce::AudioProcessor::BusesProperties PluginBase<P>::getDefaultBusLayout() {
        return BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true);
    }

#if JUCE_MODULE_AVAILABLE_chowdsp_plugin_state

    template<class State>
    PluginBase<State>::PluginBase(SynthBase &_parent, const juce::ValueTree &v_, juce::UndoManager *um,
                                  const juce::AudioProcessor::BusesProperties &layout)
        : InternalProcessor(layout, v_),
          parent(_parent),
          state(*this, v_, um) {
        if (v.isValid())
            chowdsp::Serialization::deserialize<bitklavier::XMLSerializer>(v.createXml(), state);
        createUuidProperty(v);
        if (!v.hasProperty(IDs::soundset)) {
            v.setProperty(IDs::soundset, IDs::syncglobal.toString(), nullptr);
        }

        setupModulationMappings();
    }
#else
    template <class Processor>
    PluginBase<Processor>::PluginBase (juce::UndoManager* um, const juce::AudioProcessor::BusesProperties& layout) : juce::AudioProcessor (layout),
                                                                                                                     vts (*this, um, juce::Identifier ("Parameters"), createParameterLayout())
    {
    }

    template <class Processor>
    juce::AudioProcessorValueTreeState::ParameterLayout PluginBase<Processor>::createParameterLayout()
    {
        juce::Parameters params;

        static_assert (HasAddParameters<Processor>, "Processor class MUST contain a static addParameters function!");
        Processor::addParameters (params);

        return { params.begin(), params.end() };
    }
#endif

    template<class P>
    int PluginBase<P>::getNumPrograms() {
        return 1;
    }

    template<class P>
    int PluginBase<P>::getCurrentProgram() {
        return 1;
    }

    template<class P>
    void PluginBase<P>::setCurrentProgram(int index) {
        return;
    }

    template<class P>
    const juce::String PluginBase<P>::getProgramName(int index) // NOLINT(readability-const-return-type): Needs to return a const juce::String for override compatibility
    {
        return juce::String("");
    }

    template<class P>
    void PluginBase<P>::changeProgramName(int index, const juce::String &newName) {
        return;
    }

    template<class P>
    bool PluginBase<P>::isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const {
        // // only supports mono and stereo (for now)
        // if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        //     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        //     return false;
        //
        // // input and output layout must be the same
        // if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        //     return false;

        return true;
    }

    template<class P>
    void PluginBase<P>::prepareToPlay(double sampleRate, int samplesPerBlock) {
        setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
#if JUCE_MODULE_AVAILABLE_foleys_gui_magic
        magicState.prepareToPlay (sampleRate, samplesPerBlock);
#endif
    }

    template<class P>
    void PluginBase<P>::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) {
        juce::ScopedNoDenormals noDenormals;

#if JUCE_MODULE_AVAILABLE_chowdsp_plugin_state
        state.getParameterListeners().callAudioThreadBroadcasters();
#endif

        // processAudioBlock(buffer);
    }

    // magic number to identify memory blocks that we've stored as XML
    const juce::uint32 magicXmlNumber = 0x21324356;
#if JUCE_MODULE_AVAILABLE_chowdsp_plugin_state
    template<class State>
    void PluginBase<State>::getStateInformation(juce::MemoryBlock &data) {
        //    {
        //        juce::MemoryOutputStream out (data, false);
        //        out.writeInt (magicXmlNumber);
        //        out.writeInt (0);
        //        xml.writeTo (out, juce::XmlElement::TextFormat().singleLine());
        //        out.writeByte (0);
        //    }
        const auto &param_default = v.getChildWithName(IDs::PARAM_DEFAULT);

        if (!param_default.isValid() || !v.isValid())
            return;

        // Copy every property in `from` onto `to` (overwrites same-name props, leaves others untouched)
        for (int i = 0; i < param_default.getNumProperties(); ++i) {
            const juce::Identifier name = param_default.getPropertyName(i);
            const juce::var value = param_default.getProperty(name);
            v.setProperty(name, value, nullptr);
        }
        state.serialize(data);
    }

    template<class State>
    void PluginBase<State>::setStateInformation(const void *data, int sizeInBytes) {
        state.deserialize(juce::MemoryBlock{data, (size_t) sizeInBytes});
    }
#else
    template <class Processor>
    void PluginBase<Processor>::getStateInformation (juce::MemoryBlock& data)
    {
#if JUCE_MODULE_AVAILABLE_foleys_gui_magic
        magicState.getStateInformation (data);
#else
        auto state = vts.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, data);
#endif
    }

    template <class Processor>
    void PluginBase<Processor>::setStateInformation (const void* data, int sizeInBytes)
    {
#if JUCE_MODULE_AVAILABLE_foleys_gui_magic
        magicState.setStateInformation (data, sizeInBytes, getActiveEditor());
#else
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

        if (xmlState != nullptr)
            if (xmlState->hasTagName (vts.state.getType()))
                vts.replaceState (juce::ValueTree::fromXml (*xmlState));
#endif
    }
#endif

    template<class P>
    juce::String PluginBase<P>::getWrapperTypeString() const {
#if JUCE_MODULE_AVAILABLE_chowdsp_clap_extensions
        return CLAPExtensions::CLAPInfoExtensions::getPluginTypeString (wrapperType);
#else
        return juce::AudioProcessor::getWrapperTypeDescription(wrapperType);
#endif
    }

    template<class P>
    bool PluginBase<P>::supportsParameterModulation() const {
#if JUCE_MODULE_AVAILABLE_chowdsp_clap_extensions
        return CLAPExtensions::CLAPInfoExtensions::is_clap;
#else
        return false;
#endif
    }
} // bitklavier

#endif //BITKLAVIER2_PLUGINBASE_H
