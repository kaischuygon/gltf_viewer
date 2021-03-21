#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

// uniform float u_specularPower;
uniform vec3 u_lightPosition; // position of light source

// Vertex inputs (attributes from vertex buffers)
layout(location = 0) in vec4 a_position;
layout(location = 2) in vec3 a_normal;
layout(location = 1) in vec2 a_texcoord; // texture coordinate of the current vertex

// Vertex shader outputs
// out vec3 v_color;
out vec3 N;
out vec3 L;
out vec3 V;

out vec2 texcoord; // interpolated texture coordinate

void main() {
    // Calculate modelview matrix
    mat4 mv = u_view * u_model;

    // Transform the vertex position to view space (eye coordinates)
    vec3 positionEye = vec3(mv * a_position);
    positionEye = normalize(positionEye);

    // Calculate the view-space normal
    N = normalize(mat3(mv) * a_normal);

    // Calculate the view-space light direction
    L = normalize(u_lightPosition - positionEye);

    // view vector
    V = normalize(-positionEye);

    texcoord = a_texcoord;

    mat4 MVP = u_projection * u_view * u_model;
    gl_Position = MVP * a_position;
}
