// Utility functions for loading shaders, textures, etc.
//
// Author: Fredrik Nysjo (2021)
//

#pragma once

#include <GL/gl3w.h>

#include <string>
#include <vector>

namespace cg {

// Helper function for loading values from environment variables
std::string get_env_var(const std::string &name);

// This function should be called at the beginning of each frame and whenever
// we want to restore the OpenGL pipeline to its default state. Feel free to
// change or extend this function if necessary!
void reset_gl_render_state();

GLuint load_shader_program(const std::string &vertexShaderFilename,
                           const std::string &fragmentShaderFilename);

GLuint load_texture_2d(const std::string &filename);

GLuint load_cubemap(const std::string &filename);

GLuint load_cubemap_prefilterd(const std::string &filename);

GLuint create_depth_texture(int width=512, int height=512);

GLuint create_depth_framebuffer(GLuint depth_texture);

}  // namespace cg
