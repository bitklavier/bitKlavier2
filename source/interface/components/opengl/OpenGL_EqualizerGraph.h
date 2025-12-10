//
// Created by Myra Norton on 11/27/25.
//

#ifndef OPENGL_EQUALIZERGRAPH_H
#define OPENGL_EQUALIZERGRAPH_H

#include "../BKComponents/BKEqualizerGraph.h"

class OpenGL_EqualizerGraph : public OpenGlAutoImageComponent<BKEqualizerGraph>, BKEqualizerGraph::Listener
{
    public:
    OpenGL_EqualizerGraph (EQProcessor &eqProcessor, chowdsp::ParameterListeners &listeners) :
        OpenGlAutoImageComponent<BKEqualizerGraph>(eqProcessor)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    void resized() override {
        OpenGlAutoImageComponent<BKEqualizerGraph>::resized();
        redoImage();
    }

    void paintBackground(juce::Graphics& g) {
        OpenGlAutoImageComponent<BKEqualizerGraph>::paint(g);
        redoImage();
    }
};


#endif //OPENGL_EQUALIZERGRAPH_H
