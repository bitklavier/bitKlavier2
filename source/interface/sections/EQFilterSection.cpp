//
// Created by Myra Norton on 11/25/25.
//

#include "EQFilterSection.h"

EQPeakFilterSection::EQPeakFilterSection (
    juce::String name,
    EQPeakFilterParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent)  : SynthSection(name)
{
    setComponentID(parent.getComponentID());
    setName("eqpeakfilter");
    setLookAndFeel(DefaultLookAndFeel::instance());

    auto peakImage = juce::ImageCache::getFromMemory(
    BinaryData::peak_png,
    BinaryData::peak_pngSize);

    peak_filter_button = std::make_unique<filter_button>(peakImage);
    filter_active_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.filterActive,listeners,*peak_filter_button,nullptr);
    addOpenGlComponent (peak_filter_button->getImageComponent());
    addAndMakeVisible(peak_filter_button.get());

    freq_knob = std::make_unique<SynthSlider>(params.filterFreq->paramID, params.filterFreq->getModParam());
    addSlider(freq_knob.get());
    freq_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freq_knob->setPopupPlacement(juce::BubbleComponent::below);
    freq_knob->setShowPopupOnHover(true);
    freq_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.filterFreq, listeners, *freq_knob, nullptr);
    freq_knob->addAttachment(freq_knob_attachment.get());

    gain_knob = std::make_unique<SynthSlider>(params.filterGain->paramID, params.filterGain->getModParam());
    addSlider(gain_knob.get());
    gain_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gain_knob->setPopupPlacement(juce::BubbleComponent::below);
    gain_knob->setShowPopupOnHover(true);
    gain_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.filterGain, listeners, *gain_knob, nullptr);
    gain_knob->addAttachment(gain_knob_attachment.get());

    q_knob = std::make_unique<SynthSlider>(params.filterQ->paramID, params.filterQ->getModParam());
    addSlider(q_knob.get());
    q_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    q_knob->setPopupPlacement(juce::BubbleComponent::below);
    q_knob->setShowPopupOnHover(true);
    q_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.filterQ, listeners, *q_knob, nullptr);
    q_knob->addAttachment(q_knob_attachment.get());

    sectionBorder.setName("eqfilterborder");
    addAndMakeVisible(sectionBorder);
}

void EQPeakFilterSection::paintBackground(juce::Graphics& g) {
    setLabelFont(g);
    drawLabelForComponent (g, "freq", freq_knob.get());
    drawLabelForComponent (g, "gain", gain_knob.get());
    drawLabelForComponent (g, "q", q_knob.get());
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
    sectionBorder.paint(g);
}

void EQPeakFilterSection::resized()
{
    juce::Rectangle<int> bounds (getLocalBounds());
    sectionBorder.setBounds(bounds);

    int smallpadding = findValue(Skin::kPadding);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int knobsectionwidth = 50;

    bounds.removeFromTop (smallpadding*3);
    peak_filter_button->setBounds (bounds.removeFromTop(knobsectionheight*1.2).withWidth (100).withX(bounds.getX() + (bounds.getWidth() - 100)/2));
    bounds = bounds.withWidth(knobsectionwidth)
           .withX(bounds.getX() + (bounds.getWidth() - knobsectionwidth)/2);
    freq_knob->setBounds(bounds.removeFromTop(knobsectionheight *.85));
    bounds.removeFromTop (smallpadding);
    gain_knob->setBounds(bounds.removeFromTop(knobsectionheight *.85));
    bounds.removeFromTop (smallpadding);
    q_knob->setBounds(bounds.removeFromTop(knobsectionheight *.85));

    SynthSection::resized();
}

EQCutFilterSection::EQCutFilterSection (
    juce::String name,
    EQCutFilterParams &params,
    chowdsp::ParameterListeners &listeners,
    SynthSection &parent)  : SynthSection(name)
{
    setComponentID(parent.getComponentID());
    setName("eqcutfilter");
    setLookAndFeel(DefaultLookAndFeel::instance());

    auto locutImage = juce::ImageCache::getFromMemory(
        BinaryData::locut_png,
        BinaryData::locut_pngSize);
    auto hicutImage = juce::ImageCache::getFromMemory(
        BinaryData::hi_cut_png,
        BinaryData::hi_cut_pngSize);
    if (params.idPrepend == "loCut")
    {
        cut_filter_button = std::make_unique<filter_button>(locutImage);
    }
    else cut_filter_button = std::make_unique<filter_button>(hicutImage);

    filter_active_attachment = std::make_unique<chowdsp::ButtonAttachment>(params.filterActive,listeners,*cut_filter_button,nullptr);
    addOpenGlComponent (cut_filter_button->getImageComponent());
    addAndMakeVisible(cut_filter_button.get());

    freq_knob = std::make_unique<SynthSlider>(params.filterFreq->paramID, params.filterFreq->getModParam());
    addSlider(freq_knob.get());
    freq_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freq_knob->setPopupPlacement(juce::BubbleComponent::below);
    freq_knob->setShowPopupOnHover(true);
    freq_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.filterFreq, listeners, *freq_knob, nullptr);
    freq_knob->addAttachment(freq_knob_attachment.get());

    slope_knob = std::make_unique<SynthSlider>(params.filterSlope->paramID, params.filterSlope->getModParam());
    addSlider(slope_knob.get());
    slope_knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slope_knob->setPopupPlacement(juce::BubbleComponent::below);
    slope_knob->setShowPopupOnHover(true);
    slope_knob_attachment = std::make_unique<chowdsp::SliderAttachment>(params.filterSlope, listeners, *slope_knob, nullptr);
    slope_knob->addAttachment(slope_knob_attachment.get());

    sectionBorder.setName("eqfilterborder");
    addAndMakeVisible(sectionBorder);
}

void EQCutFilterSection::paintBackground(juce::Graphics& g) {
    setLabelFont(g);
    drawLabelForComponent (g, "freq", freq_knob.get());
    drawLabelForComponent (g, "slope", slope_knob.get());
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
    sectionBorder.paint(g);
}

void EQCutFilterSection::resized()
{
    juce::Rectangle<int> bounds (getLocalBounds());
    sectionBorder.setBounds(bounds);

    int smallpadding = findValue(Skin::kPadding);
    int knobsectionheight = findValue(Skin::kKnobSectionHeight);
    int knobsectionwidth = 50;

    bounds.removeFromTop (smallpadding*3);
    cut_filter_button->setBounds (bounds.removeFromTop(knobsectionheight*1.2).withWidth (100).withX(bounds.getX() + (bounds.getWidth() - 100)/2));

    bounds = bounds.withWidth(knobsectionwidth)
           .withX(bounds.getX() + (bounds.getWidth() - knobsectionwidth)/2);
    freq_knob->setBounds(bounds.removeFromTop(knobsectionheight *.85));
    bounds.removeFromTop (smallpadding);
    slope_knob->setBounds(bounds.removeFromTop(knobsectionheight *.85));
    SynthSection::resized();
}