//
// Created by Davis Polito on 6/9/25.
//

#ifndef PLUGINSCANNERSUBPROCESS_H
#define PLUGINSCANNERSUBPROCESS_H
#include "juce_events/juce_events.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "tracktion_AtomicWrapper.h"

class PluginScannerSubprocess final : private juce::ChildProcessWorker,
                                      private juce::AsyncUpdater
{
public:
    PluginScannerSubprocess()
    {
        addDefaultFormatsToManager(formatManager);
    }

    using ChildProcessWorker::initialiseFromCommandLine;

private:
    void handleMessageFromCoordinator (const juce::MemoryBlock& mb) override
    {
        if (mb.isEmpty())
            return;

        const std::lock_guard<std::mutex> lock (mutex);

        // If we are on the message thread (not expected for IPC callback) or it's a fast scan, do it now.
        // Otherwise, defer to the message thread.
        if (juce::MessageManager::getInstance()->isThisTheMessageThread() || isFastScanPossible (mb))
        {
            if (const auto results = doScan (mb); ! results.isEmpty())
            {
                sendResults (results);
                return;
            }
        }

        pendingBlocks.emplace (mb);
        triggerAsyncUpdate();
    }

    void handleConnectionLost() override
    {
        juce::JUCEApplicationBase::quit();
    }

    void handleAsyncUpdate() override
    {
        for (;;)
        {
            const std::lock_guard<std::mutex> lock (mutex);

            if (pendingBlocks.empty())
                return;

            sendResults (doScan (pendingBlocks.front()));
            pendingBlocks.pop();
        }
    }

    bool isFastScanPossible (const juce::MemoryBlock& block)
    {
        juce::MemoryInputStream stream { block, false };
        const auto formatName = stream.readString();
        const auto identifier = stream.readString();

        if (formatName != "VST3")
            return true; // We only care about VST3 slow scans for thread safety here

        const juce::File file (identifier);
        return file.getChildFile ("Contents").getChildFile ("Resources").getChildFile ("moduleinfo.json").exists()
            || file.getChildFile ("Contents").getChildFile ("moduleinfo.json").exists()
            || file.getChildFile ("moduleinfo.json").exists();
    }

    juce::OwnedArray<juce::PluginDescription> doScan (const juce::MemoryBlock& block)
    {
        juce::MemoryInputStream stream { block, false };
        const auto formatName = stream.readString();
        const auto identifier = stream.readString();

        juce::PluginDescription pd;
        pd.fileOrIdentifier = identifier;
        pd.uniqueId = pd.deprecatedUid = 0;

        const auto matchingFormat = [&]() -> juce::AudioPluginFormat*
        {
            for (auto* format : formatManager.getFormats())
                if (format->getName() == formatName)
                    return format;

            return nullptr;
        }();

        juce::OwnedArray<juce::PluginDescription> results;

        if (matchingFormat != nullptr)
        {
            if (matchingFormat->getName() == "VST3")
            {
                // VST3 is special: it asserts if slow-scanned on background thread in DEBUG,
                // and might SIGTRAP if slow-scanned on background thread (e.g. UAD plugins) on macOS.
                if (isFastScanPossible (block) || juce::MessageManager::getInstance()->isThisTheMessageThread())
                {
                    matchingFormat->findAllTypesForFile (results, identifier);
                }
                else
                {
                    // If we get here, it means we are trying a slow scan on a background thread.
                    // This should be avoided by the check in handleMessageFromCoordinator.
                    juce::Logger::writeToLog ("Warning: VST3 slow scan requested on background thread: " + identifier);
                }
            }
            else if (juce::MessageManager::getInstance()->isThisTheMessageThread()
                || matchingFormat->requiresUnblockedMessageThreadDuringCreation (pd))
            {
                matchingFormat->findAllTypesForFile (results, identifier);
            }
            else
            {
                juce::WaitableEvent finished;

                juce::MessageManager::callAsync ([&]
                {
                    matchingFormat->findAllTypesForFile (results, identifier);
                    finished.signal();
                });

                finished.wait (20000); // 20 seconds timeout for scanning a single file
            }
        }

        return results;
    }

    void sendResults (const juce::OwnedArray<juce::PluginDescription>& results)
    {
        juce::XmlElement xml ("LIST");

        for (const auto& desc : results)
            xml.addChildElement (desc->createXml().release());

        const auto str = xml.toString();
        sendMessageToCoordinator ({ str.toRawUTF8(), str.getNumBytesAsUTF8() });
    }

    std::mutex mutex;
    std::queue<juce::MemoryBlock> pendingBlocks;
    juce::AudioPluginFormatManager formatManager;
};
//==============================================================================
class Superprocess final : private juce::ChildProcessCoordinator
{
public:
    Superprocess()
    {
        launchWorkerProcess (juce::File::getSpecialLocation (juce::File::currentExecutableFile), "bitklavierscanner", 0, 0);
    }

    enum class State
    {
        timeout,
        gotResult,
        connectionLost,
    };

    struct Response
    {
        State state;
        std::unique_ptr<juce::XmlElement> xml;
    };

    Response getResponse()
    {
        std::unique_lock<std::mutex> lock { mutex };

        if (! condvar.wait_for (lock, std::chrono::milliseconds { 50 }, [&] { return gotResult || connectionLost; }))
            return { State::timeout, nullptr };

        const auto state = connectionLost ? State::connectionLost : State::gotResult;
        connectionLost = false;
        gotResult = false;

        return { state, std::move (pluginDescription) };
    }

    using ChildProcessCoordinator::sendMessageToWorker;

private:
    void handleMessageFromWorker (const juce::MemoryBlock& mb) override
    {
        const std::lock_guard<std::mutex> lock { mutex };
        pluginDescription = parseXML (mb.toString());
        gotResult = true;
        condvar.notify_one();
    }

    void handleConnectionLost() override
    {
        const std::lock_guard<std::mutex> lock { mutex };
        connectionLost = true;
        condvar.notify_one();
    }

    std::mutex mutex;
    std::condition_variable condvar;

    std::unique_ptr<juce::XmlElement> pluginDescription;
    bool connectionLost = false;
    bool gotResult = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Superprocess)
};
//==============================================================================
class CustomPluginScanner final : public juce::KnownPluginList::CustomScanner

{
public:
    CustomPluginScanner(const juce::ValueTree& vt) : settings(vt), scanInProcess(vt.getProperty("pluginScanMode"))
    {
    }

    ~CustomPluginScanner() override
    {

    }

    bool findPluginTypesFor (juce::AudioPluginFormat& format,
                             juce::OwnedArray<juce::PluginDescription>& result,
                             const juce::String& fileOrIdentifier) override
    {
        bool fastScanPossible = false;
        if (format.getName() == "VST3")
        {
            const juce::File file (fileOrIdentifier);
            if (file.getChildFile ("Contents").getChildFile ("Resources").getChildFile ("moduleinfo.json").exists()
                || file.getChildFile ("Contents").getChildFile ("moduleinfo.json").exists()
                || file.getChildFile ("moduleinfo.json").exists())
            {
                fastScanPossible = true;
            }
        }

        // VST3 slow-scans are dangerous: they can SIGTRAP on load or assert on wrong thread.
        // If we are in-process and it's a VST3 slow-scan, we force it to use the subprocess for safety.
        bool forceSubprocess = (format.getName() == "VST3" && !fastScanPossible);

        if (scanInProcess || forceSubprocess)
        {
            if (addPluginDescriptions (format.getName(), fileOrIdentifier, result))
                return true;

            superprocess = nullptr;
            return false;
        }

        superprocess = nullptr;
        if (format.getName() == "VST3")
        {
#if JUCE_DEBUG
            if (fastScanPossible || juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                format.findAllTypesForFile (result, fileOrIdentifier);
            }
            else
            {
                juce::Logger::writeToLog ("Skipping VST3 plugin in Debug (requires slow scan on message thread): " + fileOrIdentifier);
            }
#else
            format.findAllTypesForFile (result, fileOrIdentifier);
#endif
        }
        else if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        {
            format.findAllTypesForFile (result, fileOrIdentifier);
        }
        else
        {
            juce::WaitableEvent finished;

            juce::MessageManager::callAsync ([&]
            {
                format.findAllTypesForFile (result, fileOrIdentifier);
                finished.signal();
            });

            finished.wait (20000);
        }

        return true;
    }

    void scanFinished() override
    {
        superprocess = nullptr;
    }

private:
    /*  Scans for a plugin with format 'formatName' and ID 'fileOrIdentifier' using a subprocess,
        and adds discovered plugin descriptions to 'result'.

        Returns true on success.

        Failure indicates that the subprocess is unrecoverable and should be terminated.
    */
    bool addPluginDescriptions (const juce::String& formatName,
                                const juce::String& fileOrIdentifier,
                                juce::OwnedArray<juce::PluginDescription>& result)
    {
        if (superprocess == nullptr)
            superprocess = std::make_unique<Superprocess>();

        juce::MemoryBlock block;
        juce::MemoryOutputStream stream { block, true };
        stream.writeString (formatName);
        stream.writeString (fileOrIdentifier);

        if (! superprocess->sendMessageToWorker (block))
            return false;

        for (;;)
        {
            if (shouldExit())
                return true;

            const auto response = superprocess->getResponse();

            if (response.state == Superprocess::State::timeout)
                continue;

            if (response.xml != nullptr)
            {
                for (const auto* item : response.xml->getChildIterator())
                {
                    auto desc = std::make_unique<juce::PluginDescription>();

                    if (desc->loadFromXml (*item))
                        result.add (std::move (desc));
                }
            }

            return (response.state == Superprocess::State::gotResult);
        }
    }



    std::unique_ptr<Superprocess> superprocess;
    juce::ValueTree settings;

    tracktion::AtomicWrapper<int> scanInProcess;
    //std::atomic<bool> scanInProcess { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomPluginScanner)
};
/** A type that encapsulates a PluginDescription and some preferences regarding
    how plugins of that description should be instantiated.
*/
struct PluginDescriptionAndPreference
{
    enum class UseARA { no, yes };

    PluginDescriptionAndPreference() = default;

    explicit PluginDescriptionAndPreference (juce::PluginDescription pd)
        : pluginDescription (std::move (pd)),
          useARA (pluginDescription.hasARAExtension ? PluginDescriptionAndPreference::UseARA::yes
                                                    : PluginDescriptionAndPreference::UseARA::no)
    {}

    PluginDescriptionAndPreference (juce::PluginDescription pd, UseARA ara)
        : pluginDescription (std::move (pd)), useARA (ara)
    {}

    juce::PluginDescription pluginDescription;
    UseARA useARA = UseARA::no;
};


#endif //PLUGINSCANNERSUBPROCESS_H
