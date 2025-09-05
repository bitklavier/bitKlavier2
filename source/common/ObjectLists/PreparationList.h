//
// Created by Davis Polito on 6/11/25.
//

#ifndef PREPARATIONLIST_H
#define PREPARATIONLIST_H
#include <juce_audio_processors/juce_audio_processors.h>
//==============================================================================
class InternalPlugin final : public juce::AudioPluginInstance
{
public:
    explicit InternalPlugin (std::unique_ptr<AudioProcessor> innerIn)
        : inner (std::move (innerIn))
    {
        jassert (inner != nullptr);

        for (auto isInput : { true, false })
            matchChannels (isInput);

        setBusesLayout (inner->getBusesLayout());
    }

    //==============================================================================
    const juce::String getName() const override                                         { return inner->getName(); }
    juce::StringArray getAlternateDisplayNames() const override                         { return inner->getAlternateDisplayNames(); }
    double getTailLengthSeconds() const override                                  { return inner->getTailLengthSeconds(); }
    bool acceptsMidi() const override                                             { return inner->acceptsMidi(); }
    bool producesMidi() const override                                            { return inner->producesMidi(); }
    juce::AudioProcessorEditor* createEditor() override                                 { return inner->createEditor(); }
    bool hasEditor() const override                                               { return inner->hasEditor(); }
    int getNumPrograms() override                                                 { return inner->getNumPrograms(); }
    int getCurrentProgram() override                                              { return inner->getCurrentProgram(); }
    void setCurrentProgram (int i) override                                       { inner->setCurrentProgram (i); }
    const juce::String getProgramName (int i) override                                  { return inner->getProgramName (i); }
    void changeProgramName (int i, const juce::String& n) override                      { inner->changeProgramName (i, n); }
    void getStateInformation (juce::MemoryBlock& b) override                      { inner->getStateInformation (b); }
    void setStateInformation (const void* d, int s) override                      { inner->setStateInformation (d, s); }
    void getCurrentProgramStateInformation (juce::MemoryBlock& b) override        { inner->getCurrentProgramStateInformation (b); }
    void setCurrentProgramStateInformation (const void* d, int s) override        { inner->setCurrentProgramStateInformation (d, s); }

    void prepareToPlay (double sr, int bs) override
    {
        inner->setProcessingPrecision (getProcessingPrecision());
        inner->setRateAndBufferSizeDetails (sr, bs);
        inner->prepareToPlay (sr, bs);
    }

    void releaseResources() override                                              { inner->releaseResources(); }
    void memoryWarningReceived() override                                         { inner->memoryWarningReceived(); }
    void processBlock (juce::AudioBuffer<float>& a, juce::MidiBuffer& m) override             { inner->processBlock (a, m); }
    void processBlock (juce::AudioBuffer<double>& a, juce::MidiBuffer& m) override            { inner->processBlock (a, m); }
    void processBlockBypassed (juce::AudioBuffer<float>& a, juce::MidiBuffer& m) override     { inner->processBlockBypassed (a, m); }
    void processBlockBypassed (juce::AudioBuffer<double>& a, juce::MidiBuffer& m) override    { inner->processBlockBypassed (a, m); }
    bool supportsDoublePrecisionProcessing() const override                       { return inner->supportsDoublePrecisionProcessing(); }
    bool supportsMPE() const override                                             { return inner->supportsMPE(); }
    bool isMidiEffect() const override                                            { return inner->isMidiEffect(); }
    void reset() override                                                         { inner->reset(); }
    void setNonRealtime (bool b) noexcept override                                { inner->setNonRealtime (b); }
    void refreshParameterList() override                                          { inner->refreshParameterList(); }
    void numChannelsChanged() override                                            { inner->numChannelsChanged(); }
    void numBusesChanged() override                                               { inner->numBusesChanged(); }
    void processorLayoutsChanged() override                                       { inner->processorLayoutsChanged(); }
    void setPlayHead (juce::AudioPlayHead* p) override                                  { inner->setPlayHead (p); }
    void updateTrackProperties (const TrackProperties& p) override                { inner->updateTrackProperties (p); }
    bool isBusesLayoutSupported (const BusesLayout& layout) const override        { return inner->checkBusesLayoutSupported (layout); }
    bool applyBusLayouts (const BusesLayout& layouts) override                    { return inner->setBusesLayout (layouts) && AudioPluginInstance::applyBusLayouts (layouts); }

    bool canAddBus (bool) const override                                          { return true; }
    bool canRemoveBus (bool) const override                                       { return true; }

    //==============================================================================
    void fillInPluginDescription (juce::PluginDescription& description) const override
    {
        description = getPluginDescription (*inner);
    }
    std::unique_ptr<AudioProcessor> inner;
private:
    static juce::PluginDescription getPluginDescription (const AudioProcessor& proc)
    {
        const auto ins                  = proc.getTotalNumInputChannels();
        const auto outs                 = proc.getTotalNumOutputChannels();
        const auto identifier           = proc.getName();
        const auto registerAsGenerator  = ins == 0;
        const auto acceptsMidi          = proc.acceptsMidi();

        juce::PluginDescription descr;

        descr.name              = identifier;
        descr.descriptiveName   = identifier;
        descr.pluginFormatName  =  "bitklavier";//juce::InternalPluginFormat::getIdentifier();
        descr.category          = (registerAsGenerator ? (acceptsMidi ? "Synth" : "Generator") : "Effect");
        descr.manufacturerName  = "bitklavier";
        descr.version           = "1";//juce::ProjectInfo::versionString;
        descr.fileOrIdentifier  = identifier;
        descr.isInstrument      = (acceptsMidi && registerAsGenerator);
        descr.numInputChannels  = ins;
        descr.numOutputChannels = outs;

        descr.uniqueId = descr.deprecatedUid = identifier.hashCode();

        return descr;
    }

    void matchChannels (bool isInput)
    {
        const auto inBuses = inner->getBusCount (isInput);

        while (getBusCount (isInput) < inBuses)
            addBus (isInput);

        while (inBuses < getBusCount (isInput))
            removeBus (isInput);
    }



    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalPlugin)
};
#include <tracktion_ValueTreeUtilities.h>
#include "../Identifiers.h"
#include "synth_base.h"
#include "../templates/Factory.h"
class KeymapProcessor;
class PluginInstanceWrapper  {
    public:
    PluginInstanceWrapper (juce::AudioProcessor* proc,const juce::ValueTree& v, juce::AudioProcessorGraph::NodeID nodeID) : state(v), proc(proc),node_id(nodeID) {
        // if (!v.getProperty(IDs::nodeID))
            state.setProperty(IDs::nodeID, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(nodeID),nullptr);
    }
    juce::AudioProcessor* proc;
    juce::AudioProcessorGraph::NodeID node_id;
    juce::ValueTree state;
    void returnModulatedParamsToDefault() {
        for (auto param_default: state) {
            if (param_default.hasType(IDs::PARAM_DEFAULT)) {
                // Iterate over properties
                for (int i = 0; i < param_default.getNumProperties(); ++i)
                {
                    juce::Identifier propName = param_default.getPropertyName(i);
                    juce::var value = param_default.getProperty(propName);

                    state.setProperty(propName,value,nullptr);

                }
            }
        }
    }
    // KeymapProcessor* keymap_processor;

};
template <class Base>
class Factory {
public:
    using CreateFunction = std::function<std::unique_ptr<Base>(std::any)>;

    template <typename T, typename... Args>
    void registerType(const std::string& typeName) {
        creators[typeName] = [](std::any args) -> std::unique_ptr<Base> {
            try {
                auto tupleArgs = std::any_cast<std::tuple<Args...>>(args); // Unpack std::any into tuple
                return std::apply([](auto&&... unpackedArgs) {
                    return std::make_unique<T>(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);  // Create shared_ptr with forwarded arguments
                }, tupleArgs);  // Apply the arguments
            } catch (const std::bad_any_cast& e) {
                std::cerr << "std::bad_any_cast: " << e.what() << " (expected tuple)" << std::endl;
                return nullptr;
            }
        };
    }

    // Create object with arguments wrapped in std::any
    std::unique_ptr<Base> create(const std::string& typeName, std::any args) const {
        auto it = creators.find(typeName);
        if (it != creators.end()) {
            return std::move(it->second(args));  // Call the creation function with arguments
        }
        return nullptr;  // Type not found
    }
    bool contains(std::string str) const {
       return creators.find(str) != creators.end();
    }
private:
    std::map<std::string, CreateFunction> creators;
};
typedef Factory<juce::AudioProcessor> PreparationFactory; //, int,SynthBase& ,const juce::ValueTree&  > PreparationFactory;
class PreparationList : public tracktion::engine::ValueTreeObjectList<PluginInstanceWrapper>, public juce::ChangeBroadcaster{
public:
    PreparationList(SynthBase& parent, const juce::ValueTree & v);
    ~PreparationList() {
        deleteAllGui();
        freeObjects();
    }

    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return prepFactory.contains(v.getType().toString().toStdString());
    }
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void moduleListChanged() = 0;
        virtual void moduleAdded(PluginInstanceWrapper* newModule) = 0;
        virtual void removeModule(PluginInstanceWrapper* moduleToRemove) = 0;
    };
    void addListener (Listener* l) { listeners_.push_back (l); }

    void removeListener (Listener* l) {listeners_.erase(
                std::remove(listeners_.begin(), listeners_.end(), l),
                listeners_.end());
    }
    PluginInstanceWrapper* createNewObject(const juce::ValueTree& v) override;
    void deleteObject (PluginInstanceWrapper* at) override;



    void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.appendChild(child,undoManager);

    }

    void removeChild (juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.removeChild(child,undoManager);
    }

    void newObjectAdded (PluginInstanceWrapper*) override;
    void objectRemoved (PluginInstanceWrapper*) override {};     //resized(); }
    void objectOrderChanged() override              { }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override;
    void valueTreeRedirected (juce::ValueTree&) override ;
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override
    {
        tracktion::engine::ValueTreeObjectList<PluginInstanceWrapper>::valueTreePropertyChanged (v, i);
        if(v.getProperty(IDs::sync,0))
        {
            for(auto obj : objects)
            {
                juce::MemoryBlock data;
                obj->proc->getStateInformation(data);
                auto xml = juce::parseXML(data.toString());
                if (xml != nullptr) {
                    //read the current state of the paramholder
                    auto vt = juce::ValueTree::fromXml(*xml);
                    for (int i = 0; i < vt.getNumProperties(); ++i)
                    {
                        juce::Identifier propName = vt.getPropertyName(i);
                        juce::var value = vt.getProperty(propName);

                        obj->state.setProperty(propName, value, nullptr); // Overwrite or add
                    }
                    //update any params that have been modulated to their "default" state
                    //i.e. the state set by moving the knobs in the preparation view
                    //not what they may have been modulated to
                    obj->returnModulatedParamsToDefault();
                }
                else if ( obj->state.getChildWithName(IDs::PLUGIN).isValid() ) {

                    juce::MemoryBlock m;
                    obj->proc->getStateInformation (m);
                    obj->state.getOrCreateChildWithName("STATE",nullptr).setProperty ("base64",m.toBase64Encoding(),nullptr);
                }
                if (obj->state.getChildWithName(IDs::modulationproc).isValid()) {
                    obj->state.setProperty(IDs::sync,1,nullptr);
                    for (auto a : obj->state)
                    {
                    if (a.hasType(IDs::modulationproc))
                        a.setProperty(IDs::sync,1,nullptr);
                    }
                    // obj->state.getChildWithName(IDs::modulationproc).setProperty(IDs::sync,1,nullptr);
                }

            }
            v.removeProperty(IDs::sync, nullptr);
        }
    }
    void addPlugin (const juce::PluginDescription& desc, const juce::ValueTree& v);

    void addPluginCallback (std::unique_ptr<juce::AudioPluginInstance> instance,
                                     const juce::String& error,
                                     const juce::ValueTree &v);
    void setValueTree(const juce::ValueTree& v);
    const juce::ValueTree& getValueTree() const {
        return parent;
    }

    void deleteAllGui();
    void rebuildAllGui();

private:
    void prependAllPianoChangeProcessorsTo(const PluginInstanceWrapper*);
    void prependPianoChangeProcessorToAll(const PluginInstanceWrapper*);
    std::vector<PluginInstanceWrapper*> pianoSwitchProcessors;
    std::unique_ptr<juce::AudioPluginInstance> temporary_instance;
    PreparationFactory prepFactory;
    std::vector<Listener*> listeners_;
    SynthBase& synth;
};



#endif //PREPARATIONLIST_H
