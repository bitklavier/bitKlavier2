//
// Created by Davis Polito on 12/18/24.
//
#include "common.h"
#include "ModulationConnection.h"
#include <regex>
#include "ModulatorBase.h"
#include "ModulationProcessor.h"
namespace bitklavier
{
    //namespace
    //{
    const std::string kModulationSourceDelimiter = "_";
    const std::set<std::string> kBipolarModulationSourcePrefixes = {
        "lfo",
        "random",
        "pitch",
        "audio"
    };

    force_inline bool isConnectionAvailable (ModulationConnection* connection)
    {
        return connection->source_name.empty() && connection->destination_name.empty();
    }
    //}

    //    ModulationConnection::ModulationConnection(int index, std::string from, std::string to) :
    //                                                                                               source_name(std::move(from)), destination_name(std::move(to)) {
    //        modulation_processor = std::make_unique<ModulationConnectionProcessor>(index);
    //    }

    //    ModulationConnection::~ModulationConnection() { }
    bool ModulationConnection::isModulationSourceDefaultBipolar(const std::string& source) {
        //std::size_t pos = source.find(kModulationSourceDelimiter);

        std::regex pattern(R"(_(.*?)_)");  // Matches text between underscores

        std::smatch match;
        std::regex_search(source,match, pattern);
//        if (std::regex_search(text, match, pattern)) {
//            std::cout << "Found: " << match[1] << std::endl;
//        } else {
//            std::cout << "No match found." << std::endl;
//        }
        std::string check = match[1];
        std::size_t pos = check.find_first_of("-");
        std::string prefix = check.substr( 0,pos);
        return kBipolarModulationSourcePrefixes.count(prefix) > 0;
    }

    ModulationConnectionBank::ModulationConnectionBank ()
    {
        for (int i = 0; i < kMaxModulationConnections; ++i)
        {
            std::unique_ptr<ModulationConnection> connection = std::make_unique<ModulationConnection> ("", "", i);
            all_connections_.push_back (std::move (connection));
        }
    }

    ModulationConnectionBank::~ModulationConnectionBank() {}

    ModulationConnection* ModulationConnectionBank::createConnection (const std::string& from, const std::string& to)
    {
        int index = 1;
        for (auto& connection : all_connections_)
        {
            std::string invalid_connection = "modulation_" + std::to_string (index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable (connection.get()))
            {
                connection->resetConnection (from, to);
                connection->setDefaultBipolar(ModulationConnection::isModulationSourceDefaultBipolar(from));
                return connection.get();
            }
        }

        return nullptr;
    }

    force_inline bool isConnectionAvailable (StateConnection* connection)
    {
        return connection->source_name.empty() && connection->destination_name.empty();
    }

     StateConnectionBank::StateConnectionBank ()
    {
        for (int i = 0; i < kMaxStateConnections; ++i)
        {
            std::unique_ptr<StateConnection> connection = std::make_unique<StateConnection> ("", "", i);
            all_connections_.push_back (std::move (connection));
        }
    }

    StateConnectionBank::~StateConnectionBank() {}

    StateConnection* StateConnectionBank::createConnection (const std::string& from, const std::string& to)
    {
        int index = 1;
        for (auto& connection : all_connections_)
        {
            std::string invalid_connection = "State_" + std::to_string (index++) + "_amount";
            if (to != invalid_connection && isConnectionAvailable (connection.get()))
            {
                connection->resetConnection (from, to);
                connection->setChangeBuffer(parameter_map.find(to)->second);
                return connection.get();
            }
        }

        return nullptr;
    }

    void StateConnectionBank::addParam(std::pair<std::string, bitklavier::ParameterChangeBuffer *> && pair) {
        parameter_map.insert(pair);
    }

    // ParamOffsetBank::ParamOffsetBank() {
    //     parameter_map.r
    // }


    void ModulationConnection::updateScalingAudioThread (float currentTotalParamUnits, float raw0) noexcept
{
    if (scalingLocked_.load (std::memory_order_acquire))
        return;

    const float start = rangeStart_.load (std::memory_order_relaxed);
    const float end   = rangeEnd_.load   (std::memory_order_relaxed);

    // 1) Current value WITH ALL MODS -> normalized absolute position [0..1]
    const float curClamped = juce::jlimit (start, end, currentTotalParamUnits);
    const float curNormAbs = range.convertTo0to1 (curClamped);

    // 2) This connection’s current contribution in normalized-delta units
    // (matches your DSP formula: (1-r)*carry + r*scale)
    const float scaleNorm = scalingValue_.load (std::memory_order_relaxed);

    float selfNorm = 0.0f;
    if (carryActive_.load (std::memory_order_acquire))
    {
        const float carryNorm = carryApplied_.load (std::memory_order_relaxed);
        selfNorm = (1.0f - raw0) * carryNorm + raw0 * scaleNorm;
    }
    else
    {
        selfNorm = raw0 * scaleNorm;
    }

    // 3) Anchor = everything except THIS connection, in normalized absolute units
    const float anchorNormAbs = juce::jlimit (0.0f, 1.0f, curNormAbs - selfNorm);
    const float anchorParam   = range.convertFrom0to1 (anchorNormAbs);

    // 4) Recompute scaling from ANCHOR (your existing semantics)
    const float modVal = modAmt_.load (std::memory_order_relaxed);

    const float base = juce::jlimit (start, end, anchorParam);
    const float baseNormAbs = range.convertTo0to1 (base);

    float newScaleNorm = 0.0f;

    // (you said ignore bipolar issues; keep your branches as-is)
    if (isOffset())
    {
        const float targetNormAbs = range.convertTo0to1 (juce::jmin (base + modVal, end));
        newScaleNorm = juce::jmax (0.0f, targetNormAbs - baseNormAbs);
    }
    else
    {
        // "target value" semantics fallback (unchanged)
        const float target = juce::jlimit (start, end, modVal);
        const float targetNormAbs = range.convertTo0to1 (target);
        newScaleNorm = targetNormAbs - baseNormAbs;
    }

    scalingValue_.store (newScaleNorm, std::memory_order_relaxed);

    // 5) Update carryApplied_ so the output is continuous at retrigger.
    // We want: selfNorm (at this raw0) to stay the same after scaling changes.
    //
    // selfNorm = (1-r)*carry + r*scale  => carry = (selfNorm - r*scale) / (1-r)
    //
    // Only meaningful when r != 1 (at end of ramp carry is irrelevant).
    constexpr float eps = 1.0e-5f;
    const float denom = juce::jmax (eps, 1.0f - raw0);
    const float newCarryNorm = (selfNorm - raw0 * newScaleNorm) / denom;

    carryApplied_.store (newCarryNorm, std::memory_order_relaxed);
    carryActive_.store  (true,         std::memory_order_release);
}

}
