#pragma once

#include <glm/glm.hpp>

struct ViewPort {
    glm::vec3 position;
    glm::vec4 fwd{1, 0, 0, 0};
    glm::vec3 up{0, 0, 1};
    glm::vec3 right{0, 1, 0};
    float pitch = 0.0f,
        yaw = 0.0f;
    float aspect = 1.0f;

    glm::vec3 rotate_direction(const glm::vec4& dir) const;

    glm::mat4 get_matrix() const;
};

