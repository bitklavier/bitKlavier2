//
// Created by Dan Trueman on 8/2/25.
//

#include "MidiTargetProcessor.h"

MidiTargetProcessor::MidiTargetProcessor ( SynthBase& parent,
    const juce::ValueTree& v, juce::UndoManager* um) : PluginBase (parent, v, um, midiTargetBusLayout())
{
    connectedPrepIds.ensureStorageAllocated (10);
    this->v.getOrCreateChildWithName (IDs::PARAM_DEFAULT, nullptr);

    // Defer listener attach instead of doing it here immediately:
    // parent.getActiveConnectionList()->addListener (this); // remove/avoid

    // Try a few times on the message thread; 50 ms gives time for lists to exist
    startTimer (50);
}

void MidiTargetProcessor::timerCallback()
{
    if (! listenerAttached)
        tryAttachListener();

    if (listenerAttached || ++attachAttempts >= kMaxAttachAttempts)
        stopTimer();
}

void MidiTargetProcessor::refreshSubscription()
{
    subscription.release();
    connectedPrepIds.clear();
    setConnectedPrep(IDs::noConnection);
    listenerAttached = false;
    attachAttempts = 0;
    tryAttachListener();
}

void MidiTargetProcessor::tryAttachListener()
{
    if (auto* list = parent.getActiveConnectionList())
    {
        subscription = list->subscribe (this);
        listenerAttached = true;

        // Re-process existing connections
        for (auto* connection : *list)
        {
            connectionAdded (connection);
        }
    }
}

void MidiTargetProcessor::connectionAdded (bitklavier::Connection* connection)
{
    auto preparationList = parent.getActivePreparationListValueTree();
    // if you are connecting a midi target, set connectedPrep. if already set, only connect to that type
    auto midiTarget_id = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty (IDs::nodeID));
    if (connection->src_id.get() == midiTarget_id)
    {
        auto nodeIdVar = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection->dest_id.get());
        auto prep = preparationList.getChildWithProperty (IDs::nodeID, nodeIdVar);
        auto prepType = prep.getType();
        if (getConnectedPrep() == IDs::noConnection) setConnectedPrep (prepType);
        else if (getConnectedPrep() != prepType)
        {
            juce::String message = "You've already connected your midiTarget to a " + getConnectedPrep().toString() +
                " preparation, which means you can't connect to a " + prepType.toString() + " preparation.";
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                "Invalid Connection", message);
            parent.getActiveConnectionList()->removeChild (connection->state, nullptr);
            return;
        }
        connectedPrepIds.add (nodeIdVar);
    }
}

void MidiTargetProcessor::removeConnection(bitklavier::Connection* connection)
{
    auto nodeIdVar = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::toVar(connection->dest_id.get());
    // if you are disconnecting a prep from a midi target, remove it from the
    // array of node ids. if that array is empty, reset the connectedPrep param
    auto midiTarget_id = juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(v.getProperty (IDs::nodeID));
    if ((connection->src_id.get() == midiTarget_id) && (connectedPrepIds.contains (nodeIdVar)))
    {
        connectedPrepIds.removeFirstMatchingValue (nodeIdVar);
        if (connectedPrepIds.isEmpty ()) setConnectedPrep (IDs::noConnection);
    };
}

void MidiTargetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // keep this copy around the iterate through
    juce::MidiBuffer saveMidi (midiMessages);

    // clear the midi buffer, because we're only going to pass the ones we want, by channel
    midiMessages.clear();

    for (auto mi : saveMidi)
    {
        auto message = mi.getMessage();

        /**
         * todo: should be able to set the limits on this loop based on
         *       the connected prep and otherwise leave this.
         */

        int startParam = 0;
        int lastParam = 0;

        if (getConnectedPrep() == IDs::blendronic)
        {
            startParam = BlendronicTargetFirst + 1;
            lastParam = BlendronicTargetNil;
        }
        else if (getConnectedPrep() == IDs::synchronic)
        {
            startParam = SynchronicTargetFirst + 1;
            lastParam = SynchronicTargetNil;
        }
        else if (getConnectedPrep() == IDs::resonance)
        {
            startParam = ResonanceTargetFirst + 1;
            lastParam = ResonanceTargetNil;
        }
        else if (getConnectedPrep() == IDs::nostalgic)
        {
            startParam = NostalgicTargetFirst + 1;
            lastParam = NostalgicTargetNil;
        }
        else if (getConnectedPrep() == IDs::direct)
        {
            startParam = DirectTargetFirst + 1;
            lastParam = DirectTargetNil;
        }
        else if (getConnectedPrep() == IDs::tuning)
        {
            startParam = TuningTargetFirst + 1;
            lastParam = TuningTargetNil;
        }
        else if (getConnectedPrep() == IDs::tempo)
        {
            startParam = TempoTargetFirst + 1;
            lastParam = TempoTargetNil;
        }

        for (int i = startParam; i < lastParam; ++i)
        {
            // Cast the integer back to the enum type
            PreparationParameterTargetType currentTarget = static_cast<PreparationParameterTargetType> (i);
            if (state.params.targetMapper[currentTarget]->get())
            {
                int newchan = i - startParam + 1;
                if(state.params.noteModeMapper[currentTarget]->get() == _NoteOn)
                {
                    //only allow noteOffs through
                    if (message.isNoteOn())
                    {
                        juce::MidiMessage newmsg(message);
                        newmsg.setChannel (newchan);
                        midiMessages.addEvent (newmsg, mi.samplePosition);
                    }
                }
                else if(state.params.noteModeMapper[currentTarget]->get() == _NoteOff)
                {
                    //only allow noteOffs through
                    if (message.isNoteOff())
                    {
                        juce::MidiMessage newmsg(message);
                        newmsg.setChannel (newchan);
                        midiMessages.addEvent (newmsg, mi.samplePosition);
                    }
                }
                else
                {
                    //put it through for both noteOns and noteOffs
                    DBG("passing through bo0th ons and offs");
                    juce::MidiMessage newmsg(message);
                    newmsg.setChannel (newchan);
                    midiMessages.addEvent (newmsg, mi.samplePosition);
                }
            }
        }
    }
}


