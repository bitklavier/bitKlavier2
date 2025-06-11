//
// Created by Davis Polito on 4/21/25.
//
#include "ApplicationCommandHandler.h"
#include "synth_gui_interface.h"
#include "PluginList.h"
ApplicationCommandHandler::ApplicationCommandHandler(SynthGuiInterface* gui) : juce::ApplicationCommandTarget(), parent(gui)
    {

        }
 bool ApplicationCommandHandler::perform(const InvocationInfo & info) {
    {
        switch (info.commandID) {
            case undo:
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Undo", "Undo triggered");
            return true;

            case redo:
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Redo", "Redo triggered");
            return true;

            case save: {
                parent->openSaveDialog();
                return true;
            }

            case load:
                parent->openLoadDialog();
                return true;

            case CommandIDs::showPluginListEditor:
                if (parent->pluginListWindow == nullptr)
                    parent->pluginListWindow.reset (new SynthGuiInterface::PluginListWindow (*parent, *parent->userPreferences, parent->userPreferences->userPreferences->formatManager));

                parent->pluginListWindow->toFront (true);
                return true;

            default:
                return false;
        }
    }
  }