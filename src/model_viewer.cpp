// Model viewer code for the assignments in Computer Graphics 1TD388/1MD150.
//
// Modify this and other source files according to the tasks in the instructions.
//

#include "gltf_io.h"
#include "gltf_scene.h"
#include "gltf_render.h"
#include "cg_utils.h"
#include "cg_trackball.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <iostream>

// Struct for our application context
struct Context {
    int width = 1024;
    int height = 512;
    GLFWwindow *window;
    gltf::GLTFAsset asset;
    gltf::DrawableList drawables;
    cg::Trackball trackball;
    GLuint program;
    GLuint emptyVAO;
    float elapsedTime;
    std::string gltfFilename = "teapot.gltf";
    // Add more variables here...
    glm::vec3 diffuseColor = glm::vec3(0.0f, 0.5f, 0.0f);
    bool ambientEnabled = true;
    bool diffuseEnabled = true;
    bool specularEnabled;
    float specularPower = 16.0f;
    glm::vec3 lightPosition = glm::vec3(1, 1, 1);
    glm::vec3 bgColor = glm::vec3(0.3f);;
    bool ortho;
    bool gamma = true;
    float zoom = 60.0f;

    bool envMapping;
    int textureIndex = 4;
    int sceneIndex = 2;
    GLuint cubemap;

    gltf::TextureList textures;
    bool texMapping;
    bool textureCoordinates;
    bool lighting;

    bool quantizationEnabled;
    const float qmap[3][8] = { // quantization map textures
        {0.3, 0.3, 0.5, 0.5, 0.5, 0.7, 0.7, 0.9},
        {0.3, 0.3, 0.4, 0.4, 0.6, 0.6, 0.8, 0.8},
        {0.3, 0.3, 0.3, 0.6, 0.6, 0.6, 0.8, 0.8},
    };
    int qmapIndex = 0;
    GLuint quantizationTexture;

    GLuint outlineProgram;
    GLuint outlineFBO;
    bool viewDepth;
    GLuint depthTexture;
    bool viewNormals;
    GLuint normalTexture;
};

// Returns the absolute path to the src/shader directory
std::string shader_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/src/shaders/";
}

std::string cubemap_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/cubemaps/";
}

// Returns the absolute path to the assets/gltf directory
std::string gltf_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/gltf/";
}

void do_initialization(Context &ctx)
{
    ctx.program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
    ctx.outlineProgram = cg::load_shader_program(shader_dir() + "outline.vert", shader_dir() + "outline.frag");

    gltf::load_gltf_asset(ctx.gltfFilename, gltf_dir(), ctx.asset);
    gltf::create_drawables_from_gltf_asset(ctx.drawables, ctx.asset);
    gltf::create_textures_from_gltf_asset(ctx.textures, ctx.asset);

    // quantization initialization
    glGenTextures(1, &ctx.quantizationTexture);
    glBindTexture(GL_TEXTURE_1D, ctx.quantizationTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_1D, 0);
    
    // outline framebuffer initialization
    // screen quad VAO
    glGenFramebuffers(1, &ctx.outlineFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.outlineFBO);

    // normal map texture
    glGenTextures(1, &ctx.normalTexture);
    glBindTexture(GL_TEXTURE_2D, ctx.normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.width, ctx.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // depth texture
    glGenTextures(1, &ctx.depthTexture);
    glBindTexture(GL_TEXTURE_2D, ctx.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, ctx.width, ctx.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ctx.normalTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ctx.depthTexture, 0);
    GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT};
    glDrawBuffers(2, DrawBuffers);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void defineUniforms(Context &ctx) {
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsedTime);

    if (ctx.ambientEnabled) glUniform1f(glGetUniformLocation(ctx.program, "u_ambientEnabled"), 1.0);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_ambientEnabled"), 0.0);

    if (ctx.diffuseEnabled) glUniform1f(glGetUniformLocation(ctx.program, "u_diffuseEnabled"), 1.0);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_diffuseEnabled"), 0.0);

    if (ctx.specularEnabled) glUniform1f(glGetUniformLocation(ctx.program, "u_specularEnabled"), 1.0);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_specularEnabled"), 0.0);

    glUniform3f(glGetUniformLocation(ctx.program, "u_diffuseColor"), ctx.diffuseColor[0], ctx.diffuseColor[1], ctx.diffuseColor[2]);

    glUniform1f(glGetUniformLocation(ctx.program, "u_specularPower"), ctx.specularPower);
    glUniform3f(glGetUniformLocation(ctx.program, "u_lightPosition"), ctx.lightPosition[0], ctx.lightPosition[1], ctx.lightPosition[2]);

    if (ctx.gamma) glUniform1f(glGetUniformLocation(ctx.program, "u_gamma"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_gamma"), 0.0f);

    if (ctx.textureCoordinates) glUniform1f(glGetUniformLocation(ctx.program, "u_viewTextureCoords"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_viewTextureCoords"), 0.0f);

    if (ctx.envMapping) glUniform1f(glGetUniformLocation(ctx.program, "u_envMapping"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_envMapping"), 0.0f);

    if (ctx.texMapping) glUniform1f(glGetUniformLocation(ctx.program, "u_texMapping"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_texMapping"), 0.0f);

    if (ctx.lighting) glUniform1f(glGetUniformLocation(ctx.program, "u_lighting"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_lighting"), 0.0f);

    if (ctx.quantizationEnabled) glUniform1f(glGetUniformLocation(ctx.program, "u_quantizationEnabled"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_quantizationEnabled"), 0.0f);
    
    if (ctx.viewDepth) glUniform1f(glGetUniformLocation(ctx.program, "u_viewDepth"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_viewDepth"), 0.0f);

    if (ctx.viewNormals) glUniform1f(glGetUniformLocation(ctx.program, "u_viewNormals"), 1.0f);
    else glUniform1f(glGetUniformLocation(ctx.program, "u_viewNormals"), 0.0f);
}

void draw_scene(Context &ctx, GLuint program)
{
    // Set render state
    glUseProgram(program);
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    glm::mat4 Projection = glm::mat4(1.0f);
    glm::mat4 View = glm::mat4(1.0f);
    glm::mat4 Model = glm::mat4(1.0f);

    // Projection Matrix
    if (ctx.ortho) Projection = glm::ortho(
       -float(ctx.width / ctx.height),  // left
        float(ctx.width / ctx.height),  // right
       -1.0f, 1.0f,                     // top, bottom
        1.f, 7.5f                       // zNear, zFar
    );
    else Projection = glm::perspective( 
        glm::radians(ctx.zoom),         // FOV
        float(ctx.width / ctx.height),  // view ratio
        1.f, 7.5f                       // zNear, zFar
    );
    // Camera matrix
    View = glm::lookAt(
        glm::vec3(1, 1, 1),  // Camera is at (1,1,1) in World Space
        glm::vec3(0, 0, 0),  // looks at the origin
        glm::vec3(0, 1, 0)   // Head is up
        ) * glm::mat4(-ctx.trackball.orient);
    
    // Define per-scene uniforms
    glUniformMatrix4fv(glGetUniformLocation(program, "u_projection"), 1, GL_FALSE, &Projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, &View[0][0]);

    defineUniforms(ctx);
    // Cubemapping
    std::vector<std::string> selectedCubemap{"0.125/", "0.5/", "2/", "8/", "32/", "128/", "512/", "2048/"};
    std::vector<std::string> selectedScene{"Forrest/", "LarnacaCastle/", "RomeChurch/"};
    ctx.cubemap = cg::load_cubemap(cubemap_dir() + selectedScene[ctx.sceneIndex] + "prefiltered/" + selectedCubemap[ctx.textureIndex]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.cubemap);
    glUniform1i(glGetUniformLocation(ctx.program, "u_cubemap"), 0);

    // Toon shading Quantization
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, ctx.quantizationTexture);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, 8, 0, GL_RED, GL_FLOAT, &ctx.qmap[ctx.qmapIndex]);
    glUniform1i(glGetUniformLocation(ctx.program, "u_quantization"), 1);

    // depth texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ctx.depthTexture);
    glUniform1i(glGetUniformLocation(ctx.program, "u_depthTexture"), 2);

    // normal texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, ctx.normalTexture);
    glUniform1i(glGetUniformLocation(ctx.program, "u_normalTexture"), 3);

    // Draw scene
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define per-object uniforms
        // Model matrix : an identity matrix (model will be at the origin)
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.75f));  // scale by 0.5
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));
        glm::mat4 translationMatrix = glm::mat4(1.0f);
        Model = translationMatrix * rotationMatrix * scaleMatrix;
        glUniformMatrix4fv(glGetUniformLocation(program, "u_model"), 1, GL_FALSE, &Model[0][0]);

        // texture mapping (ASSIGNMENT 3 PART 3)
        const gltf::Mesh &mesh = ctx.asset.meshes[node.mesh];
        if (mesh.primitives[0].hasMaterial) {
            const gltf::Primitive &primitive = mesh.primitives[0];
            const gltf::Material &material = ctx.asset.materials[primitive.material];
            const gltf::PBRMetallicRoughness &pbr = material.pbrMetallicRoughness;

            // Define material textures and uniforms
            if (pbr.hasBaseColorTexture) {
                GLuint texture_id = ctx.textures[pbr.baseColorTexture.index];
                // Bind texture and define uniforms...
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, texture_id);
                glUniform1i(glGetUniformLocation(ctx.program, "u_texture"), 4);
            } else {
                // Need to handle this case as well, by telling
                // the shader that no texture is available
                ctx.texMapping = false;
            }
        }

        // Draw object
        glBindVertexArray(drawable.vao);
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType, 
            (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
    }

    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
}

void do_rendering(Context &ctx)
{
    cg::reset_gl_render_state();

    // 1. first render to outline framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, ctx.outlineFBO);
    draw_scene(ctx, ctx.outlineProgram);

    // 2. then render scene as normal
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(ctx.bgColor[0], ctx.bgColor[1], ctx.bgColor[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_scene(ctx, ctx.program);
}

void reload_shaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    glDeleteProgram(ctx->outlineProgram);
    ctx->program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
    ctx->outlineProgram = cg::load_shader_program(shader_dir() + "outline.vert", shader_dir() + "outline.frag");
}

void error_callback(int /*error*/, const char *description)
{
    std::cerr << description << std::endl;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) { reload_shaders(ctx); }
}

void char_callback(GLFWwindow *window, unsigned int codepoint)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) return;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        ctx->trackball.tracking = (action == GLFW_PRESS);
    }
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    if (ImGui::GetIO().WantCaptureMouse) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    cg::trackball_move(ctx->trackball, float(x), float(y));
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_ScrollCallback(window, x, y);
    if (ImGui::GetIO().WantCaptureMouse) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->zoom -= float(y);
    if (ctx->zoom <= 1.0f) ctx->zoom = 1.0f;
    if (ctx->zoom >= 110.0f) ctx->zoom = 110.0f;
}

void resize_callback(GLFWwindow *window, int width, int height)
{
    // Update window size and viewport rectangle
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    glViewport(0, 0, width, height);
}

int main(int argc, char *argv[])
{
    Context ctx = Context();
    if (argc > 1) { ctx.gltfFilename = std::string(argv[1]); }

    // Create a GLFW window
    glfwSetErrorCallback(error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);
    glfwSetMouseButtonCallback(ctx.window, mouse_button_callback);
    glfwSetCursorPosCallback(ctx.window, cursor_pos_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);
    glfwSetFramebufferSizeCallback(ctx.window, resize_callback);

    // Load OpenGL functions
    if (gl3wInit() || !gl3wIsSupported(3, 3) /*check OpenGL version*/) {
        std::cerr << "Error: failed to initialize OpenGL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize ImGui
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(ctx.window, false /*do not install callbacks*/);
    ImGui_ImplOpenGL3_Init("#version 330" /*GLSL version*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.emptyVAO);
    glBindVertexArray(ctx.emptyVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    do_initialization(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsedTime = glfwGetTime();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (ImGui::Begin("Settings")) {
            if (ImGui::CollapsingHeader("Material")) {
                ImGui::Checkbox("Diffuse Enabled", &ctx.diffuseEnabled);
                if (!ctx.texMapping || !ctx.envMapping)
                    ImGui::ColorEdit3("Diffuse Color", &ctx.diffuseColor[0]);
                ImGui::SliderFloat("Specular power", &ctx.specularPower, 5.0f, 50.0f);
                ImGui::Checkbox("Specular Enabled", &ctx.specularEnabled);
            }
            if (ImGui::CollapsingHeader("Lighting")) {
                ImGui::SliderFloat3("Light Position", &ctx.lightPosition[0], 0, 1);
                ImGui::Checkbox("Ambient Enabled", &ctx.ambientEnabled);
            }
            if (ImGui::CollapsingHeader("Background")) {
                ImGui::ColorEdit3("Background Color", &ctx.bgColor[0]);
            }
            if (ImGui::CollapsingHeader("Mapping")) {
                if (!ctx.texMapping) ImGui::Checkbox("Environment Mapping", &ctx.envMapping);
                if (ctx.envMapping) ImGui::SliderInt("Scene", &ctx.sceneIndex, 0, 2);
                if (ctx.envMapping) ImGui::SliderInt("Texture ID", &ctx.textureIndex, 0, 7);

                if (!ctx.envMapping) ImGui::Checkbox("Texture Mapping", &ctx.texMapping);
                if (ctx.texMapping) ImGui::Checkbox("Blinn-Phong Lighting", &ctx.lighting);
            }
            if (ImGui::CollapsingHeader("Toon Shading")) {
                ImGui::Checkbox("Quantization", &ctx.quantizationEnabled);
                if (ctx.quantizationEnabled) ImGui::SliderInt("Q-map", &ctx.qmapIndex, 0, 2);
                ImGui::Checkbox("View Depth Texture", &ctx.viewDepth);
                ImGui::Checkbox("View Normal Texture", &ctx.viewNormals);
            }
            if (ImGui::CollapsingHeader("Misc")) {
                ImGui::Checkbox("Orthographic Projection", &ctx.ortho);
                ImGui::Checkbox("Gamma Correction", &ctx.gamma);
                ImGui::Checkbox("Texture Coordinates", &ctx.textureCoordinates);
            }
        }
        ImGui::End();
        do_rendering(ctx);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
