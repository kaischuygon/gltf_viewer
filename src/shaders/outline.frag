#version 330
#extension GL_ARB_explicit_attrib_location : require

in vec3 N; // view space normal vector
in vec2 texcoord; // interpolated texture coordinate
layout (location = 0) out vec3 rgb_normal;
layout (location = 1) out vec3 depth;

void main() {
    rgb_normal = N * 0.5 + 0.5;
    depth = vec3(gl_FragCoord.z);
}