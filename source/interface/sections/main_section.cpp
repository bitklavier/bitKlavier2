//
// Created by Davis Polito on 2/1/24.
//

#include "main_section.h"
#include "synth_section.h"
#include "ConstructionSite.h"

MainSection::MainSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper & open_gl, SynthGuiData* data, juce::ApplicationCommandManager& _manager)
    : SynthSection("main_section"), um(um), commandManager (_manager)
{

    constructionSite_ = std::make_unique<ConstructionSite>(v.getChildWithName(IDs::PIANO), um, open_gl, data, commandManager);
    addMouseListener(constructionSite_.get(), true);
    constructionSite_->view = &constructionPort;
    // constructionSite_->initializeCommandManager();
    addSubSection(constructionSite_.get(), true);

    setSkinOverride(Skin::kNone);
}

MainSection::~MainSection()
{

}

void MainSection::removeAllGuiListeners()
{
   constructionSite_->removeAllGuiListeners();
}
void MainSection::paintBackground(juce::Graphics& g)
{
    paintContainer(g);

    g.setColour(findColour(Skin::kBody, true));
    paintChildrenBackgrounds(g);

    g.saveState();

    g.restoreState();


}

void MainSection::resized()
{
    constructionSite_->setColour(Skin::kBody,juce::Colours::antiquewhite);
    int height = getHeight();
    int width = getWidth();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int large_padding = findValue(Skin::kLargePadding);
    constructionSite_->setBounds(large_padding, 0,  width, height);
    //constructionPort.setBounds(large_padding, 0,getDisplayScale()* width, getDisplayScale() * height);
    //constructionPort.setBounds(large_padding, 0,width, height);
    SynthSection::resized();
}