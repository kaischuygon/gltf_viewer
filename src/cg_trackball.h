// Basic trackball implementation for interaction.
//
// Author: Fredrik Nysjo (2021)
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace cg {

// Struct for representing a virtual trackball for interaction
struct Trackball {
    glm::vec2 center = glm::vec2(0.0f);
    glm::quat orient = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float speed = 0.004f;
    float clamp = 100.0f;
    bool tracking = false;
};

// Update trackball state from 2D mouse input that should be provided in
// screen-space coordinates
void trackball_move(Trackball &trackball, float x, float y);

}  // namespace cg
