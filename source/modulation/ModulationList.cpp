//
// Created by Davis Polito on 2/11/25.
//
#include "ModulationList.h"
#include "ModulatorBase.h"
#include "synth_base.h"
#include "Identifiers.h"
#include "ModulationProcessor.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

ModulationList::ModulationList(const juce::ValueTree &v,SynthBase* p,bitklavier::ModulationProcessor* proc) : tracktion::ValueTreeObjectList<ModulatorBase>(v),
        parent_(p), proc_(proc), um(&parent_->getUndoManager())
{
    rebuildObjects();
    for (auto object : objects)
    {
        newObjectAdded (object);
    }
    isInit = false;
}

ModulationList::~ModulationList()
{
    // Notify listeners (copy to allow self-unregistering safely)
    {
        auto listenersCopy = listeners_;
        for (auto* l : listenersCopy)
            if (l != nullptr)
                l->listAboutToBeDeleted(this);
    }
    shutdown = true;
    freeObjects();
}

void ModulationList::deleteObject(ModulatorBase * base)
{
    //this will call delete in the modulationprocessor
    // base->state.getProperty(IDs::)
    for (auto listener: listeners_)
    {
        listener->removeModulator(base);
    }

    if(shutdown)
    {
        base->parent_->removeModulator(base);
        for (auto vt : base->connections_)
        {
            vt.getParent().removeChild (vt,nullptr);
        }

        delete base;
        return;
    }

    if (parent_ && parent_->getGuiInterface())
    {
        for (const auto& vt : base->connections_)
        {
            vt.getParent().removeChild (vt,nullptr);
        }
        base->parent_->removeModulator(base);
        delete base;

    } else
    {
        // should only ever be called on shutdown
    }
}

ModulatorBase *ModulationList::createNewObject(const juce::ValueTree &v) {

    try {
        auto proc = parent_->modulator_factory.create(v.getProperty(IDs::type).toString().toStdString(),std::any(std::tie( v, um )));
        this->proc_->addModulator(proc);
        return proc;
    } catch (const std::bad_any_cast& e) {
        std::cerr << "Error during object creation: " << e.what() << std::endl;
    }

    return nullptr;
}

void ModulationList::newObjectAdded(ModulatorBase * m) {
    for (auto listener: listeners_)
    {
        listener->modulatorAdded(m);
    }
}

void ModulationList:: valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i)
{
    if(v.getProperty(IDs::sync,0))
    {
        for(auto obj : objects)
        {
            juce::MemoryBlock data;
            obj->getStateInformation(data);
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
            }
        }
        v.removeProperty(IDs::sync, nullptr);
    }
}
