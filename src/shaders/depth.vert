#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform mat4 u_model;
uniform mat4 u_lightSpace;
// ...

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
// ...

// Vertex shader outputs
// ...

void main()
{
    gl_Position = u_lightSpace * u_model * a_position;
}