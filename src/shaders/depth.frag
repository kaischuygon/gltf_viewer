#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
// ...

// Fragment shader inputs
// ...

// Fragment shader outputs
out vec4 frag_color;

void main()
{
    // Write depth to fragment color (for debug drawing)
    frag_color = vec4(vec3(gl_FragCoord.z), 1.0);
}