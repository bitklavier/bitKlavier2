/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#pragma once



inline juce::String getFormatSuffix (const juce::AudioProcessor* plugin)
{
    const auto format = [plugin]()
    {
        if (auto* instance = dynamic_cast<const juce::AudioPluginInstance*> (plugin))
            return instance->getPluginDescription().pluginFormatName;

        return juce::String();
    }();

    return format.isNotEmpty() ? (" (" + format + ")") : format;
}

/**
    A window that shows a log of parameter change messages sent by the plugin.
*/
class PluginDebugWindow final : public juce::AudioProcessorEditor,
                                public juce::AudioProcessorParameter::Listener,
                                public juce::ListBoxModel,
                                public juce::AsyncUpdater
{
public:
    PluginDebugWindow (juce::AudioProcessor& proc)
        : AudioProcessorEditor (proc), audioProc (proc)
    {
        setSize (500, 200);
        addAndMakeVisible (list);

        for (auto* p : audioProc.getParameters())
            p->addListener (this);

        log.add ("Parameter debug log started");
    }

    ~PluginDebugWindow() override
    {
        for (auto* p : audioProc.getParameters())
            p->removeListener (this);
    }

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        auto* param = audioProc.getParameters()[parameterIndex];
        auto value = param->getCurrentValueAsText().quoted() + " (" + juce::String (newValue, 4) + ")";

        appendToLog ("parameter change", *param, value);
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override
    {
        auto* param = audioProc.getParameters()[parameterIndex];
        appendToLog ("gesture", *param, gestureIsStarting ? "start" : "end");
    }

private:
    void appendToLog (juce::StringRef action, juce::AudioProcessorParameter& param, juce::StringRef value)
    {
        juce::String entry (action + " " + param.getName (30).quoted() + " [" + juce::String (param.getParameterIndex()) + "]: " + value);

        {
            juce::ScopedLock lock (pendingLogLock);
            pendingLogEntries.add (entry);
        }

        triggerAsyncUpdate();
    }

    void resized() override
    {
        list.setBounds (getLocalBounds());
    }

    int getNumRows() override
    {
        return log.size();
    }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool) override
    {
        g.setColour (getLookAndFeel().findColour (juce::TextEditor::textColourId));

        if (juce::isPositiveAndBelow (rowNumber, log.size()))
            g.drawText (log[rowNumber], juce::Rectangle<int> { 0, 0, width, height }, juce::Justification::left, true);
    }

    void handleAsyncUpdate() override
    {
        if (log.size() > logSizeTrimThreshold)
            log.removeRange (0, log.size() - maxLogSize);

        {
            juce::ScopedLock lock (pendingLogLock);
            log.addArray (pendingLogEntries);
            pendingLogEntries.clear();
        }

        list.updateContent();
        list.scrollToEnsureRowIsOnscreen (log.size() - 1);
    }

    constexpr static const int maxLogSize = 300;
    constexpr static const int logSizeTrimThreshold = 400;

    juce::ListBox list { "Log", this };

    juce::StringArray log;
    juce::StringArray pendingLogEntries;
    juce::CriticalSection pendingLogLock;

    juce::AudioProcessor& audioProc;
};

//==============================================================================
/**
    A desktop window containing a plugin's GUI.
*/
class PluginWindow final : public juce::DocumentWindow
{
public:
    enum class Type
    {
        normal = 0,
        generic,
        programs,
        audioIO,
        debug,
        araHost,
        numTypes
    };

    PluginWindow (juce::AudioProcessorGraph::Node* n, Type t, juce::OwnedArray<PluginWindow>& windowList)
        : DocumentWindow (n->getProcessor()->getName() + getFormatSuffix (n->getProcessor()),
                          juce::LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                          DocumentWindow::minimiseButton | DocumentWindow::closeButton),
          activeWindowList (windowList),
          node (n), type (t)
    {
        setSize (400, 300);

        if (auto* ui = createProcessorEditor (*node->getProcessor(), type))
        {
            setContentOwned (ui, true);
            setResizable (ui->isResizable(), false);
        }

        setConstrainer (&constrainer);

       #if JUCE_IOS || JUCE_ANDROID
        const auto screenBounds = Desktop::getInstance().getDisplays().getTotalBounds (true).toFloat();
        const auto scaleFactor = jmin ((screenBounds.getWidth()  - 50.0f) / (float) getWidth(),
                                       (screenBounds.getHeight() - 50.0f) / (float) getHeight());

        if (scaleFactor < 1.0f)
        {
            setSize ((int) (scaleFactor * (float) getWidth()),
                     (int) (scaleFactor * (float) getHeight()));
        }

        setTopLeftPosition (20, 20);
       #else
        setTopLeftPosition (node->properties.getWithDefault (getLastXProp (type), juce::Random::getSystemRandom().nextInt (500)),
                            node->properties.getWithDefault (getLastYProp (type), juce::Random::getSystemRandom().nextInt (500)));
       #endif

        node->properties.set (getOpenProp (type), true);

        setVisible (true);
    }

    ~PluginWindow() override
    {
        clearContentComponent();
    }

    void moved() override
    {
        node->properties.set (getLastXProp (type), getX());
        node->properties.set (getLastYProp (type), getY());
    }

    void closeButtonPressed() override
    {
        node->properties.set (getOpenProp (type), false);
        activeWindowList.removeObject (this);
    }

    static juce::String getLastXProp (Type type)    { return "uiLastX_" + getTypeName (type); }
    static juce::String getLastYProp (Type type)    { return "uiLastY_" + getTypeName (type); }
    static juce::String getOpenProp  (Type type)    { return "uiopen_"  + getTypeName (type); }

    juce::OwnedArray<PluginWindow>& activeWindowList;
    const juce::AudioProcessorGraph::Node::Ptr node;
    const Type type;

    juce::BorderSize<int> getBorderThickness() const override
    {
       #if JUCE_IOS || JUCE_ANDROID
        const int border = 10;
        return { border, border, border, border };
       #else
        return DocumentWindow::getBorderThickness();
       #endif
    }

private:
    class DecoratorConstrainer final : public juce::BorderedComponentBoundsConstrainer
    {
    public:
        explicit DecoratorConstrainer (DocumentWindow& windowIn)
            : window (windowIn) {}

        ComponentBoundsConstrainer* getWrappedConstrainer() const override
        {
            auto* editor = dynamic_cast<juce::AudioProcessorEditor*> (window.getContentComponent());
            return editor != nullptr ? editor->getConstrainer() : nullptr;
        }

        juce::BorderSize<int> getAdditionalBorder() const override
        {
            const auto nativeFrame = [&]() -> juce::BorderSize<int>
            {
                if (auto* peer = window.getPeer())
                    if (const auto frameSize = peer->getFrameSizeIfPresent())
                        return *frameSize;

                return {};
            }();

            return nativeFrame.addedTo (window.getContentComponentBorder());
        }

    private:
        DocumentWindow& window;
    };

    DecoratorConstrainer constrainer { *this };

    float getDesktopScaleFactor() const override     { return 1.0f; }

    static juce::AudioProcessorEditor* createProcessorEditor (juce::AudioProcessor& processor,
                                                        PluginWindow::Type type)
    {
        if (type == PluginWindow::Type::normal)
        {
            if (processor.hasEditor())
                if (auto* ui = processor.createEditorIfNeeded())
                    return ui;

            type = PluginWindow::Type::generic;
        }

        if (type == PluginWindow::Type::araHost)
        {
           #if JUCE_PLUGINHOST_ARA && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX)
            if (auto* araPluginInstanceWrapper = dynamic_cast<ARAPluginInstanceWrapper*> (&processor))
                if (auto* ui = araPluginInstanceWrapper->createARAHostEditor())
                    return ui;
           #endif
            return {};
        }

        if (type == PluginWindow::Type::generic)
        {
            auto* result = new juce::GenericAudioProcessorEditor (processor);
            result->setResizeLimits (200, 300, 1'000, 10'000);
            return result;
        }

        if (type == PluginWindow::Type::programs)
            return new ProgramAudioProcessorEditor (processor);

        if (type == PluginWindow::Type::debug)
            return new PluginDebugWindow (processor);

        jassertfalse;
        return {};
    }

    static juce::String getTypeName (Type type)
    {
        switch (type)
        {
            case Type::normal:     return "Normal";
            case Type::generic:    return "Generic";
            case Type::programs:   return "Programs";
            case Type::audioIO:    return "IO";
            case Type::debug:      return "Debug";
            case Type::araHost:    return "ARAHost";
            case Type::numTypes:
            default:               return {};
        }
    }

    //==============================================================================
    struct ProgramAudioProcessorEditor final : public juce::AudioProcessorEditor
    {
        explicit ProgramAudioProcessorEditor (juce::AudioProcessor& p)
            : juce::AudioProcessorEditor (p)
        {
            setOpaque (true);

            addAndMakeVisible (listBox);
            listBox.updateContent();

            const auto rowHeight = listBox.getRowHeight();

            setSize (400, juce::jlimit (rowHeight, 400, p.getNumPrograms() * rowHeight));
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colours::grey);
        }

        void resized() override
        {
            listBox.setBounds (getLocalBounds());
        }

    private:
        class Model : public juce::ListBoxModel
        {
        public:
            Model (Component& o, juce::AudioProcessor& p)
                : owner (o), proc (p) {}

            int getNumRows() override
            {
                return proc.getNumPrograms();
            }

            void paintListBoxItem (int rowNumber,
                                   juce::Graphics& g,
                                   int width,
                                   int height,
                                   bool rowIsSelected) override
            {
                const auto textColour = owner.findColour (juce::ListBox::textColourId);

                if (rowIsSelected)
                {
                    const auto defaultColour = owner.findColour (juce::ListBox::backgroundColourId);
                    const auto c = rowIsSelected ? defaultColour.interpolatedWith (textColour, 0.5f)
                                                 : defaultColour;

                    g.fillAll (c);
                }

                g.setColour (textColour);
                g.drawText (proc.getProgramName (rowNumber),
                            juce::Rectangle<int> { width, height }.reduced (2),
                            juce::Justification::left,
                            true);
            }

            void selectedRowsChanged (int row) override
            {
                if (0 <= row)
                    proc.setCurrentProgram (row);
            }

        private:
            Component& owner;
            juce::AudioProcessor& proc;
        };

        Model model { *this, *getAudioProcessor() };
        juce::ListBox listBox { "Programs", &model };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProgramAudioProcessorEditor)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginWindow)
};
