#include "ViewPortController.hpp"
#include <GLFW/glfw3.h>

void ViewPortController::apply_movement() {
    auto mag = mv_speed * mv_speed_mod * timer.lap().count();
    if(!(f | b | l | r | u | d))
        return;

    auto mv_vec_cam = glm::normalize(glm::vec4(1.0f * f - 1.0f * b,
                                               1.0f * l - 1.0f * r,
                                               1.0f * u - 1.0f * d,
                                               0.0f)) * mag;

    auto mv_vec_world = vp.rotate_direction(mv_vec_cam);
    vp.position += mv_vec_world;
}

void ViewPortController::serve_state_change(bool &direction, bool pressed) {
    if(direction == pressed) return;
    apply_movement();

    direction = pressed;
}

void ViewPortController::on_key(int key, bool pressed) {
    switch(key) {
    case GLFW_KEY_W: serve_state_change(f, pressed); break;
    case GLFW_KEY_S: serve_state_change(b, pressed); break;
    case GLFW_KEY_A: serve_state_change(l, pressed); break;
    case GLFW_KEY_D: serve_state_change(r, pressed); break;
    case GLFW_KEY_Q: serve_state_change(u, pressed); break;
    case GLFW_KEY_E: serve_state_change(d, pressed); break;
    case GLFW_KEY_C: vp.fov = pressed ? 0.722 : 1.222f; break;
    case GLFW_KEY_LEFT_SHIFT: mv_speed_mod = pressed ? 10.0f : 1.0f; break;
    default: break;
    };
}

void ViewPortController::on_mouse_mv(float x, float y) {
    vp.yaw = std::remainder(vp.yaw - x * view_speed, 2 * M_PIf);
    vp.pitch = std::clamp(vp.pitch + y * view_speed, -pitch_limit, pitch_limit);
}
