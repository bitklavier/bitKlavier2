#include "PluginEditor.h"
#include "catch2/benchmark/catch_benchmark_all.hpp"
#include "catch2/catch_test_macros.hpp"

TEST_CASE ("Boot performance")
{
    BENCHMARK_ADVANCED ("Processor constructor")
    (Catch::Benchmark::Chronometer meter)
    {
        std::vector<Catch::Benchmark::storage_for<PluginProcessor>> storage (size_t (meter.runs()));
        meter.measure ([&] (int i) { storage[(size_t) i].construct(); });
    };

    BENCHMARK_ADVANCED ("Processor destructor")
    (Catch::Benchmark::Chronometer meter)
    {
        std::vector<Catch::Benchmark::destructable_object<PluginProcessor>> storage (size_t (meter.runs()));
        for (auto& s : storage)
            s.construct();
        meter.measure ([&] (int i) { storage[(size_t) i].destruct(); });
    };

    // BENCHMARK_ADVANCED ("Editor open and close")
    // (Catch::Benchmark::Chronometer meter)
    // {
    //     PluginProcessor plugin;
    //
    //     // due to complex construction logic of the editor, let's measure open/close together
    //     meter.measure ([&] (int /* i */) {
    //         auto editor = plugin.createEditorIfNeeded();
    //         plugin.editorBeingDeleted (editor);
    //         delete editor;
    //         return plugin.getActiveEditor();
    //     });
    // };
}

std::vector<std::string> generateRandomIndexValueStrings(
    size_t runs,
    int maxIndex = 128,
    float minVal = 0.0f,
    float maxVal = 10.0f,
    int minPairs = 40,
    int maxPairs = 80)
{
    std::vector<std::string> result;
    result.reserve(runs);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> lengthDist(minPairs, maxPairs);
    std::uniform_int_distribution<int> indexDist(0, maxIndex);
    std::uniform_real_distribution<float> valueDist(minVal, maxVal);

    for (size_t i = 0; i < runs; ++i)
    {
        int numPairs = lengthDist(gen);
        std::unordered_set<int> usedIndices;

        std::ostringstream oss;
        for (int j = 0; j < numPairs; ++j)
        {
            int index;
            do {
                index = indexDist(gen);
            } while (usedIndices.find(index) != usedIndices.end());
            usedIndices.insert(index);

            float value = valueDist(gen);

            oss << index << ":" << value;
            if (j != numPairs - 1)
                oss << " ";
        }

        result.push_back(oss.str());
    }

    return result;
}

template <std::size_t N>
std::array<float, N> parseIndexValueStringToArray(const std::string& input)
{
    std::array<float, N> result{};
    std::istringstream iss(input);
    std::string token;

    while (iss >> token)
    {
        auto colonPos = token.find(':');
        if (colonPos == std::string::npos) continue;

        int index = std::stoi(token.substr(0, colonPos));
        float value = std::stof(token.substr(colonPos + 1));

        if (index >= 0 && static_cast<std::size_t>(index) < N)
            result[index] = value;
    }

    return result;
}

TEST_CASE ("String read performance") {
    BENCHMARK_ADVANCED ("Editor open and close")
       (Catch::Benchmark::Chronometer meter)
       {
           auto strings   = generateRandomIndexValueStrings(meter.runs());
           // due to complex construction logic of the editor, let's measure open/close together
           meter.measure ([&] (int i) {
               auto arr = parseIndexValueStringToArray<128>(strings[i]);
           });
       };
    BENCHMARK_ADVANCED("Sleep 32 sample block")
    (Catch::Benchmark::Chronometer meter)
    {

        meter.measure([&](int i) {
            std::this_thread::sleep_for(std::chrono::microseconds(724));
        });
    };
}
