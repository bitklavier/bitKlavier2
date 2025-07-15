//
// Created by Davis Polito on 6/11/25.
//

#include "PreparationList.h"
#include "DirectProcessor.h"
#include "KeymapProcessor.h"
#include "ModulationProcessor.h"
#include "ResetProcessor.h"
#include "UserPreferences.h"

PreparationList::PreparationList(SynthBase &parent, const juce::ValueTree &v) : tracktion::engine::ValueTreeObjectList<
    PluginInstanceWrapper>(v), synth(parent) {
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeDirect, DirectProcessor::create);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeKeymap, KeymapProcessor::create);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeModulation,
                         bitklavier::ModulationProcessor::create);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeTuning, TuningProcessor::create);
    prepFactory.Register(bitklavier::BKPreparationType::PreparationTypeReset, bitklavier::ResetProcessor::create);
}

PluginInstanceWrapper *PreparationList::createNewObject(const juce::ValueTree &v) {
    juce::AudioProcessor *rawPtr;
    juce::AudioProcessorGraph::Node::Ptr node_ptr;
    juce::ValueTree state = v;
    if (temporary_instance == nullptr && static_cast<int>(state.getProperty(IDs::type)) <
        bitklavier::BKPreparationType::PreparationTypeVST) {
        auto processor = prepFactory.CreateObject((int) state.getProperty(IDs::type), synth, v);
        rawPtr = processor.get();
        processor->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());
        //looking at ProcessorGraph i actually don't think their is any need to try to wrap this in thread safety because
        //the graph rebuild is inherently gonna trigger some async blocking

        node_ptr = synth.addProcessor(std::move(processor),
                                      juce::AudioProcessorGraph::NodeID(
                                          juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow()));
    } else if (temporary_instance == nullptr && static_cast<int>(state.getProperty(IDs::type)) ==
               bitklavier::BKPreparationType::PreparationTypeVST) {
        juce::String err;

        juce::PluginDescription pd;
        pd.loadFromXml(*v.getChildWithName(IDs::PLUGIN).createXml());
        temporary_instance = synth.user_prefs->userPreferences->formatManager.createPluginInstance(
            pd,
            synth.getSampleRate(),
            synth.getBufferSize(),
            err);
        if (temporary_instance == nullptr) {
            auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                                  TRANS("Couldn't create plugin"),
                                                                  err);
            //TODO show error
        }
        if (v.getChildWithName("STATE").isValid()) {
            juce::MemoryBlock m;
            m.fromBase64Encoding(v.getChildWithName("STATE").getProperty("base64").toString());
            temporary_instance->setStateInformation(m.getData(), (int) m.getSize());
        }
        // createUuidProperty(state);
        rawPtr = temporary_instance.get();
        temporary_instance->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());
        node_ptr = synth.addProcessor(std::move(temporary_instance),
                                      juce::AudioProcessorGraph::NodeID(
                                          juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow()));
    } else {
        createUuidProperty(state);
        rawPtr = temporary_instance.get();
        temporary_instance->prepareToPlay(synth.getSampleRate(), synth.getBufferSize());
        node_ptr = synth.addProcessor(std::move(temporary_instance),
                                      juce::AudioProcessorGraph::NodeID(
                                          juce::Uuid(state.getProperty(IDs::uuid).toString()).getTimeLow()));
    }
    if (node_ptr) {
        node_ptr->properties.set(
            "x", juce::VariantConverter<juce::Point<int> >::fromVar(v.getProperty(IDs::x_y)).getX());
        node_ptr->properties.set(
            "y", juce::VariantConverter<juce::Point<int> >::fromVar(v.getProperty(IDs::x_y)).getY());
        node_ptr->properties.set("type", v.getProperty(IDs::type));
        //sendChangeMessage();
    }
    return new PluginInstanceWrapper(rawPtr, state, node_ptr->nodeID);
}

void PreparationList::deleteObject(PluginInstanceWrapper *at) {
    for (auto listener: listeners_)
        listener->removeModule(at);
    synth.removeProcessor(at->node_id);
    delete at;
}

void PreparationList::valueTreeRedirected(juce::ValueTree &) {
    deleteAllObjects();
    rebuildObjects();
    for (auto object: objects) {
        newObjectAdded(object);
    }
}

void PreparationList::newObjectAdded(PluginInstanceWrapper *instance_wrapper) {
    for (auto listener: listeners_)
        listener->moduleAdded(instance_wrapper);
}

void PreparationList::deleteAllGui() {
    for (auto obj: objects)
        for (auto listener: listeners_)
            listener->removeModule(obj);

}
void PreparationList::rebuildAllGui() {
    for (auto obj: objects)
        for (auto listener: listeners_)
            listener->moduleAdded(obj);
}


void PreparationList::valueTreeParentChanged(juce::ValueTree &) {
}

void PreparationList::addPlugin(const juce::PluginDescription &desc, const juce::ValueTree &v) {
    if (desc.pluginFormatName != "AudioUnit") {
        juce::String err;
        temporary_instance = synth.user_prefs->userPreferences->formatManager.createPluginInstance(desc,
            synth.getSampleRate(),
            synth.getBufferSize(),
            err);
        if (temporary_instance == nullptr) {
            auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                                  TRANS("Couldn't create plugin"),
                                                                  err);
            //TODO show error
            //return nullptr;
            return;
        }
        parent.addChild(v, -1, nullptr);
    } else {
        synth.user_prefs->userPreferences->formatManager.createPluginInstanceAsync(desc,
            synth.getSampleRate(),
            synth.getBufferSize(),
            [this, v](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String &error) {
                addPluginCallback(std::move(instance), error, v);
            });
    }
    //temporary_instance = nullptr;
}

void PreparationList::addPluginCallback(std::unique_ptr<juce::AudioPluginInstance> instance,
                                        const juce::String &error,
                                        const juce::ValueTree &v) {
    if (instance == nullptr) {
        auto options = juce::MessageBoxOptions::makeOptionsOk(juce::MessageBoxIconType::WarningIcon,
                                                              TRANS("Couldn't create plugin"),
                                                              error);
        //messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
    } else {
        instance->enableAllBuses();
        temporary_instance = std::move(instance);
        parent.addChild(v, -1, nullptr);
    }
}

void PreparationList::setValueTree(const juce::ValueTree &v) {
    parent = v;
}
