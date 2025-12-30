#include "open_gl_line.h"

// Constructor: Initializes the OpenGlLine
OpenGlLine::OpenGlLine(juce::Component* start_component, juce::Component* end_component, juce::Component* target,
                       Shaders::FragmentShader shader)
        : OpenGlComponent("line"),
          active_(true),
          start_component_(start_component),
          end_component_(end_component),
          target_component_(target),
          fragment_shader_(shader)
{
    // Check that components are valid
    if (start_component_ == nullptr || end_component_ == nullptr || target_component_ == nullptr) {
        DBG("Error: Invalid components supplied to OpenGlLine");
        return;
    }

    // Calculate the clip-space coordinates for the start and end components
    juce::Point<float> startClip = getClipSpaceCoordinates(start_component_, target_component_);
    juce::Point<float> endClip = getClipSpaceCoordinates(end_component_, target_component_);

    // Debug outputs to verify the calculated coordinates
    DBG("------------ OpenGlLine Created ------------");
    DBG("Start Component Clip Space Coordinates: (" + juce::String(startClip.getX()) +
        ", " + juce::String(startClip.getY()) + ")");
    DBG("End Component Clip Space Coordinates: (" + juce::String(endClip.getX()) +
        ", " + juce::String(endClip.getY()) + ")");
    DBG("-------------------------------------------");

    // Store the data in the vertex buffer format (clip-space coordinates)
    data_ = std::make_unique<float[]>(4);
    data_[0] = startClip.getX();
    data_[1] = startClip.getY();
    data_[2] = endClip.getX();
    data_[3] = endClip.getY();

    // Set default indices for drawing the line
    indices_ = std::make_unique<int[]>(2);
    indices_[0] = 0;
    indices_[1] = 1;

    vertex_buffer_ = 0;

    // Debugging additional information
    DBG("Line Vertex Data: X1 = " + juce::String(data_[0]) + ", Y1 = " + juce::String(data_[1]) +
        ", X2 = " + juce::String(data_[2]) + ", Y2 = " + juce::String(data_[3]));
}

// Static function to calculate OpenGL clip-space coordinates
juce::Point<float> OpenGlLine::getClipSpaceCoordinates(juce::Component* component, juce::Component* target) {
    // Ensure that both the component and target are valid
    if (!component || !target) {
        jassertfalse; // Shouldn't happen in a working system
        return {0.0f, 0.0f};
    }

    // Get the position of the component relative to the target
    juce::Point<int> localPosition = component->getBounds().getCentre();

    // Normalize the coordinates to the range [0, 1]
    float normX = static_cast<float>(localPosition.getX()) / static_cast<float>(target->getWidth());
    float normY = static_cast<float>(localPosition.getY()) / static_cast<float>(target->getHeight());

    // Map normalized coordinates to OpenGL clip space [-1, 1]
    float clipX = normX * 2.0f - 1.0f;
    float clipY = 1.0f - normY * 2.0f; // Flip Y-axis for OpenGL clip space

    return {clipX, clipY};
}

// Function to check if the object is initialized
bool OpenGlLine::isInit() {
    return _init;
}

void OpenGlLine::paint(juce::Graphics& g) {
    // Custom painting logic (to be implemented as needed)
}

void OpenGlLine::paintBackground(juce::Graphics& g) {
    // Custom background painting logic (to be implemented as needed)
}

// Function to initialize OpenGL buffers
void OpenGlLine::init(OpenGlWrapper& open_gl) {
    // Set up the points
    open_gl.context.extensions.glGenBuffers(1, &vertex_buffer_);
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);
    jassert(vertex_buffer_ != 0); // Ensure the buffer was successfully created

    GLsizeiptr vert_size = static_cast<GLsizeiptr>(4 * sizeof(float));
    open_gl.context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, vert_size, data_.get(),
                                            juce::gl::GL_STATIC_DRAW);

    // Set up how they connect
    open_gl.context.extensions.glGenBuffers(1, &indices_buffer_);
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

    GLsizeiptr bar_size = static_cast<GLsizeiptr>(2 * sizeof(int));
    open_gl.context.extensions.glBufferData(juce::gl::GL_ELEMENT_ARRAY_BUFFER, bar_size, indices_.get(),
                                            juce::gl::GL_STATIC_DRAW);

    shader_ = open_gl.shaders->getShaderProgram(Shaders::kSimpleLineVertex, fragment_shader_);
    shader_->use();
    if (!shader_->getLastError().isEmpty()) {
        DBG(shader_->getLastError());
        jassertfalse; // Shader compilation/linking failed
    }

    // Unbind the buffer (optional safety)
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);

    float debugData[4];
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);
    juce::gl::glGetBufferSubData(juce::gl::GL_ARRAY_BUFFER, 0, sizeof(debugData), debugData);

    // Format the vertex data into a single debug string
    juce::String vertexBufferData;
    for (int i = 0; i < 4; ++i) {
        vertexBufferData += juce::String(debugData[i]) + ", ";
    }
    DBG("Vertex Buffer Data: " + vertexBufferData);

    // Unbind the buffer (optional safety)
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    _init = true;
}

// Rendering logic for the OpenGL line
void OpenGlLine::render(OpenGlWrapper& open_gl, bool animate) {
    juce::Component* component = (target_component_ == nullptr ? this : static_cast<juce::Component*>(target_component_));
    if (component == nullptr)
        return;
    if (!active_ || (!draw_when_not_visible_ && !component->isVisible()) || component->getTopLevelComponent() == nullptr || !setViewPort(component, open_gl))
        return;
    if (shader_ == nullptr)
        init(open_gl);
    if(dirty_)
    {
        dirty_ = false;
  // Calculate updated clip-space coordinates for start and end components
        auto* startComp = static_cast<juce::Component*>(start_component_);
        auto* endComp = static_cast<juce::Component*>(end_component_);
        auto* targetComp = static_cast<juce::Component*>(target_component_);
        if (startComp == nullptr || endComp == nullptr || targetComp == nullptr)
            return;
        juce::Point<float> startClip = getClipSpaceCoordinates(startComp, targetComp);
        juce::Point<float> endClip = getClipSpaceCoordinates(endComp, targetComp);

        // Update the vertex buffer's data
        GLfloat updatedVertices[] = {
                startClip.getX(), startClip.getY(),  // Start point X, Y
                endClip.getX(), endClip.getY()       // End point X, Y
        };

        //memory is copied to the gpu so this statck allocation should be fine
        // Bind the vertex buffer and update its data
        open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);
        open_gl.context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, 0, sizeof(updatedVertices), updatedVertices);

    }
    shader_->use();
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);
    open_gl.context.extensions.glVertexAttribPointer(0, 2, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 2 * sizeof(float), nullptr);
    open_gl.context.extensions.glEnableVertexAttribArray(0);

    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);
    juce::gl::glDrawElements(juce::gl::GL_LINES, 2, juce::gl::GL_UNSIGNED_INT, nullptr);
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
}

// Resized logic
void OpenGlLine::resized() {
//    OpenGlComponent::resized();
    dirty_ = true;
}