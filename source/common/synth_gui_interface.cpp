/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synth_gui_interface.h"
#include "ConstructionSite.h"
#include "CompressorParameterView.h"
#include "EQParameterView.h"
#include "SampleLoadManager.h"
#include "UserPreferences.h"
#include "load_save.h"
#include "modulation_manager.h"
#include "sound_engine.h"
#include "synth_base.h"

SynthGuiData::SynthGuiData (SynthBase* synth_base) : synth (synth_base),
                                                     tree (synth_base->getValueTree()),
                                                     um (synth_base->getUndoManager())
{
    //tree = mainSynth->getValueTree();
    //    um = synth_base->getUndoManager();
    //sampleLoadManager->loadSamples(0, true);
}
#if HEADLESS

SynthGuiInterface::SynthGuiInterface (SynthBase* mainSynth, bool use_gui) : synth_ (mainSynth) {}
SynthGuiInterface::~SynthGuiInterface() {}
void SynthGuiInterface::updateFullGui() {}
void SynthGuiInterface::updateGuiControl (const std::string& name, float value) {}
float SynthGuiInterface::getControlValue (const std::string& name) { return 0.0f; }
void SynthGuiInterface::connectModulation (std::string source, std::string destination) {}
void SynthGuiInterface::connectModulation (bitklavier::ModulationConnection* connection) {}
void SynthGuiInterface::setModulationValues (const std::string& source, const std::string& destination, float amount, bool bipolar, bool stereo, bool bypass) {}
void SynthGuiInterface::disconnectModulation (std::string source, std::string destination) {}
void SynthGuiInterface::disconnectModulation (bitklavier::ModulationConnection* connection) {}
void SynthGuiInterface::setFocus() {}
void SynthGuiInterface::notifyChange() {}
void SynthGuiInterface::notifyFresh() {}
void SynthGuiInterface::openSaveDialog() {}
void SynthGuiInterface::externalPresetLoaded (juce::File preset) {}
void SynthGuiInterface::setGuiSize (float scale) {}

#else
    #include "../interface/fullInterface.h"
    #include "../interface/look_and_feel/default_look_and_feel.h"
    #include <memory>
    #include "PluginList.h"

SynthGuiInterface::SynthGuiInterface (SynthBase* synth, bool use_gui) : synth_ (synth)
{
    gallery = synth_->getValueTree();
    auto sets = synth->sampleLoadManager->getAllSampleSets();
    // auto it = std::find(sets.begin(), sets.end(), "Default");
    auto it = std::find(sets.begin(), sets.end(), "Yamaha_Default");
    int defaultIndex = (it != sets.end())
        ? static_cast<int>(std::distance(sets.begin(), it))
        : -1;
    if (defaultIndex >= 0) {
        synth_->sampleLoadManager->loadSamples(sets[defaultIndex], synth_->getValueTree());
    }
    if (use_gui) {
        SynthGuiData synth_data (synth_);
        gui_ = std::make_unique<FullInterface> (&synth_data, commandManager);
        gui_->showLoadingSection();
        // for registering hotkeys etc.
        commandManager.registerAllCommandsForTarget(this);
        if (defaultIndex >= 0)
            gui_->header_->setSampleSelectText(sets[defaultIndex]);
    }
}

SampleLoadManager* SynthGuiInterface::getSampleLoadManager() {
    return getSynth()->sampleLoadManager.get();
}

bool SynthGuiInterface::perform(const InvocationInfo & info) {
    {
        switch (info.commandID) {
            case undo:
            {
                getUndoManager()->undo();
                // juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Undo", " Undo triggered");
                return true;
            }
            case redo:
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Redo", "Redo triggered");
                return true;
            }
            case CommandIDs::showPluginListEditor:
            {
                if (pluginListWindow == nullptr)
                    pluginListWindow.reset (new SynthGuiInterface::PluginListWindow (*this, *synth_->user_prefs, synth_->user_prefs->userPreferences->formatManager));

                pluginListWindow->toFront (true);
                return true;
            }

            default:
                return false;
        }
    }
}

SynthGuiInterface::~SynthGuiInterface() {
    DBG("synthguiinterface destroyed");
    synth_->clearAllGuiListeners();
}

void SynthGuiInterface::updateFullGui() {
    if (gui_ == nullptr)
        return;

    gui_->reset();
}

OpenGlWrapper *SynthGuiInterface::getOpenGlWrapper() {
    return &gui_->open_gl_;
}

juce::CriticalSection* SynthGuiInterface::getOpenGlCriticalSection()
{
    return &gui_->open_gl_critical_section_;
}

void SynthGuiInterface::updateGuiControl(const std::string &name, float value) {
    if (gui_ == nullptr)
        return;

    //  gui_->setValue(name, value, NotificationType::dontSendNotification);
}

void SynthGuiInterface::notifyModulationsChanged() {
    gui_->modulationChanged();
}

void SynthGuiInterface::notifyPrepPopupMoved() {
    gui_->modulation_manager->resized();
}

void SynthGuiInterface::connectStateModulation(std::string source, std::string destination) {
    bool created = synth_->connectStateModulation(source, destination);
    //  if (created)
    //    initModulationValues(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::connectModulation(std::string source, std::string destination) {
    DBG("SynthGuiInterface::connectModulation");
    bool created = synth_->connectModulation(source, destination);
    // if (created)
    //   initModulationValues(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::disconnectModulation(std::string source, std::string destination) {
    synth_->disconnectModulation(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::disconnectStateModulation(std::string source, std::string destination) {
    synth_->disconnectStateModulation(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::disconnectModulation(bitklavier::ModulationConnection *connection) {
    // synth_->disconnectModulation(connection);
    notifyModulationsChanged();
}

void SynthGuiInterface::disconnectModulation(bitklavier::StateConnection *connection) {
    // synth_->disconnectModulation(connection);
    notifyModulationsChanged();
}

bool SynthGuiInterface::loadFromFile(juce::File preset, std::string &error) {
    bool success = getSynth()->loadFromFile(preset, error);
    if (success)
    {
        gui_->header_->gallerySelectText->setText(preset.getFileNameWithoutExtension());
        gui_->header_->updateCurrentPianoName();
    }
    return success;
    //sampleLoadManager->loadSamples()
}

juce::UndoManager *SynthGuiInterface::getUndoManager() {
    return &getSynth()->getUndoManager();
}

void SynthGuiInterface::tryEnqueueProcessorInitQueue(juce::FixedSizeFunction<64, void()> callback) {
    if (loading) {
        callback();
    }
    else {
        synth_->processorInitQueue.try_enqueue(std::move(callback));
    }
}

void SynthGuiInterface::setFocus() {
    if (gui_ == nullptr)
        return;

    gui_->setFocus();
}

void SynthGuiInterface::notifyChange() {
    if (gui_ == nullptr)
        return;

    gui_->notifyChange();
}

void SynthGuiInterface::notifyFresh() {
    if (gui_ == nullptr)
        return;

    gui_->notifyFresh();
}

void SynthGuiInterface::setGuiSize (float scale)
{
    if (gui_ == nullptr)
        return;

    juce::Point<int> position = gui_->getScreenBounds().getCentre();
    const juce::Displays::Display& display = juce::Desktop::getInstance().getDisplays().findDisplayForPoint (position);

    juce::Rectangle<int> display_area = juce::Desktop::getInstance().getDisplays().getTotalBounds (true);
    juce::ComponentPeer* peer = gui_->getPeer();
    if (peer)
        peer->getFrameSize().subtractFrom (display_area);

    float window_size = scale / display.scale;
    window_size = std::min (window_size, display_area.getWidth() * 1.0f / bitklavier::kDefaultWindowWidth);
    window_size = std::min (window_size, display_area.getHeight() * 1.0f / bitklavier::kDefaultWindowHeight);
    //LoadSave::saveWindowSize(window_size);

    int width = std::round (window_size * bitklavier::kDefaultWindowWidth);
    int height = std::round (window_size * bitklavier::kDefaultWindowHeight);

    juce::Rectangle<int> bounds = gui_->getBounds();
    bounds.setWidth (width);
    bounds.setHeight (height);
    gui_->getParentComponent()->setBounds (bounds);
    gui_->redoBackground();
}

bool SynthGuiInterface::isConnected (juce::AudioProcessorGraph::Connection& connection)
{
    return synth_->getEngine()->isConnected (connection);
}

void SynthGuiInterface::addProcessor (std::unique_ptr<juce::AudioPluginInstance> instance)
{
    // tryEnqueueProcessorInitQueue([this, object] {
    //     if (auto listener = dynamic_cast<juce::ValueTree::Listener *>(object->getProcessor()))
    //         sampleLoadManager->t.addListener(listener);
    //     object->setNodeInfo(getSynth()->addProcessor(std::move(object->getProcessorPtr()), object->pluginID));
    // });
}

void SynthGuiInterface::addModulationNodeConnection (juce::AudioProcessorGraph::NodeID source,
    juce::AudioProcessorGraph::NodeID destination)
{
    tryEnqueueProcessorInitQueue ([this, source, destination] {
        synth_->addModulationConnection (source, destination);
    });
}

juce::File SynthGuiInterface::getActiveFile()
{
    return synth_->getActiveFile();
}

void SynthGuiInterface::openLoadDialog()
{
    auto active_file = getActiveFile();
    filechooser = std::make_unique<juce::FileChooser> ("Open Gallery", active_file, juce::String ("*.") + bitklavier::kPresetExtension);

    auto flags = juce::FileBrowserComponent::openMode
                 | juce::FileBrowserComponent::canSelectFiles;
    filechooser->launchAsync (flags, [this] (const juce::FileChooser& fc) {
        if (fc.getResult() == juce::File {})
        {
            return;
        }

        std::string error;
        juce::File choice = fc.getResult();
        loading = true;
        if (!this->loadFromFile (choice, error))
        {
            DBG (error);
        }
        loading = false;
    });
}

void SynthGuiInterface::openSaveDialog()
{
    filechooser = std::make_unique<juce::FileChooser> ("Export the gallery", juce::File(), juce::String ("*.") + bitklavier::kPresetExtension, true);
    filechooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories,
        [this] (const juce::FileChooser& chooser) {
            auto result = chooser.getURLResult();
            auto name = result.isEmpty()
                            ? juce::String()
                            : (result.isLocalFile()
                                      ? result.getLocalFile().getFullPathName()
                                      : result.toString (true));
            juce::File file (name);
            if (!result.isEmpty())
            {
                if (getSynth()->saveToFile(file))
                {
                    if (gui_ && gui_->header_ && gui_->header_->gallerySelectText)
                        gui_->header_->gallerySelectText->setText(file.getFileNameWithoutExtension());
                }
            }
        });
}

void SynthGuiInterface::saveCurrentGallery()
{
    // If there is no active file yet (brand‑new document), fall back to "Save As..."
    auto active = getActiveFile();
    if (active == juce::File())
    {
        openSaveDialog();
        return;
    }

    // Protect the default "Basic Piano" from accidental overwrite with Cmd+S
    // Detect the known installed location in the user's Documents/bitKlavier/galleries
    auto docs = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                    .getChildFile("Documents")
                    .getChildFile("bitKlavier")
                    .getChildFile("galleries");
    juce::File basicA = docs.getChildFile("Basic Piano.").withFileExtension(bitklavier::kPresetExtension.c_str());
    juce::File basicB = docs.getChildFile("Basic Piano").withFileExtension(bitklavier::kPresetExtension.c_str());
    juce::File basicC = docs.getChildFile("BasicPiano").withFileExtension(bitklavier::kPresetExtension.c_str());

    const auto activePath = active.getFullPathName();
    if (activePath == basicA.getFullPathName()
        || activePath == basicB.getFullPathName()
        || activePath == basicC.getFullPathName())
    {
        // Use Save As so the default preset isn't overwritten
        openSaveDialog();
        return;
    }

    // Otherwise save to the currently active file
    synth_->saveToFile(active);
}

void SynthGuiInterface::setActivePiano (const juce::ValueTree& v)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (synth_->switch_trigger_thread == SwitchTriggerThread::MessageThread)
        synth_->setActivePiano (v, synth_->switch_trigger_thread);
    gui_->main_->constructionSite_->setActivePiano();
}

std::vector<std::string> SynthGuiInterface::getAllPianoNames()
{
    std::vector<std::string> names;
    for (const auto& vt : gallery)
    {
        if (vt.hasType (IDs::PIANO))
        {
            names.push_back (vt.getProperty (IDs::name).toString().toStdString());
        }
    }
    return names;
}

void SynthGuiInterface::allNotesOff()
{
    synth_->getEngine()->allNotesOff();
}

void SynthGuiInterface::setPianoSwitchTriggerThreadMessage()
{
    synth_->switch_trigger_thread = SwitchTriggerThread::MessageThread;
}

void SynthGuiInterface::removeAllGuiListeners()
{
    if (gui_ == nullptr)
        return;

    gui_->removeAllGuiListeners();
}

void SynthGuiInterface::addPiano (const juce::String& piano_name)
{
}

const juce::CriticalSection& SynthGuiInterface::getCriticalSection()
{
    return synth_->getCriticalSection();
}

bool SynthGuiInterface::isConnected (juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest)
{
    return synth_->isConnected (src, dest);
}

static bool containsDuplicateNames (const juce::Array<juce::PluginDescription>& plugins, const juce::String& name)
{
    int matches = 0;

    for (auto& p : plugins)
        if (p.name == name && ++matches > 1)
            return true;

    return false;
}

static constexpr int menuIDBase = bitklavier::BKPreparationType::PreparationTypeVST;
static void addToMenu (const juce::KnownPluginList::PluginTree& tree,
    PopupItems& m,
    const juce::Array<juce::PluginDescription>& allPlugins,
    juce::Array<PluginDescriptionAndPreference>& addedPlugins)
{
    for (auto* sub : tree.subFolders)
    {
        PopupItems subMenu;
        subMenu.name = sub->folder.toStdString();
        addToMenu (*sub, subMenu, allPlugins, addedPlugins);

        m.addItem (subMenu);
    }

    auto addPlugin = [&] (const auto& descriptionAndPreference, const auto& pluginName) {
        addedPlugins.add (descriptionAndPreference);
        const auto menuID = addedPlugins.size() - 1 + menuIDBase;
        m.addItem (menuID, pluginName.toStdString());
    };

    for (auto& plugin : tree.plugins)
    {
        auto name = plugin.name;

        if (containsDuplicateNames (tree.plugins, name))
            name << " (" << plugin.pluginFormatName << ')';

        addPlugin (PluginDescriptionAndPreference { plugin, PluginDescriptionAndPreference::UseARA::no }, name);

    #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
        if (plugin.hasARAExtension)
        {
            name << " (ARA)";
            addPlugin (PluginDescriptionAndPreference { plugin }, name);
        }
    #endif
    }
}

PopupItems SynthGuiInterface::getPluginPopupItems()
{
    PopupItems popup = getPreparationPopupItems();

    auto pluginDescriptions = synth_->user_prefs->userPreferences->knownPluginList.getTypes();

    auto tree = juce::KnownPluginList::createTree (pluginDescriptions, synth_->user_prefs->userPreferences->pluginSortMethod);
    synth_->user_prefs->userPreferences->pluginDescriptionsAndPreference = {};
    popup.addItem (-1, "");
    addToMenu (*tree, popup, pluginDescriptions, synth_->user_prefs->userPreferences->pluginDescriptionsAndPreference);
    return popup;
}

PopupItems SynthGuiInterface::getPreparationPopupItems()
{
    PopupItems popup;

    PopupItems separator ("separator");
    separator.enabled = false; // This makes it non-selectable
    separator.id = -1; // will be a separator line

    for (const auto& [type, name] : bitklavier::AllPrepInfo) {
        // 'type' is the BKPreparationType
        // 'name' is the std::string
        printf("Type ID: %d | Display Name: %s\n", (int)type, name.c_str());
        if (type < bitklavier::BKPreparationType::PreparationTypeComment) // leave out those above this for now
            popup.addItem(type, name);
        if (type == bitklavier::BKPreparationType::PreparationTypeResonance ||
            type == bitklavier::BKPreparationType::PreparationTypeTempo ||
            type == bitklavier::BKPreparationType::PreparationTypeMidiTarget)
            popup.addItem(separator);
    }

    return popup;
}

PopupItems SynthGuiInterface::getVSTPopupItems()
{
    PopupItems popup;

    auto pluginDescriptions = synth_->user_prefs->userPreferences->knownPluginList.getTypes();

    auto tree = juce::KnownPluginList::createTree (pluginDescriptions, synth_->user_prefs->userPreferences->pluginSortMethod);
    synth_->user_prefs->userPreferences->pluginDescriptionsAndPreference = {};
    popup.addItem (-1, "");
    addToMenu (*tree, popup, pluginDescriptions, synth_->user_prefs->userPreferences->pluginDescriptionsAndPreference);
    return popup;
}

const std::vector<std::string> SynthGuiInterface::getAllGalleries()
{
    std::vector<std::string> galleriesList;

    auto galleriesPath = synth_->user_prefs->userPreferences->tree.getProperty ("default_galleries_path");
    juce::File baseDir(galleriesPath);

    if (baseDir.isDirectory()) {
        // 1. Directories
        juce::Array<juce::File> dirs = baseDir.findChildFiles(
            juce::File::findDirectories,
            false
        );

        for (auto &d: dirs)
            galleriesList.push_back(d.getFileName().toStdString());
    }

    return galleriesList;
}

std::unique_ptr<SynthSection> SynthGuiInterface::getCompressorPopup() {
    auto proc = synth_->getEngine()->getCompressorProcessor();
    return std::make_unique<CompressorParameterView> (proc->getState(), proc->getState().params, proc->v.getProperty (IDs::uuid).toString(), getOpenGlWrapper());
}

std::unique_ptr<SynthSection> SynthGuiInterface::getEQPopup() {
    auto proc = synth_->getEngine()->getEQProcessor();
    return std::make_unique<EQParameterView> (proc->getState(), proc->getState().params, proc->v.getProperty (IDs::uuid).toString(), getOpenGlWrapper());
}

#endif
