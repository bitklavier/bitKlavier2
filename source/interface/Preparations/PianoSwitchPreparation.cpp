//
// Created by Dan Trueman on 7/22/25.
//

#include "PianoSwitchPreparation.h"
#include "BKitems/BKItem.h"
#include "synth_button.h"
#include "ParametersView.h"

PianoSwitchPreparation::PianoSwitchPreparation(
    juce::ValueTree v, OpenGlWrapper& open_gl, juce::AudioProcessorGraph::NodeID node, SynthGuiInterface* interface):
                                    PreparationSection(
                                        juce::String("pianoswitch"),
                                        v,
                                        open_gl,
                                        node,
                                        *interface->getUndoManager()
                                    )
{
    // Initializes member variable `item` of PreparationSection class
    item = std::make_unique<PianoSwitchItem> ();

    // Calls member function of SynthSection (parent class to PreparationSection)
    addOpenGlComponent (item->getImageComponent(),true);

    _open_gl.context.executeOnGLThread([this](juce::OpenGLContext& context)
        {item->getImageComponent()->init(_open_gl);
        },false);

    addAndMakeVisible (item.get());
    pianoSelectText = std::make_shared<PlainTextComponent>("Piano", "---");
    addOpenGlComponent(pianoSelectText);
    pianoSelector = std::make_unique<juce::ShapeButton>("Selector", juce::Colour(0xff666666),
        juce::Colour(0xffaaaaaa), juce::Colour(0xff888888));
    addAndMakeVisible(pianoSelector.get());
    pianoSelector->setAlwaysOnTop(true);
    pianoSelectText->setAlwaysOnTop(true);
    if (v.getProperty(IDs::selectedPianoName) != "")
        pianoSelectText->setText(v.getProperty(IDs::selectedPianoName));
    pianoSelector->addListener(this);
    pianoSelector->setTriggeredOnMouseDown(true);
    pianoSelector->setShape(juce::Path(), true, true, true);
    currentPianoIndex = v.getProperty(IDs::selectedPianoIndex);
    ///availablePianosMenu = std::make_unique<OpenGLComboBox>("pianos");
//    availablePianosMenu_attachment= std::make_unique<chowdsp::ComboBoxAttachment>(*tuningParams->tuningState.tuningSystem.get(), listeners, *availablePianosMenu, nullptr);
    //addAndMakeVisible(availablePianosMenu.get());
    //addOpenGlComponent(availablePianosMenu->getImageComponent());
}

std::unique_ptr<SynthSection> PianoSwitchPreparation::getPrepPopup()
{
//    if (auto parent = findParentComponentOfClass<SynthGuiInterface>())
//        if (auto* proc = dynamic_cast<PianoSwitchPreparation*> (getProcessor()))
//            return std::make_unique<MidiFilterParametersView> (proc->getState(), proc->getState().params, state.getProperty (IDs::uuid).toString(), open_gl);

/**
 * no popup for piano switch!
 */
    return nullptr;
}
void PianoSwitchPreparation::buttonClicked (juce::Button* clicked_button) {
    if (clicked_button == pianoSelector.get())
    {
        PopupItems options;
        auto interface = findParentComponentOfClass<SynthGuiInterface>();
        auto names = interface->getAllPianoNames();
        for (int i = 0; i < names.size(); i++)
        {
            options.addItem (i, names[i]);
        }
        juce::Point<int> position(pianoSelector->getX(), pianoSelector->getBottom());
        showPopupSelector(this, position, options, [=](int selection, int) {
            pianoSelectText->setText(names[selection]);
            state.setProperty(IDs::selectedPianoIndex, selection,nullptr);
            state.setProperty(IDs::selectedPianoName,juce::String(names[selection]),nullptr);
//            for (auto vt: gallery) {
//                if (vt.hasType(IDs::PIANO)) {
//                    vt.setProperty(IDs::isActive, 0, nullptr);
//                }
//            }
//            for (auto vt: gallery) {
//                if (vt.hasType(IDs::PIANO) && vt.getProperty(IDs::name) == pianoSelectText->getText()) {
//                    vt.setProperty(IDs::isActive, 1, nullptr);
//                }
//            }
//            auto interface = findParentComponentOfClass<SynthGuiInterface>();
//            interface->allNotesOff();
            resized();
        });
    }
}

void PianoSwitchPreparation::resized()
{
    PreparationSection::resized();
    juce::Colour body_text = findColour(Skin::kBodyText, true);
    pianoSelectText->setColor(body_text);
    float label_text_height = findValue(Skin::kLabelHeight);
    pianoSelectText->setTextSize(2*label_text_height);
    juce::Rectangle<int> area (getLocalBounds());
    int comboboxheight = findValue(Skin::kComboMenuHeight);
    auto newarea = area.removeFromBottom(2*comboboxheight);
    pianoSelector->setBounds(area.removeFromBottom(comboboxheight));
    pianoSelectText->setBounds(pianoSelector->getBounds());

}

