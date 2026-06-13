// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"
#include "interface/FullInterface.h"
#include "interface/look_and_feel/default_look_and_feel.h"
//namespace ProjectInfo
//{
//    const char* const  projectName    = "bitKlavier";
//    const char* const  companyName    = "Many Arrows Music";
//    const char* const  versionString  = "3.4a";
//    const int          versionNumber  = 0x30400;
//}
PluginEditor::PluginEditor (PluginProcessor& p)
    : juce::AudioProcessorEditor (&p), SynthGuiInterface (&p), processorRef (p), was_animating_ (true)
{
    DBG ("PluginEditor constructed: " + juce::String::toHexString ((juce::uint64) this) + " for processor: " + juce::String::toHexString ((juce::uint64) &p));
    static constexpr int kHeightBuffer = 50;
    juce::ignoreUnused (processorRef);
    setLookAndFeel (DefaultLookAndFeel::instance());
    gui_->reset();
    addAndMakeVisible (gui_.get());

    constrainer_.setMinimumSize (bitklavier::kMinWindowWidth, bitklavier::kMinWindowHeight);
    double ratio = (1.0 * bitklavier::kDefaultWindowWidth) / bitklavier::kDefaultWindowHeight;
    constrainer_.setFixedAspectRatio (ratio);
    constrainer_.setGui (gui_.get());
    setConstrainer (&constrainer_);

    juce::Rectangle<int> total_bounds = juce::Desktop::getInstance().getDisplays().getTotalBounds (true);
    total_bounds.removeFromBottom (kHeightBuffer);

    float window_size = 1.0f;
    window_size = std::min (window_size, total_bounds.getWidth() / (1.0f * bitklavier::kDefaultWindowWidth));
    window_size = std::min (window_size, total_bounds.getHeight() / (1.0f * bitklavier::kDefaultWindowHeight));
    int width = std::round (window_size * bitklavier::kDefaultWindowWidth);
    int height = std::round (window_size * bitklavier::kDefaultWindowHeight);

    setResizable (false, false);
    setSize (width, height);
    gui_->addKeyListener (commandManager.getKeyMappings());

    // for plugin formats
    if (synth_->samplesLoaded)
    {
        // don't want to see Samples Loading popup....
        gui_->hideLoadingSection();

        // make sure current gallery name is showing
        if (gui_->header_->gallerySelectText)
            gui_->header_->gallerySelectText->setText(getActiveFile().getFileNameWithoutExtension());
    }

    // Bind the construction site to the active piano when the editor opens AFTER a
    // saved-state restore has already completed. In that case no PIANO children are
    // being added (so valueTreeChildAdded won't fire) and finishedSampleLoading has
    // already run without a GUI to inform — so without this call the construction
    // site stays in its default-empty state until the user does something.
    //
    // This MUST be deferred via callAsync, not done synchronously here. The reason is
    // that Cable::connectionAdded routes its visibility setup through executeOnGLThread,
    // which silently no-ops if the OpenGL context hasn't been attached yet. While we're
    // still inside the PluginEditor ctor, the editor hasn't been added to Logic's host
    // window — the GL context isn't attached, the GL job is dropped, the subsequent
    // setVisible(true) callAsync is never queued, and cables stay invisible. Posting
    // via callAsync lets the host add us to its window first, attach the GL context,
    // and only then have CableView create cables that actually become visible.
    //
    // We also re-read getActivePianoValueTree() INSIDE the lambda — capturing it now
    // would be the same stale-tree bug the original code had during in-flight restore.
    juce::WeakReference<SynthGuiInterface> weakThis (this);
    juce::MessageManager::callAsync ([weakThis]()
    {
        if (auto* self = weakThis.get())
        {
            if (self->getSynth()->isSamplesLoading() || self->getSynth()->hasPendingPreset())
                return;
            self->setActivePiano (self->getSynth()->getActivePianoValueTree());
        }
    });
}

PluginEditor::~PluginEditor()
{
    DBG ("PluginEditor destroyed: " + juce::String::toHexString ((juce::uint64) this));
}

//void PluginEditor::paint (juce::Graphics& g)
//{
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//
//    auto area = getLocalBounds();
//    g.setColour (juce::Colours::white);
//    g.setFont (16.0f);
//    auto helloWorld = juce::String ("Hello from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
//    g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);
//}

void PluginEditor::resized()
{
    // layout the positions of your child components here
    auto area = getLocalBounds();
    area.removeFromBottom (50);
    juce::AudioProcessorEditor::resized();
    gui_->setBounds (getLocalBounds());
}

void PluginEditor::setScaleFactor (float newScale)
{
    juce::AudioProcessorEditor::setScaleFactor (newScale);
    gui_->redoBackground();
}

void PluginEditor::updateFullGui()
{
    SynthGuiInterface::updateFullGui();
    updateHostDisplay();
}
