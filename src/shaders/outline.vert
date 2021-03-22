#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 2) in vec3 a_normal;

out vec3 N;

void main() {
    // Calculate modelview matrix
    mat4 mv = u_view * u_model;

    N = normalize(mat3(mv) * a_normal);

    mat4 MVP = u_projection * u_view * u_model;
    gl_Position = MVP * a_position;
}