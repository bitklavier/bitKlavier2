//
// Created by Davis Polito on 2/1/24.
//

#include "main_section.h"
#include "synth_section.h"
#include "ConstructionSite.h"

MainSection::MainSection(juce::ValueTree v, juce::UndoManager &um, OpenGlWrapper & open_gl, SynthGuiData* data) : SynthSection("main_section"), um(um)
{
    juce::ValueTree t{IDs::PREPARATIONS};
    v.appendChild(t, nullptr);
    juce::ValueTree t2{IDs::CONNECTIONS};
    juce::ValueTree t3{IDs::MODCONNECTIONS};
    v.appendChild(t2, nullptr);
    v.appendChild(t3, nullptr);
    constructionSite_ = std::make_unique<ConstructionSite>(t, um, open_gl, data);
    addMouseListener(constructionSite_.get(), true);
    constructionSite_->view = &constructionPort;
    addSubSection(constructionSite_.get(), true);

    setSkinOverride(Skin::kNone);
}

MainSection::~MainSection()
{

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