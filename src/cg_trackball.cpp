// Basic trackball implementation for interaction.
//
// Author: Fredrik Nysjo (2021)
//

#include "cg_trackball.h"

#include <cstdio>

namespace cg {

void trackball_move(Trackball &trackball, float x, float y)
{
    if (!trackball.tracking) return;

    glm::vec2 motion = glm::vec2(x, y) - trackball.center;
    if (glm::abs(motion.x) < 1.0f && glm::abs(motion.y) < 1.0f) return;

    glm::vec2 theta = trackball.speed * glm::clamp(motion, -trackball.clamp, trackball.clamp);
    glm::quat deltaX = glm::angleAxis(theta.x, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat deltaY = glm::angleAxis(theta.y, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat q = deltaY * deltaX * trackball.orient;
    q = (glm::length(q) > 0.0f) ? glm::normalize(q) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // Note: final quaternion should be in positive hemisphere
    trackball.orient = (q.w >= 0.0f) ? q : -q;
    trackball.center = glm::vec2(x, y);
}

}  // namespace cg
