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
        parent_(p), proc_(proc)
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
    if (parent_->getGuiInterface()) {
        parent_->getGuiInterface()->tryEnqueueProcessorInitQueue(
            [this, base] {
                if (base->parent_ != nullptr) {

                    base->parent_->removeModulator(base);
                    base->parent_->callOnMainThread ([this, base] {
                        for (auto vt : base->connections_)
                        {
                            vt.getParent().removeChild (vt,nullptr);
                        }
                        // for (auto listener: listeners_)
                        // {
                        //     listener->removeModulator(base);
                        // }
                        delete base;

                    });
                    // delete base;
                }

            });
    }



}
ModulatorBase *ModulationList::createNewObject(const juce::ValueTree &v) {
//LEAF* leaf = parent->getLEAF();
    std::any args = std::make_tuple( v );

    try {
        auto proc = parent_->modulator_factory.create(v.getProperty(IDs::type).toString().toStdString(),args);
        if (!isInit) {
            parent_->getGuiInterface()->tryEnqueueProcessorInitQueue(
                    [this, proc] {
                        this->proc_->addModulator(proc);
                    });
        }
        else {
            this->proc_->addModulator(proc);
        }
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