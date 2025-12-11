//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationSection.h"
#include "modulation_button.h"
#include "modulation_manager.h"
ModulationSection::ModulationSection( const juce::ValueTree &v,SynthSection* editor, juce::UndoManager &um) : SynthSection(editor->getName()), state(v), _view(editor), undo(um),
mod_button(new ModulationButton(editor->getComponentID()+"_mod"))
{
    setComponentID(v.getParent().getProperty(IDs::uuid).toString());
    addModulationButton(mod_button);
    addAndMakeVisible(mod_button.get());
    mod_button->setAlwaysOnTop(true);
    addSubSection(_view.get());
    mod_button->setStateModulation(v.getProperty(IDs::isState));
    exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());
}

ModulationSection::~ModulationSection()
{
    _view.reset();
}

void ModulationSection::resized()
{
    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int section_height = getKnobSectionHeight();

    juce::Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
    _view->setBounds(getLocalBounds());
    mod_button->setBounds(0, 0,40,40);
    exit_button_->setBounds(_view->getWidth()-40, 0,40,40);
    int knob_y2 =0;
    SynthSection::resized();
}

void ModulationSection::addModButtonListener(ModulationManager* manager)
{
    mod_button->addListener(manager);
}

//void ModulationSection::setParametersViewEditor (bitklavier::ParametersViewEditor&& editor)
//{
//   _view_editor = editor;
//   addSubSection(_view);
//
//}

void ModulationSection::buttonClicked(juce::Button* button)
{
    if (button == exit_button_.get()) {
        this->setVisible(false);
        //DBG("state " state.getParent())
        undo.beginNewTransaction();
        state.getParent().removeChild(state,&undo);
    }
}