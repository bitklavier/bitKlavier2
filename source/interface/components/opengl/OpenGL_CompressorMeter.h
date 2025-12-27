//
// Created by Myra Norton on 12/22/25.
//

#ifndef OPENGL_COMPRESSORMETER_H
#define OPENGL_COMPRESSORMETER_H
#include "../BKComponents/BKCompressorMeter.h"

class OpenGL_CompressorMeter : public OpenGlAutoImageComponent<BKCompressorMeter>/*, BKCompressorMeter::Listener*/
{
public:
    OpenGL_CompressorMeter (CompressorParams *params, chowdsp::ParameterListeners &listeners) :
        OpenGlAutoImageComponent<BKCompressorMeter>(params)
    {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    void resized() override {
        OpenGlAutoImageComponent<BKCompressorMeter>::resized();
        redoImage();
    }

    void paintBackground(juce::Graphics& g) {
        OpenGlAutoImageComponent<BKCompressorMeter>::paint(g);
        redoImage();
    }
};

#endif //OPENGL_COMPRESSORMETER_H
