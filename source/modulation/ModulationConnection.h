//
// Created by Davis Polito on 12/10/24.
//

#ifndef BITKLAVIER_MODULATIONCONNECTION_H
#define BITKLAVIER_MODULATIONCONNECTION_H
#include "open_gl_component.h"
#include "valuetree_utils/VariantConverters.h"
#include <juce_data_structures/juce_data_structures.h>
#include "Identifiers.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <chowdsp_parameters/chowdsp_parameters.h>
#include "StateModulator.h"

class ModulatorBase;

namespace bitklavier {
class ModulationProcessor;
    struct ModulationConnection : ModulatorBase::Listener {
        ModulationConnection(const std::string& from, const std::string& to, int index)
            : source_name(from), destination_name(to),state(IDs::ModulationConnection)
        {
            createUuidProperty(state);
            state.setProperty(IDs::isState,false,nullptr);
            uuid = state.getProperty(IDs::uuid);
            index_in_all_mods = index;
            scalingValue_ = 0.0f;
            setDestination(to);
            setSource(from);
        }

        ~ModulationConnection()
        {
            //count--;
        }

        static bool isModulationSourceDefaultBipolar(const std::string& source);
        void setSource(const std::string& from)
        {
            if(from.contains("lfo")) {
                isContinuousMod = true;
            } else {
                isContinuousMod = false;
            }
            state.setProperty(IDs::src, juce::String(from), nullptr);
        }

        void setDestination(const std::string& uuid_to)
        {
            state.setProperty(IDs::dest, juce::String(uuid_to), nullptr);
        }
        std::atomic<float> modAmt_ { 0.0f }; // whatever “modVal” means in your system

        void setModulationAmount(float amt)
        {
            modAmt_.store (amt, std::memory_order_relaxed);
            state.setProperty(IDs::modAmt, amt, nullptr);
        }

        void setPolarity(bool isBipolar)
        {
            state.setProperty(IDs::isBipolar, isBipolar, nullptr);
            setBipolar(isBipolar);
        }

        void setIsOffset(bool isOffset)
        {
            state.setProperty(IDs::isOffsetMod, isOffset, nullptr);
            setIsOffset(isOffset);
        }

        void resetConnection(const std::string& from, const std::string& to) {
            source_name = from;
            destination_name = to;
            setSource(source_name);
            setDestination(destination_name);
            if (source_name == "" || destination_name == "")
            {
                state.getParent().removeChild(state,nullptr);
            }
        }

        float getCurrentBaseValue()
        {
                return state.getProperty(IDs::modAmt,  0.f);
        }

        void setBypass(bool bypass) {}
        void setStereo(bool stereo) {}
        bool isBipolar() const { return bipolar_; }
        bool isOffset() const { return offset_; }
        bool isBypass() const {return bypass_; }
        bool isStereo() const {return stereo_; }
        bool setDefaultBipolar (bool val)
        {
            defaultBipolar = val;
            setBipolar(val);
        }

        bool isDefaultBipolar() {
            return defaultBipolar;
        }

        void setBipolar(bool bipolar) {
            bipolar_ = bipolar;
        }

        void setOffsetMode(bool isOffset) {
            offset_ = isOffset;
        }

        void reset() {
            source_name = "";
            destination_name = "";
        }
        float getScaling() {
            return scalingValue_.load();
        }

        std::string source_name;
        std::string destination_name;        //must be named state to be picked up by valuetreeobjectlist - dont know
        // if i'll be using this for that or not
        int index_in_all_mods;
        int index_in_mapping;
        juce::Uuid uuid;
        bool bipolar_;
        bool offset_ = false; // true => mod value is offset from sliderVal, otherwise, false => mod value is literal target val; false is the only mode for now, for ramp mods
        bool bypass_;
        bool stereo_;
        bool defaultBipolar;
        juce::AudioProcessorGraph::Connection connection_;
        ModulationProcessor* parent_processor;
        ModulatorBase* processor;
        int modulation_output_bus_index;
        float currentDestinationSliderVal;

        void setParamTree(const juce::ValueTree& v) {
            param_tree = v;
            currentDestinationSliderVal = param_tree.getProperty(IDs::sliderval);
            // Use a safe initial call that guards against invalid ranges/values
            setScalingValue(modAmt_.load(std::memory_order_relaxed), currentDestinationSliderVal);
        }

        void setStateValueTree(const juce::ValueTree&v) {
            state = v;
            //setScalingValue()
            scalingValue_ = state.getProperty(IDs::mod0to1);
            modAmt_ = state.getProperty(IDs::modAmt);
        }

        juce::ValueTree state;
        void setScalingValue (float modVal, float sliderVal)
        {
            // compute modRangeNorm exactly as you do now...
            // (this is your pre-trigger authoring step)

            rangeStart_ = static_cast<float>(param_tree.getProperty(IDs::start));
            rangeEnd_   = static_cast<float>(param_tree.getProperty(IDs::end));
            rangeSkew_  = static_cast<float>(param_tree.getProperty(IDs::skew));
            float start = rangeStart_.load (std::memory_order_relaxed);
            float end   = rangeEnd_.load   (std::memory_order_relaxed);
            float skew  = rangeSkew_.load  (std::memory_order_relaxed);

            // Safety: ensure a valid, non-degenerate range
            if (end < start)
                std::swap(start, end);

            // Avoid zero-width ranges which would break NormalisableRange math
            if (juce::approximatelyEqual(start, end))
            {
                // Expand minimally around the center
                const float eps = 1.0e-6f;
                start -= eps;
                end   += eps;
            }

            // Skew must be positive; default to linear if invalid
            if (!(skew > 0.0f))
                skew = 1.0f;

            /*
             * don't rescale parameter range if mod exceeds it
             */
            // if (sliderVal + modVal > end)
            // {
            //     end = sliderVal + modVal;
            //     param_tree.setProperty(IDs::end,end,nullptr);
            // }
            // else if (sliderVal - modVal < start)
            // {
            //     start = sliderVal - modVal;
            //     param_tree.setProperty(IDs::start,start,nullptr);
            // }

            range =  juce::NormalisableRange<float> (start, end, 0.0f, skew);
            DBG("setScalingValue min/max/modVal/sliderVal = " << start << " " << end << " " << modVal << " " << sliderVal);
            // Clamp slider to the valid range before converting to 0..1 to satisfy JUCE assertions
            const float sliderValClamped = juce::jlimit(start, end, sliderVal);
            float sliderNorm = range.convertTo0to1(sliderValClamped);

            float modRangeNorm = 0.0f;

            if (isBipolar())
            {
                // Symmetric modulation up and down
                const float plusVal  = juce::jlimit(start, end, sliderValClamped + std::abs(modVal));
                const float minusVal = juce::jlimit(start, end, sliderValClamped - std::abs(modVal));
                float plusNorm  = range.convertTo0to1(plusVal);
                float minusNorm = range.convertTo0to1(minusVal);

                // Half the total range (from center to one side)
                modRangeNorm = 0.5f * std::abs(plusNorm - minusNorm);
            }
            else if (isOffset())
            {
                // Unipolar modulation (e.g., 0 to +modVal)
                //  - currently not used in bK
                const float targetVal = juce::jlimit(start, end, sliderValClamped + modVal);
                float targetNorm = range.convertTo0to1(targetVal);
                modRangeNorm = std::max(0.0f, targetNorm - sliderNorm);
            }
            else
            {
                // Mod is actual target val
                if(modVal > sliderVal)
                {
                    const float targetVal = juce::jlimit(start, end, modVal);
                    float targetNorm = range.convertTo0to1(targetVal);
                    modRangeNorm = targetNorm - sliderNorm; // mod system expects value that is offset from sliderNorm
                }
                else
                {
                    const float targetVal = juce::jlimit(start, end, modVal);
                    float targetNorm = range.convertTo0to1(targetVal);
                    modRangeNorm = (sliderNorm - targetNorm) * -1.f;
                }
            }

            state.setProperty(IDs::sliderval, sliderValClamped, nullptr);
            scalingValue_.store(modRangeNorm);
            state.setProperty(IDs::mod0to1, scalingValue_.load(),nullptr);
            currentDestinationSliderVal  = sliderValClamped;
            setModulationAmount(modVal);

            scalingValue_.store (modRangeNorm, std::memory_order_relaxed);

            state.setProperty(IDs::mod0to1, modRangeNorm, nullptr);
            state.setProperty(IDs::sliderval, sliderValClamped, nullptr);
            // state.setProperty(IDs::modAmt, modVal, nullptr);
        }

        // called when the modulator is actually triggered (audio thread)
        void lockScaling() noexcept
        {
            // lock only once
            bool expected = false;
            if (scalingLocked_.compare_exchange_strong (expected, true,
                                                       std::memory_order_acq_rel))
            {
                lockedScaling_.store (scalingValue_.load(std::memory_order_relaxed),
                                      std::memory_order_release);
            }
        }

        // called when modulation ends / reset (message thread or audio thread)
        void unlockScaling() noexcept
        {
            scalingLocked_.store (false, std::memory_order_release);
        }

        // this is what the audio thread should use
        float getScalingForDSP() const noexcept
        {
            if (scalingLocked_.load (std::memory_order_acquire))
                return lockedScaling_.load (std::memory_order_relaxed);

            return scalingValue_.load (std::memory_order_relaxed);
        }

        void modulationTriggered() //listener funciton
        {
        DBG("INACTIVE: ModulationConnection::modulationTriggered() for dest :" + destination_name + "src: "  + source_name);
        // changeBuffer->changeState.emplace_back(0,change);
        // lockScaling();
        }

        void resetTriggered()
        {
            DBG("ModulationConnection::resetTriggered()");
            unlockScaling();
            // changeBuffer->changeState.emplace_back(0,changeBuffer->defaultState);
        }

        int getDestParamIndex() {
            return destParamIndex;
        }

        int setDestParamIndex(int index) {
            destParamIndex = index;
        }

       void updateScalingAudioThread(float knobValueParamUnits) noexcept {
            // Don’t change once the mod has started
            if (scalingLocked_.load (std::memory_order_acquire))
                return;
            // Pull the current amount (modVal) from atomic
            const float modVal = modAmt_.load (std::memory_order_relaxed);
            const float start = rangeStart_.load (std::memory_order_relaxed);
            const float end   = rangeEnd_.load   (std::memory_order_relaxed);
            const float skew  = rangeSkew_.load  (std::memory_order_relaxed);

            const float base = juce::jlimit (start, end, knobValueParamUnits);
            const float baseNorm = range.convertTo0to1 (base);
            float scaleNorm = 0.0f;
            if (isBipolar())
            {
                const float d = std::abs (modVal);
                const float plusNorm  = range.convertTo0to1 (juce::jmin (base + d, end));
                const float minusNorm = range.convertTo0to1 (juce::jmax (base - d, start));
                scaleNorm = 0.5f * std::abs (plusNorm - minusNorm);
            }
            else if (isOffset())
            {
                const float targetNorm = range.convertTo0to1 (juce::jmin (base + modVal, end));
                scaleNorm = juce::jmax (0.0f, targetNorm - baseNorm);
            }
            else
            {
                // Your existing “target value” semantics branch:
                // here modVal is interpreted as the *target value* in param units.
                const float target = juce::jlimit (start, end, modVal);
                const float targetNorm = range.convertTo0to1 (target);
                scaleNorm = targetNorm - baseNorm;
            }
            scalingValue_.store (scaleNorm, std::memory_order_relaxed);
        }

        void updateScalingAudioThread (float currentValueParamUnits, float m /* this connection’s current mod sample */) noexcept;

        float setCurrentTotalBaseValue(float basevalue) {
            currentTotalBaseValue = basevalue;
        }

        void setCarryActive(float carry) {
            if (isContinuousMod)
                return;
            // carryApplied_.store(carry, std::memory_order_relaxed);
            carryActive_.store(true, std::memory_order_release); // only enable for ramp
        }

        std::atomic<bool> isContinuousMod{false};
        // bool requestRetrigger;
        // std::atomic<float> lastApplied_ { 0.0f };  // in mod-bus units (normalized contribution)
        std::atomic<float> carryApplied_ { 0.0f };   // scaled contribution captured at retrigger
        std::atomic<bool>  carryActive_  { false };
        // std::atomic<float> lastAppliedPrev_ { 0.0f };
        juce::NormalisableRange<float> range;

    private:
        float currentTotalBaseValue;
        std::atomic<float> scalingValue_   { 0.0f }; // editable pre-trigger
        std::atomic<float> lockedScaling_  { 0.0f }; // frozen at trigger time
        std::atomic<bool>  scalingLocked_  { false };
        // RT-safe cached range
        std::atomic<float> rangeStart_ { 0.0f }, rangeEnd_{ 1.0f }, rangeSkew_{ 1.0f };
        int destParamIndex;
        // std::atomic<float> currentSliderVal {0.0f};
        juce::ValueTree param_tree;
//        std::atomic<float>* bipolarOffset;
    };

    typedef struct mapping_change
    {
        bool disconnecting;
        ModulationConnection* connection;
        std::string destination;
        std::string source;
        int dest_param_index;
        int source_param_uuid;
    } mapping_change;

    class ModulationConnectionBank {
    public:
        ModulationConnectionBank();
        ~ModulationConnectionBank();
        ModulationConnection* createConnection(const std::string& from, const std::string& to);

        ModulationConnection* atIndex(int index) { return all_connections_[index].get(); }
        size_t numConnections() { return all_connections_.size(); }
        void reset() {
            for ( auto& c : all_connections_ ) {
                c->reset();
            }
        }

    private:
        std::vector<std::unique_ptr<ModulationConnection>> all_connections_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationConnectionBank)
    };

struct StateConnection : public ModulatorBase::Listener{
        StateConnection(const std::string& from, const std::string& to, int index)
                : source_name(from), destination_name(to),state(IDs::ModulationConnection)
        {
            createUuidProperty(state);
            uuid = state.getProperty(IDs::uuid);
            state.setProperty(IDs::isState,true,nullptr);
            index_in_all_mods = index;
            setSource(source_name);
            setDestination(destination_name);
        }

        ~StateConnection()
        {
            //count--;
        }

        void setSource(const std::string& from)
        {
            state.setProperty(IDs::src, juce::String(from), nullptr);
        }

        void setDestination(const std::string& uuid_to)
        {
            state.setProperty(IDs::dest, juce::String(uuid_to), nullptr);
        }

        void setModulationAmount(float amt)
        {
            state.setProperty(IDs::modAmt, amt, nullptr);
        }

        void setChangeBuffer(ParameterChangeBuffer* buf) {
            changeBuffer = buf;
        }

        void setChange(const juce::ValueTree& _change) {change = _change;}

        void resetConnection(const std::string& from, const std::string& to) {
            source_name = from;
            destination_name = to;
            setSource(source_name);
            setDestination(destination_name);
            if (source_name == "" || destination_name == "")
            {
                state.getParent().removeChild(state,nullptr);
            }
            changeBuffer = nullptr;

        }

        void modulationTriggered() //listener funciton
        {
            DBG("StateConnection::modulationTriggered()");
            changeBuffer->changeState.emplace_back(0,change);
        }

        void resetTriggered()
        {
            DBG("StateConnection::resetTriggered()");
            DBG(changeBuffer->defaultState.toXmlString());
            changeBuffer->changeState.emplace_back(0,changeBuffer->defaultState);
        }

        void reset() {
            source_name = "";
            destination_name = "";
        }

        std::string source_name;
        std::string destination_name; //must be named state to be picked up by valuetreeobjectlist - dont know if i'll be using this for that or not
        juce::ValueTree state;
        int index_in_all_mods;
        int index_in_mapping;
        juce::Uuid uuid;
        bool defaultBipolar;
        juce::AudioProcessorGraph::Connection connection_;
        ModulationProcessor* parent_processor;
        ModulatorBase* processor;
        int modulation_output_bus_index;
        ParameterChangeBuffer* changeBuffer = nullptr;

    private:
        juce::ValueTree change;
    };

    class StateConnectionBank {
    public:
        StateConnectionBank();
        ~StateConnectionBank();
        StateConnection* createConnection(const std::string& from, const std::string& to);

        StateConnection* atIndex(int index) { return all_connections_[index].get(); }
        size_t numConnections() { return all_connections_.size(); }
        void addParam(std::pair<std::string,ParameterChangeBuffer*>&&);
        void reset() {
            for ( auto& c : all_connections_ ) {
                c->reset();
            }
        }

    private:
        std::map<std::string,ParameterChangeBuffer*> parameter_map;
        std::vector<std::unique_ptr<StateConnection>> all_connections_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateConnectionBank)
    };


    class ParamOffsetBank {
    public:
        // ParamOffsetBank();
        // ~ParamOffsetBank();
        // MESSAGE THREAD: register (or find) an index for this param key
        int getOrAddIndex (const std::string& key)
        {
            JUCE_ASSERT_MESSAGE_THREAD

            if (auto it = index_bank.find (key); it != index_bank.end())
                return it->second;

            const int newIndex = (int) audio_thread_offset_bank.size();

            // grow only on message thread
            audio_thread_offset_bank.resize ((size_t) newIndex + 1);

            // init atomics explicitly
            audio_thread_offset_bank[(size_t) newIndex] = 0.f; //.store (0.0f, std::memory_order_relaxed);

            index_bank.emplace (key, newIndex);
            return newIndex;
        }
        // MESSAGE THREAD: register a param and return its index
        int addParam (std::pair<std::string, float>&& p)
        {
            JUCE_ASSERT_MESSAGE_THREAD;

            const auto& key = p.first;

            // already exists
            if (auto it = index_bank.find (key); it != index_bank.end())
                return it->second;

            const int newIndex = (int) audio_thread_offset_bank.size();

            audio_thread_offset_bank.resize ((size_t) newIndex + 1);
            audio_thread_offset_bank[(size_t) newIndex] = p.second;//.store (p.second, std::memory_order_relaxed);

            index_bank.emplace (key, newIndex);
            return newIndex;
        }
        // MESSAGE THREAD: query only (no insert)
        int getIndexIfExists (const std::string& key) const
        {
            JUCE_ASSERT_MESSAGE_THREAD

            if (auto it = index_bank.find (key); it != index_bank.end())
                return it->second;

            return -1;
        }
        // Write the current offset (overwrites)
        void setOffset (int index, float value) noexcept
        {
            if ( index >=  audio_thread_offset_bank.size())
                return;

            audio_thread_offset_bank[(size_t) index] = value;//.store (value, std::memory_order_relaxed);
        }
        float getOffset (int index) const noexcept
        {
            if ( index >=audio_thread_offset_bank.size())
                return 0.0f;

            return audio_thread_offset_bank[(size_t) index];// = value.load (std::memory_order_relaxed);
        }
        // Convenience: build "<uuid>_<paramName>" and add it.
        // v is expected to have IDs::uuid
        int addParam (const juce::ValueTree& v, const juce::String& paramName, float initialValue = 0.0f)
        {
            JUCE_ASSERT_MESSAGE_THREAD;

            const std::string key =
                v.getProperty (IDs::uuid).toString().toStdString()
                + "_"
                + paramName.toStdString();

            auto val = addParam ({ key, initialValue });
            DBG("adding param " + juce::String(key)  + "with index " + juce::String(val));
            return val;
        }

    private:
        std::map<std::string,int> index_bank;
        std::vector<float> audio_thread_offset_bank;
    };
}

#endif //BITKLAVIER_MODULATIONCONNECTION_H
