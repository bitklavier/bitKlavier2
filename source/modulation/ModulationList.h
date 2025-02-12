//
// Created by Davis Polito on 2/11/25.
//

#ifndef BITKLAVIER2_MODULATIONLIST_H
#define BITKLAVIER2_MODULATIONLIST_H
#include <tracktion_ValueTreeUtilities.h>
#include "Identifiers.h"
class ModulatorBase;
class SynthBase;
namespace bitklavier {
    class ModulationProcessor;
}
class ModulationList : public tracktion::engine::ValueTreeObjectList<ModulatorBase>
{
public:
    class Listener
    {
    public :
        virtual ~Listener(){}
        virtual void modulatorAdded( ModulatorBase*) = 0;
//        virtual void modulationRemoved() = 0;
    };
    ModulationList(const juce::ValueTree&, SynthBase*, bitklavier::ModulationProcessor*);
    ~ModulationList();
    ModulatorBase* createNewObject(const juce::ValueTree &) override;
    void newObjectAdded (ModulatorBase*) override;
    void objectRemoved (ModulatorBase*) override     { }//resized(); }
    void objectOrderChanged() override              { }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override{};
    void valueTreeRedirected (juce::ValueTree&) override{};
    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType(IDs::modulationproc);
    }

    juce::ValueTree getValueTree()
    {
        return parent;
    }
    void deleteObject(ModulatorBase*){}
    bitklavier::ModulationProcessor *proc_;
    SynthBase* parent_;
    void addListener (Listener* l) { listeners_.push_back (l); }

//    void removeListener (Listener* l) { listeners_.remove (l); }
private:
    std::vector<Listener*> listeners_;

};


#endif //BITKLAVIER2_MODULATIONLIST_H
