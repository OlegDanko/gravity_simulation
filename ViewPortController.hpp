#pragma once
#include "ViewPort.hpp"
#include "Utils.hpp"

struct ViewPortController {
    ViewPort& vp;
    bool f = false,
        b = false,
        l = false,
        r = false,
        u = false,
        d = false;
    float mv_speed = 1.0f/1000.0f;
    float mv_speed_mod = 1.0f;
    float view_speed = 1.0f/1000.0f;
    float pitch_limit = M_PI_2f - 0.001f;
    Timer<std::chrono::milliseconds> timer{};

    void apply_movement();

    void serve_state_change(bool& direction, bool pressed);

    void on_key(int key, bool pressed);
    void on_mouse_mv(float x, float y);
};
