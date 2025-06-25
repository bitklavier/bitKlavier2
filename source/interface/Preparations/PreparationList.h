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
#include "Identifiers.h"
#include "synth_base.h"
#include "templates/Factory.h"
class PluginInstanceWrapper  {
    public:
    PluginInstanceWrapper (juce::AudioProcessor* proc,const juce::ValueTree& v, juce::AudioProcessorGraph::NodeID nodeID) : state(v), proc(proc),node_id(nodeID) {
        // if (!v.getProperty(IDs::nodeID))
            state.setProperty(IDs::nodeID, juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(nodeID),nullptr);
    }
    juce::AudioProcessor* proc;
    juce::AudioProcessorGraph::NodeID node_id;
    juce::ValueTree state;

};
typedef Loki::Factory<std::unique_ptr<juce::AudioProcessor>, int,SynthBase& ,const juce::ValueTree&  > PreparationFactory;
class PreparationList : public tracktion::engine::ValueTreeObjectList<PluginInstanceWrapper>, public juce::ChangeBroadcaster{
public:
    PreparationList(SynthBase& parent, const juce::ValueTree & v);
    ~PreparationList() {
        freeObjects();
    }

    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType (IDs::PREPARATION) ;
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

    void newObjectAdded (PluginInstanceWrapper*) override;
    void objectRemoved (PluginInstanceWrapper*) override;     //resized(); }
    void objectOrderChanged() override              { }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override;
    void valueTreeRedirected (juce::ValueTree&) override ;
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override
    {
        tracktion::engine::ValueTreeObjectList<PluginInstanceWrapper>::valueTreePropertyChanged (v, i);
        if(v.getProperty("sync",0))
        {
            for(auto obj : objects)
            {
                juce::MemoryBlock data;
                obj->proc->getStateInformation(data);
                auto xml = juce::parseXML(data.toString());
                //auto xml = juce::AudioProcessor::getXmlF(data.getData(), (int)data.getSize());
                if (obj->state.getChild(0).isValid() && xml != nullptr)
                    obj->state.getChild(0).copyPropertiesFrom(juce::ValueTree::fromXml(*xml),nullptr);
                //  state.addChild(juce::ValueTree::fromXml(*xml),0,nullptr);
            }
            v.removeProperty("sync", nullptr);
        }
    }
    void addPlugin (const juce::PluginDescription& desc, const juce::ValueTree& v);

    void addPluginCallback (std::unique_ptr<juce::AudioPluginInstance> instance,
                                     const juce::String& error,
                                     const juce::ValueTree &v);
    void setValueTree(const juce::ValueTree& v);
private:
    std::unique_ptr<juce::AudioPluginInstance> temporary_instance;
    PreparationFactory prepFactory;
    std::vector<Listener*> listeners_;
    SynthBase& synth;
};



#endif //PREPARATIONLIST_H
