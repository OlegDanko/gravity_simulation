#include "ViewPort.hpp"
#include <glm/gtx/transform.hpp>

glm::vec3 ViewPort::rotate_direction(const glm::vec4 &dir) const {
    return glm::rotate(yaw, up)
           * glm::rotate(pitch, right)
           * dir;
}

glm::mat4 ViewPort::get_matrix() const {
    auto P = glm::perspective(1.222f, aspect, 0.01f, 1000.0f);
    glm::vec3 target = position + rotate_direction(fwd);
    auto V = glm::lookAt(position, target, up);
    return P * V;
}
