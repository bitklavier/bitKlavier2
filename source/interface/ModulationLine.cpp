//
// Created by Davis Polito on 2/5/25.
//
#include "ModulationLineView.h"
#include "ModulationLine.h"
#include "ConstructionSite.h"
#include "open_gl_line.h"
ModulationLine::ModulationLine(ConstructionSite *site, ModulationLineView *lineView,
                               const juce::ValueTree &v) : site(site), lineView(lineView), state(v) {

    line = std::make_shared<OpenGlLine>(site->getComponentForPlugin(juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(state.getProperty(IDs::src))),
                                        site->getComponentForPlugin(juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(state.getProperty(IDs::dest))),
                                        lineView);
    src_id.referTo(state, IDs::src, nullptr);
    dest_id.referTo(state, IDs::dest, nullptr);
}
void ModulationLine::getPoints (juce::Point<int>& p1, juce::Point<int>& p2) const
{
    p1 = lastInputPos;
    p2 = lastOutputPos;

    if (auto* src = site->getComponentForPlugin ((juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(state.getProperty(IDs::src)))))
        p1 = src->getBounds().getCentre();

    if (auto* dest = site->getComponentForPlugin ((juce::VariantConverter<juce::AudioProcessorGraph::NodeID>::fromVar(state.getProperty(IDs::dest)))))
        p2 = dest->getBounds().getCentre();
}


void ModulationLine::update()
{

        juce::Point<int> p1, p2;
        getPoints(p1, p2);

        if (lastInputPos != p1 || lastOutputPos != p2)
            line->resized();
//        resizeToFit();

}