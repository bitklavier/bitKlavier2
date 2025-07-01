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

#include "SampleLoadManager.h"
#include "UserPreferences.h"

#include "sound_engine.h"
#include "load_save.h"
#include "synth_base.h"
#include "PreparationSection.h"

SynthGuiData::SynthGuiData(SynthBase *synth_base) : synth(synth_base),
                                                    tree(synth_base->getValueTree()),
                                                    um(synth_base->getUndoManager()) {
    //tree = mainSynth->getValueTree();
    //    um = synth_base->getUndoManager();
    //sampleLoadManager->loadSamples(0, true);
}
#if HEADLESS

SynthGuiInterface::SynthGuiInterface(SynthBase* mainSynth, bool use_gui) : synth_(mainSynth) { }
SynthGuiInterface::~SynthGuiInterface() { }
void SynthGuiInterface::updateFullGui() { }
void SynthGuiInterface::updateGuiControl(const std::string& name, float value) { }
float SynthGuiInterface::getControlValue(const std::string& name) { return 0.0f; }
void SynthGuiInterface::connectModulation(std::string source, std::string destination) { }
void SynthGuiInterface::connectModulation(bitklavier::ModulationConnection* connection) { }
void SynthGuiInterface::setModulationValues(const std::string& source, const std::string& destination,
                                            float amount, bool bipolar, bool stereo, bool bypass) { }
void SynthGuiInterface::disconnectModulation(std::string source, std::string destination) { }
void SynthGuiInterface::disconnectModulation(bitklavier::ModulationConnection* connection) { }
void SynthGuiInterface::setFocus() { }
void SynthGuiInterface::notifyChange() { }
void SynthGuiInterface::notifyFresh() { }
void SynthGuiInterface::openSaveDialog() { }
void SynthGuiInterface::externalPresetLoaded(juce::File preset) { }
void SynthGuiInterface::setGuiSize(float scale) { }

#else
#include <memory>
#include "../interface/look_and_feel/default_look_and_feel.h"
#include "../interface/fullInterface.h"

#include "PluginList.h"
SynthGuiInterface::SynthGuiInterface(SynthBase *synth, bool use_gui) : synth_(synth),
                                                                       userPreferences(new UserPreferencesWrapper()),
                                                                       sampleLoadManager(
                                                                           new SampleLoadManager(
                                                                               userPreferences,
                                                                               this, synth)) {
    if (use_gui) {
        SynthGuiData synth_data(synth_);
        gui_ = std::make_unique<FullInterface>(&synth_data, commandManager);
        // for registering hotkeys etc.
        commandManager.registerAllCommandsForTarget(this);
    }

    sampleLoadManager->preferences = userPreferences;
    sampleLoadManager->loadSamples(0,true);
    synth_->user_prefs = userPreferences;

    //sampleLoadManager->loadSamples(0, true);
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
                    pluginListWindow.reset (new SynthGuiInterface::PluginListWindow (*this, *userPreferences, userPreferences->userPreferences->formatManager));

                pluginListWindow->toFront (true);
                return true;
            }

            default:
                return false;
        }
    }
}

SynthGuiInterface::~SynthGuiInterface() {
}


void SynthGuiInterface::updateFullGui() {
    if (gui_ == nullptr)
        return;

    gui_->reset();
}

OpenGlWrapper *SynthGuiInterface::getOpenGlWrapper() {
    return &gui_->open_gl_;
}

void SynthGuiInterface::updateGuiControl(const std::string &name, float value) {
    if (gui_ == nullptr)
        return;

    //  gui_->setValue(name, value, NotificationType::dontSendNotification);
}

void SynthGuiInterface::notifyModulationsChanged() {
    gui_->modulationChanged();
}

void SynthGuiInterface::connectStateModulation(std::string source, std::string destination) {
    bool created = synth_->connectStateModulation(source, destination);
    //  if (created)
    //    initModulationValues(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::connectModulation(std::string source, std::string destination) {
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
    return getSynth()->loadFromFile(preset, error);
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


void SynthGuiInterface::setGuiSize(float scale) {
    if (gui_ == nullptr)
        return;

    juce::Point<int> position = gui_->getScreenBounds().getCentre();
    const juce::Displays::Display &display = juce::Desktop::getInstance().getDisplays().findDisplayForPoint(position);

    juce::Rectangle<int> display_area = juce::Desktop::getInstance().getDisplays().getTotalBounds(true);
    juce::ComponentPeer *peer = gui_->getPeer();
    if (peer)
        peer->getFrameSize().subtractFrom(display_area);

    float window_size = scale / display.scale;
    window_size = std::min(window_size, display_area.getWidth() * 1.0f / bitklavier::kDefaultWindowWidth);
    window_size = std::min(window_size, display_area.getHeight() * 1.0f / bitklavier::kDefaultWindowHeight);
    //LoadSave::saveWindowSize(window_size);

    int width = std::round(window_size * bitklavier::kDefaultWindowWidth);
    int height = std::round(window_size * bitklavier::kDefaultWindowHeight);

    juce::Rectangle<int> bounds = gui_->getBounds();
    bounds.setWidth(width);
    bounds.setHeight(height);
    gui_->getParentComponent()->setBounds(bounds);
    gui_->redoBackground();
}

bool SynthGuiInterface::isConnected(juce::AudioProcessorGraph::Connection &connection) {
    return synth_->getEngine()->isConnected(connection);
}

void SynthGuiInterface::addProcessor(std::unique_ptr<juce::AudioPluginInstance> instance ) {
    // tryEnqueueProcessorInitQueue([this, object] {
    //     if (auto listener = dynamic_cast<juce::ValueTree::Listener *>(object->getProcessor()))
    //         sampleLoadManager->t.addListener(listener);
    //     object->setNodeInfo(getSynth()->addProcessor(std::move(object->getProcessorPtr()), object->pluginID));
    // });
}

void SynthGuiInterface::addModulationNodeConnection(juce::AudioProcessorGraph::NodeID source,
                                                    juce::AudioProcessorGraph::NodeID destination) {
    tryEnqueueProcessorInitQueue([this, source, destination] {
        synth_->addModulationConnection(source, destination);
    });
}

juce::File SynthGuiInterface::getActiveFile() {
    return synth_->getActiveFile();
}

void SynthGuiInterface::openLoadDialog() {
    auto active_file = getActiveFile();
    filechooser = std::make_unique<juce::FileChooser>("Open Preset", active_file,
                                                      juce::String("*.") + bitklavier::kPresetExtension);

    auto flags = juce::FileBrowserComponent::openMode
                 | juce::FileBrowserComponent::canSelectFiles;
    filechooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
        if (fc.getResult() == juce::File{}) {
            return;
        }

        std::string error;
        juce::File choice = fc.getResult();
        loading = true;
        if (!this->loadFromFile(choice, error)) {
            //            std::string name = ProjectInfo::projectName;
            //            error = "There was an error open the preset. " + error;
            //juce::AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "PRESET ERROR, ""Error opening preset", error);
            DBG(error);
        }
        loading = false;
        //        else
        //            parent->externalPresetLoaded(choice);
    });
}

void SynthGuiInterface::openSaveDialog() {
    filechooser = std::make_unique<juce::FileChooser>("Export the gallery", juce::File(),
                                                      juce::String("*.") + bitklavier::kPresetExtension, true);
    filechooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
        juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser &chooser) {
            getSynth()->getValueTree().getChildWithName(IDs::PIANO).getChildWithName(IDs::PREPARATIONS).setProperty(
                "sync", 1, nullptr);
            juce::String mystr = (getSynth()->getValueTree().toXmlString());
            auto xml = getSynth()->getValueTree().createXml();
            juce::XmlElement xml_ = *xml;

            auto result = chooser.getURLResult();
            auto name = result.isEmpty()
                            ? juce::String()
                            : (result.isLocalFile()
                                   ? result.getLocalFile().getFullPathName()
                                   : result.toString(true));
            juce::File file(name);
            if (!result.isEmpty()) {
                juce::FileOutputStream output(file);
                output.setPosition(0);
                output.truncate();
                output.writeText(xml_.toString(), false, false, {});
                //                                         std::unique_ptr<juce::InputStream> wi (file.createInputStream());
                //                                         std::unique_ptr<juce::OutputStream> wo (result.createOutputStream());
                //
                //                                         if (wi != nullptr && wo != nullptr)
                //                                         {
                //                                             //auto numWritten = wo->writeFromInputStream (*wi, -1);
                //                                             wo->flush();
                //                                         }
                output.flush();
            }
        });
}

//probably dont need to do this. i think we can just do this from the modulationmodulesection

const juce::CriticalSection &SynthGuiInterface::getCriticalSection() {
    return synth_->getCriticalSection();
}

bool SynthGuiInterface::isConnected(juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest) {
    return synth_->isConnected(src, dest);
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
        subMenu.name    = sub->folder.toStdString();
        addToMenu (*sub, subMenu, allPlugins, addedPlugins);

        m.addItem (subMenu);
    }

    auto addPlugin = [&] (const auto& descriptionAndPreference, const auto& pluginName)
    {
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
PopupItems SynthGuiInterface::getPluginPopupItems() {
    PopupItems popup;

    popup.addItem(bitklavier::BKPreparationType::PreparationTypeDirect,"Direct");
    popup.addItem(bitklavier::BKPreparationType::PreparationTypeKeymap,"Keymap");
    popup.addItem(bitklavier::BKPreparationType::PreparationTypeTuning,"Tuning");
    popup.addItem(bitklavier::BKPreparationType::PreparationTypeModulation,"Modulation");

    auto pluginDescriptions = userPreferences->userPreferences->knownPluginList.getTypes();

    auto tree = juce::KnownPluginList::createTree (pluginDescriptions, userPreferences->userPreferences->pluginSortMethod);
    userPreferences->userPreferences->pluginDescriptionsAndPreference = {};
    popup.addItem(-1,"");
    addToMenu(*tree,popup,pluginDescriptions,userPreferences->userPreferences->pluginDescriptionsAndPreference);
    return popup;
}

#endif
