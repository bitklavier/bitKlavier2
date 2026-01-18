//
// Created by Davis Polito on 12/18/24.
//
#include "common.h"
#include "ModulationConnection.h"
#include <regex>
#include "ModulatorBase.h"
#include "ModulationProcessor.h"

namespace bitklavier {
    //namespace
    //{
    const std::string kModulationSourceDelimiter = "_";
    const std::set<std::string> kBipolarModulationSourcePrefixes = {
        "lfo",
        "random",
        "pitch",
        "audio"
    };

    force_inline bool isConnectionAvailable(ModulationConnection *connection) {
        return connection->source_name.empty() && connection->destination_name.empty();
    }

    //}

    //    ModulationConnection::ModulationConnection(int index, std::string from, std::string to) :
    //                                                                                               source_name(std::move(from)), destination_name(std::move(to)) {
    //        modulation_processor = std::make_unique<ModulationConnectionProcessor>(index);
    //    }

    //    ModulationConnection::~ModulationConnection() { }
    bool ModulationConnection::isModulationSourceDefaultBipolar(const std::string &source) {
        //std::size_t pos = source.find(kModulationSourceDelimiter);

        std::regex pattern(R"(_(.*?)_)"); // Matches text between underscores

        std::smatch match;
        std::regex_search(source, match, pattern);
        //        if (std::regex_search(text, match, pattern)) {
        //            std::cout << "Found: " << match[1] << std::endl;
        //        } else {
        //            std::cout << "No match found." << std::endl;
        //        }
        std::string check = match[1];
        std::size_t pos = check.find_first_of("-");
        std::string prefix = check.substr(0, pos);
        return kBipolarModulationSourcePrefixes.count(prefix) > 0;
    }

    ModulationConnectionBank::ModulationConnectionBank() {
        for (int i = 0; i < kMaxModulationConnections; ++i) {
            std::unique_ptr<ModulationConnection> connection = std::make_unique<ModulationConnection>("", "", i);
            all_connections_.push_back(std::move(connection));
        }
    }

    ModulationConnectionBank::~ModulationConnectionBank() {
    }

    ModulationConnection *ModulationConnectionBank::createConnection(const std::string &from, const std::string &to) {
        int index = 1;
        for (auto &connection: all_connections_) {
            std::string invalid_connection = "modulation_" + std::to_string(index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable(connection.get())) {
                connection->resetConnection(from, to);
                connection->setDefaultBipolar(ModulationConnection::isModulationSourceDefaultBipolar(from));
                return connection.get();
            }
        }

        return nullptr;
    }

    force_inline bool isConnectionAvailable(StateConnection *connection) {
        return connection->source_name.empty() && connection->destination_name.empty();
    }

    StateConnectionBank::StateConnectionBank() {
        for (int i = 0; i < kMaxStateConnections; ++i) {
            std::unique_ptr<StateConnection> connection = std::make_unique<StateConnection>("", "", i);
            all_connections_.push_back(std::move(connection));
        }
    }

    StateConnectionBank::~StateConnectionBank() {
    }

    StateConnection *StateConnectionBank::createConnection(const std::string &from, const std::string &to) {
        int index = 1;
        for (auto &connection: all_connections_) {
            std::string invalid_connection = "State_" + std::to_string(index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable(connection.get())) {
                connection->resetConnection(from, to);
                connection->setChangeBuffer(parameter_map.find(to)->second);
                return connection.get();
            }
        }

        return nullptr;
    }

    void StateConnectionBank::addParam(std::pair<std::string, bitklavier::ParameterChangeBuffer *> &&pair) {
        parameter_map.insert(pair);
    }

    void ModulationConnection::updateScalingAudioThread(float currentTotalParamUnits, float raw0) noexcept {

        DBG("ModulationConnection::updateScalingAudioThread, raw0 = " + std::to_string(raw0));
        // if (raw0 > 0.) // if this is true, we are either in the midst of ramping or have finished ramping, so we are triggering this mod consecutively
        // {
        //     /*
        //      * doing this at least for now because:
        //      * - triggering a mod twice in a row causes the resets to reset to that mod target value, rather than the expected stored knob value
        //      * - with this conditional, you can trigger multiple different mods to a single param as much as you like
        //      *      - and reset them with either:
        //      *          - the MIDITarget Reset message (which will reset the whole prep to its default values, when i finish implementation for state mods)
        //      *              - Note that if any of these mods are attached to other preps, those other preps will also be affected!
        //      *                  - i'm ok with this
        //      *          - or with the Reset prep attached to an individual Mod prep, which will only reset that Mod
        //      *
        //      * one consequence at the moment is that retriggering is not continuous; param will go immediately back to its start value and then ramp again.
        //      * - will look into addressing that later, but at the moment aiming for basic MVP behavior
        //      *      - i actually think this is a potentially interesting behavior, so might be ok to leave, or make an option
        //      * It also doesn't seem to play well with an overlayed LFO, but it wasn't before this change either
        //      *      - another thing to look at later
        //      *
        //      * Eventually, we track down why the default value is getting overwritten by the mod target whn triggering it consecutively
        //      * - probably simple, but we need to streamline all of this and make it more readable; for now I want to get the basic behavior in place
        //      * - or maybe it's just that one of the mods is still sending a nonzero value on the audio bus
        //      *
        //      * Well, still doesn't work consistently
        //      */
        //     DBG("ModulationConnection::updateScalingAudioThread, raw0 > 0, scalingLocked_ = true");
        //     scalingLocked_.store(true, std::memory_order_release);
        // }

        if (scalingLocked_.load(std::memory_order_acquire))
            return;

        DBG("ModulationConnection::updateScalingAudioThread, scalingLocked_ = false");
        const float start = rangeStart_.load(std::memory_order_relaxed);
        const float end = rangeEnd_.load(std::memory_order_relaxed);

        const float curClamped = juce::jlimit(start, end, currentTotalParamUnits);
        const float curNormAbs = range.convertTo0to1(curClamped);

        const float scaleNorm = scalingValue_.load(std::memory_order_relaxed);

        float selfNorm = 0.0f;
        const float carryNorm = carryApplied_.load(std::memory_order_relaxed);
        selfNorm = carryNorm + raw0 * scaleNorm;

        const float anchorNormAbs = juce::jlimit(0.0f, 1.0f, abs(curNormAbs - selfNorm));
        const float anchorParam = range.convertFrom0to1(anchorNormAbs);

        const float modVal = modAmt_.load(std::memory_order_relaxed);

        const float base = juce::jlimit(start, end, anchorParam);
        const float baseNormAbs = range.convertTo0to1(base);

        float newScaleNorm = 0.0f;

        if (isOffset()) {
            const float targetNormAbs = range.convertTo0to1(juce::jmin(base + modVal, end));
            newScaleNorm = juce::jmax(0.0f, targetNormAbs - baseNormAbs);
        } else {
            // "target value" semantics fallback (unchanged)
            const float target = juce::jlimit(start, end, modVal);
            const float targetNormAbs = range.convertTo0to1(target);
            newScaleNorm = targetNormAbs - curNormAbs;
        }
        scalingValue_.store(newScaleNorm, std::memory_order_relaxed);
        carryApplied_.store(selfNorm, std::memory_order_relaxed);
        carryActive_.store(true, std::memory_order_release);
    }
}
