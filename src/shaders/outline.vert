#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// Vertex inputs (attributes from vertex buffers)
// layout(location = 0) in vec4 a_position;
layout(location = 2) in vec3 a_normal;
layout(location = 1) in vec2 a_texcoord;

out vec3 N;
out vec2 texcoord; // interpolated texture coordinate

void main() {
    // Calculate modelview matrix
    mat4 mv = u_view * u_model;

    N = normalize(mat3(mv) * a_normal);
    texcoord = a_texcoord;

    // gl_Position = u_projection * u_view * u_model * a_position;
}