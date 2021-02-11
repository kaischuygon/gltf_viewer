// Utility functions for loading shaders, textures, etc.
//
// Author: Fredrik Nysjo (2021)
//

#include "cg_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace cg {

static std::string read_shader_source(const std::string &filename)
{
    std::ifstream file(filename);
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

static void show_shader_info_log(GLuint shader)
{
    GLint infoLogLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> infoLog(infoLogLength);
    glGetShaderInfoLog(shader, infoLogLength, &infoLogLength, &infoLog[0]);
    std::string infoLogStr(infoLog.begin(), infoLog.end());
    std::cerr << infoLogStr << std::endl;
}

static void show_program_info_log(GLuint program)
{
    GLint infoLogLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> infoLog(infoLogLength);
    glGetProgramInfoLog(program, infoLogLength, &infoLogLength, &infoLog[0]);
    std::string infoLogStr(infoLog.begin(), infoLog.end());
    std::cerr << infoLogStr << std::endl;
}

std::string get_env_var(const std::string &name)
{
    char const *value = std::getenv(name.c_str());
    if (value == nullptr) {
        return std::string();
    } else {
        return std::string(value);
    }
}

void reset_gl_render_state()
{
    // See e.g. http://docs.gl for information about each state
    //
    // Feel free to extend this function with defaults for other states!

    // Rasterization states
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(1.0f);

    // Multisample states
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // Depth and stencil states
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);
    glDisable(GL_STENCIL_TEST);

    // Color and blend states
    glDisable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

GLuint load_shader_program(const std::string &vertexShaderFilename,
                           const std::string &fragmentShaderFilename)
{
    // Load and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexShaderSource = read_shader_source(vertexShaderFilename);
    const char *vertexShaderSourcePtr = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);

    glCompileShader(vertexShader);
    GLint compiled = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << "Vertex shader compilation failed:" << std::endl;
        show_shader_info_log(vertexShader);
        glDeleteShader(vertexShader);
        return 0;
    }

    // Load and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragmentShaderSource = read_shader_source(fragmentShaderFilename);
    const char *fragmentShaderSourcePtr = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);

    glCompileShader(fragmentShader);
    compiled = 0;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << "Fragment shader compilation failed:" << std::endl;
        show_shader_info_log(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Create program object
    GLuint program = glCreateProgram();

    // Attach shaders to the program. Also flag the shaders for automatic
    // deletion after their shader program is destroyed.
    glAttachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glAttachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    // Link program
    glLinkProgram(program);

    // Check linking status
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        std::cerr << "Linking failed:" << std::endl;
        show_program_info_log(program);
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    return program;
}

GLuint load_texture_2d(const std::string &filename)
{
    // Load image file (as an RGBA image with four components)
    int width, height, comp;
    uint8_t *image = stbi_load(filename.c_str(), &width, &height, &comp, 4);
    if (image == nullptr) {
        std::cout << "Error: " << stbi_failure_reason() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Create texture object for the image (and set sampling parameters)
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 &(image[0]));
    stbi_image_free(image);  // Clean up resources
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

// Load cubemap texture and let OpenGL generate a mipmap chain
GLuint load_cubemap(const std::string &dirname)
{
    const char *filenames[] = {"posx.png", "negx.png", "posy.png",
                               "negy.png", "posz.png", "negz.png"};
    const GLenum targets[] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                              GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                              GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
    const unsigned nSides = 6;  // A cube always has six sides...

    // Create texture object for the cubemap
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (unsigned i = 0; i < nSides; ++i) {
        // Load image for current cube side
        std::string filename = dirname + "/" + filenames[i];
        int width, height, comp;
        uint8_t *image = stbi_load(filename.c_str(), &width, &height, &comp, 4);
        if (image == nullptr) {
            std::cerr << "Error: " << stbi_failure_reason() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        glTexImage2D(targets[i], 0, GL_SRGB8_ALPHA8, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        stbi_image_free(image);  // Clean up resources
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return texture;
}

// Load cubemap with pre-computed mipmap chain
GLuint load_cubemap_prefiltered(const std::string &dirname)
{
    const char *levels[] = {"2048", "512", "128", "32", "8", "2", "0.5", "0.125"};
    const char *filenames[] = {"posx.png", "negx.png", "posy.png",
                               "negy.png", "posz.png", "negz.png"};
    const GLenum targets[] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                              GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                              GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
    const unsigned nLevels = sizeof(levels) / sizeof(levels[0]);
    const unsigned nSides = 6;  // A cube always has six sides...

    // Create texture object for the cubemap
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (unsigned i = 0; i < nLevels; ++i) {
        for (unsigned j = 0; j < nSides; ++j) {
            // Load image for current mip level and cube side
            std::string filename = dirname + "/" + levels[i] + "/" + filenames[j];
            int width, height, comp;
            uint8_t *image = stbi_load(filename.c_str(), &width, &height, &comp, 4);
            if (image == nullptr) {
                std::cerr << "Error: " << stbi_failure_reason() << std::endl;
                std::exit(EXIT_FAILURE);
            }
            glTexImage2D(targets[j], i, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, image);
            stbi_image_free(image);  // Clean up resources
        }
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return texture;
}

GLuint create_depth_texture(int width, int height)
{
    GLuint depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    return depthTexture;
}

GLuint create_depth_framebuffer(GLuint depthTexture)
{
    GLuint depthFramebuffer;
    glGenFramebuffers(1, &depthFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFramebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: framebuffer object not complete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return depthFramebuffer;
}

}  // namespace cg
