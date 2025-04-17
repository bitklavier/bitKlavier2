//
// Created by Davis Polito on 4/15/25.
//

#ifndef APPLICATIONCOMMANDHANDLER_H
#define APPLICATIONCOMMANDHANDLER_H
#include <juce_gui_basics/juce_gui_basics.h>

class ApplicationCommandHandler : public juce::ApplicationCommandTarget {
public:
    ApplicationCommandHandler() = default;

    // Define your command IDs
    enum CommandIDs {
        undo = 0x2000,
        redo,
        save,
        load
    };

    void getAllCommands(juce::Array<juce::CommandID> &commands) override {
        commands.addArray({undo, redo, save, load});
    }

    void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo &info) override {
        switch (id) {
            case undo:
                info.setInfo("Undo", "Undo last action", "Edit", 0);
                info.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
                break;
            case redo:
                info.setInfo("Redo", "Redo last action", "Edit", 0);
                info.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
                break;
            case save:
                info.setInfo("Save", "Save Current Preset", "File", 0);
                info.addDefaultKeypress('s', juce::ModifierKeys::commandModifier);
                break;
            case load:
                info.setInfo("Load", "Load New Preset", "File", 0);
                info.addDefaultKeypress('l', juce::ModifierKeys::commandModifier);
                break;
        }
    }

    bool perform(const InvocationInfo &info) override {
        switch (info.commandID) {
            case undo:
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Undo", "Undo triggered");
                return true;

            case redo:
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Redo", "Redo triggered");
                return true;

            case save:
                //juce::JUCEApplication::getInstance()->systemRequestedQuit();
                return true;
            case load:
                //juce::JUCEApplication::getInstance()->systemRequestedQuit();
            return true;

            default:
                return false;
        }
    }

    juce::ApplicationCommandTarget *getNextCommandTarget() override {
        return nullptr;
    }
};
#endif //APPLICATIONCOMMANDHANDLER_H
