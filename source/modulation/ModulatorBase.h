//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_MODULATORBASE_H
#define ELECTROSYNTH_MODULATORBASE_H
// #include "ParameterView/ParametersView.h"
#include <juce_data_structures/juce_data_structures.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <chowdsp_serialization/chowdsp_serialization.h>
#include <functional>
#include <map>
#include <string>
#include <iostream>
#include <any>
#include "bk_XMLSerializer.h"
class SynthSection;
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
        virtual void resetTriggered() = 0;

    };
    virtual void triggerModulation()
    {
        for (auto list: listeners_)
        {
            list->modulationTriggered();
        }
    }

    virtual void triggerReset()
    {
        for (auto list: listeners_)
        {
            list->resetTriggered();
        }
    }

    std::vector<Listener*> listeners_;
    void addListener(ModulatorBase::Listener* listener) {
        listeners_.push_back(listener);
    }
    juce::String name;
    virtual void process() =0;
    virtual void getNextAudioBlock (juce::AudioBuffer<float>& bufferToFill, juce::MidiBuffer& midiMessages)  {}
    virtual void prepareToPlay (double sampleRate, int samplesPerBlock)  {}
    virtual void releaseResources() {}
    virtual SynthSection* createEditor() = 0;
    bitklavier::ModulationProcessor* parent_;
    static constexpr ModulatorType type = ModulatorType::AUDIO;
//    std::vector<bitklavier::ModulationConnection*> connections;
    bool trigger = false;
    // /** Calls an action on the main thread via chowdsp::DeferredAction */
    // template <typename Callable>
    // virtual void callOnMainThread (Callable&& func, bool couldBeAudioThread = false) = 0;
    std::vector<juce::ValueTree> connections_;
    virtual void getStateInformation (juce::MemoryBlock &destData)=0;
    virtual void setStateInformation (const void *data, int sizeInBytes)=0;
    bool isDefaultBipolar = false;
};


template <typename PluginStateType>
class ModulatorStateBase : public ModulatorBase
{
public :

    ModulatorStateBase( juce::ValueTree& tree, juce::UndoManager* um = nullptr) : ModulatorBase( tree, um)
    {
      if(tree.isValid())
        chowdsp::Serialization::deserialize<bitklavier::XMLSerializer>(tree.createXml(),_state);
    }

    void getStateInformation(juce::MemoryBlock &destData) override
    {
        _state.serialize(destData);
    }

    void setStateInformation (const void *data, int sizeInBytes) override
    {
        _state.deserialize (juce::MemoryBlock { data, (size_t) sizeInBytes });
    }

    ~ModulatorStateBase() override {}

    PluginStateType _state;
};

#endif //ELECTROSYNTH_MODULATORBASE_H
