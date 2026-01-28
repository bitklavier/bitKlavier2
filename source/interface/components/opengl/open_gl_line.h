#ifndef BITKLAVIER2_OPEN_GL_INSTANCED_LINES_H
#define BITKLAVIER2_OPEN_GL_INSTANCED_LINES_H

#include "open_gl_component.h"

class OpenGlLine : public OpenGlComponent
{
public:
    OpenGlLine(juce::Component* start_component, juce::Component* end_component, juce::Component* target,
               Shaders::FragmentShader shader = Shaders::kBlackFragment);

    void paint(juce::Graphics& g) override;
    void paintBackground(juce::Graphics& g) override;
    void init(OpenGlWrapper& open_gl);
    void render(OpenGlWrapper& open_gl, bool animate);
    void resized() override;
    bool isInit() override;

    // Static function to calculate OpenGL clip-space coordinates
    static juce::Point<float> getClipSpaceCoordinates(juce::Component* component, juce::Component* target);

private:
    bool _init = false;
    bool dirty_;
    bool active_;
    bool draw_when_not_visible_ = false;

    Shaders::FragmentShader fragment_shader_;
    juce::OpenGLShaderProgram* shader_;
    std::unique_ptr<int[]> indices_;
    GLuint vertex_buffer_;
    GLuint indices_buffer_;
    GLuint vao_ { 0 };
    std::unique_ptr<float[]> data_;

    juce::Component::SafePointer<juce::Component> target_component_;
    juce::Component::SafePointer<juce::Component> scissor_component_;
    juce::Component::SafePointer<juce::Component> start_component_;
    juce::Component::SafePointer<juce::Component> end_component_;
};

#endif // BITKLAVIER2_OPEN_GL_INSTANCED_LINES_H