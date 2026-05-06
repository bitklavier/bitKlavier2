// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Created by Myra Norton on 11/27/25.
//

#ifndef OPENGL_EQUALIZERGRAPH_H
#define OPENGL_EQUALIZERGRAPH_H

#include "../BKComponents/BKEqualizerGraph.h"

class OpenGL_EqualizerGraph : public OpenGlAutoImageComponent<BKEqualizerGraph>, BKEqualizerGraph::Listener
{
    public:
    OpenGL_EqualizerGraph (EQParams *params, chowdsp::ParameterListeners &listeners) :
        OpenGlAutoImageComponent<BKEqualizerGraph>(params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    void resized() override {
        OpenGlAutoImageComponent<BKEqualizerGraph>::resized();
        // redoImage();
    }

    void paintBackground(juce::Graphics& g) {
        OpenGlAutoImageComponent<BKEqualizerGraph>::paint(g);
        // redoImage();
    }
};


#endif //OPENGL_EQUALIZERGRAPH_H
