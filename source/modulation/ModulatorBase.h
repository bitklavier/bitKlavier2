//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_MODULATORBASE_H
#define ELECTROSYNTH_MODULATORBASE_H
#include "ParameterView/ParametersView.h"
#include <functional>
#include <map>
#include <string>
#include <iostream>
#include <any>
template <class Base>
class SimpleFactory {
public:
    using CreateFunction = std::function<Base*(std::any)>;

    template <typename T, typename... Args>
    void registerType(const std::string& typeName) {
        creators[typeName] = [](std::any args) -> Base* {
            try {
                auto tupleArgs = std::any_cast<std::tuple<Args...>>(args); // Unpack std::any into tuple
                return std::apply([](auto&&... unpackedArgs) {
                    return new T(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);  // Create shared_ptr with forwarded arguments
                }, tupleArgs);  // Apply the arguments
            } catch (const std::bad_any_cast& e) {
                std::cerr << "std::bad_any_cast: " << e.what() << " (expected tuple)" << std::endl;
                return nullptr;
            }
        };
    }

    // Create object with arguments wrapped in std::any
    Base* create(const std::string& typeName, std::any args) const {
        auto it = creators.find(typeName);
        if (it != creators.end()) {
            return it->second(args);  // Call the creation function with arguments
        }
        return nullptr;  // Type not found
    }

private:
    std::map<std::string, CreateFunction> creators;
};
namespace bitklavier{

    class ModulationProcessor;
}
enum class ModulatorType{
    NONE,
    STATE,
    AUDIO
} ;
class ModulatorBase
{
public:
    explicit ModulatorBase(juce::ValueTree& tree, juce::UndoManager* um = nullptr) :
        state(tree),parent_(nullptr)
    {

    }
    //https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors
    virtual ~ModulatorBase(){}
    juce::ValueTree state;
    struct Listener {
        virtual void modulationTriggered() = 0;
    };
    virtual void triggerModulation()
    {
        for (auto list: listeners_)
        {
            list->modulationTriggered();
        }
    }
    std::vector<Listener*> listeners_;
    void addListener(ModulatorBase::Listener* listener) {
        listeners_.push_back(listener);
    }
    juce::String name;
    virtual void process() =0;
    virtual void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages)  {}
    virtual void prepareToPlay (int samplesPerBlock, double sampleRate )  {}
    virtual void releaseResources() {}
    virtual SynthSection* createEditor() = 0;
    bitklavier::ModulationProcessor* parent_;
    static constexpr ModulatorType type = ModulatorType::NONE;
//    std::vector<bitklavier::ModulationConnection*> connections;
    bool trigger = false;

};


template <typename PluginStateType>
class ModulatorStateBase : public ModulatorBase{
public :

    ModulatorStateBase( juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ModulatorBase( tree, um)

    {

    }
    ~ModulatorStateBase() override{}
    PluginStateType _state;
};

#endif //ELECTROSYNTH_MODULATORBASE_H
