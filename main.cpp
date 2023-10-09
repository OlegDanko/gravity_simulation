#include <glm/gtx/transform.hpp>
#include <gl_context/GLContext.hpp>
#include <WindowContext/GLFWContext.hpp>

#include "ViewPortController.hpp"
#include "Renderer.hpp"

#define _CPU_COMPUTE_ 1
#define _CPU_GPU_COMPUTE_ 2
#define _ROUTINE_ _CPU_GPU_COMPUTE_

#if _ROUTINE_ == _CPU_COMPUTE_
#include "CPUComputeRoutine.hpp"
using Routine_t = CPUComputeRoutine;
#elif _ROUTINE_ == _CPU_GPU_COMPUTE_
#include "CPUGPUComputeRoutine.hpp"
using Routine_t = CPUGPUComputeRoutine;
#endif

void init_bodies(Bodies& bodies, size_t num) {
    glm::vec3 z_axis{0.0f, 0.0f, 1.0f};

    for(auto i : std::views::iota((size_t)0, num)) {
        (void)i;

        auto dist = sqrt(rand_0_1<float>());
        auto angle = rand_1_1<float>() * M_PIf;

        glm::vec4 position = (glm::rotate(angle, z_axis)
                              * glm::vec4{dist, 0.0f, 0.0f, 1.0f}) / 1.0f;

        glm::vec4 velocity{position.y, -position.x, rand_1_1<float>()/5.0f, 0.0f};
        velocity *= dist / 2000.0f;
        float mass = rand_0_1<float>() / 4.1f;

        bodies.add(position, velocity, mass);
    }
}

struct WindowCallbacks : io::IWindowResizeListener
                       , io::IKeyInputListener
                       , io::IMouseMovementListener {
    std::function<void(int, int)> resize_callback;
    std::function<void(int, int)> key_input_callback;
    std::function<void(double, double)> mouse_movement_callback;

    io::IWindowResizeListener* as_window_resize_listener() { return this; }
    io::IKeyInputListener* as_key_input_listener() { return this; }
    io::IMouseMovementListener* as_mouse_movement_listener() { return this; }

    void serve_window_resized(int width, int height) override {
        if(resize_callback) resize_callback(width, height);
    }
    void serve_key_input(int key, int action, int) override {
        if(key_input_callback) key_input_callback(key, action);
    }
    void serve_mouse_movement(double x, double y) override {
        if(mouse_movement_callback) mouse_movement_callback(x, y);
    }
};

int main() {
    auto& glfw = io::GLFWContext::get();
    GLContext::get();

    int height, width;
    ViewPort vp;
    vp.position = {0.0f, 0.0f, 2.0f};
    vp.pitch = M_PI_2f - 0.01f;
    ViewPortController vp_ctl{vp};

    WindowCallbacks callbacks;
    callbacks.resize_callback = [&](auto w, auto h) {
        width = w;
        height = h;
        vp.aspect = float(w)/h;
    };
    callbacks.key_input_callback = [&](auto key, auto pressed) { vp_ctl.on_key(key, pressed); };
    callbacks.mouse_movement_callback = [&](auto x, auto y) { vp_ctl.on_mouse_mv(x, y); };
    glfw.set_listener(callbacks.as_window_resize_listener());
    glfw.set_listener(callbacks.as_key_input_listener());
    glfw.set_listener(callbacks.as_mouse_movement_listener());

    Bodies bodies;
//    init_bodies(bodies, 4096);
    init_bodies(bodies, 1024);
//    bodies.add({0.1, 0.0, 0.0, 0.0}, glm::vec4{0.0}, 1);
//    bodies.add({-0.1, 0.0, 0.0, 0.0}, glm::vec4{0.0}, 10);

    ArrayBufferObject
        vbo_positions_render,
        vbo_radii_render;

    vbo_positions_render.bind().init<glm::vec4>(bodies.get_count());
    vbo_radii_render.bind().init<float>(bodies.get_count());

    Renderer renderer(vbo_positions_render, vbo_radii_render);

    float G = 0.000000001f;
    Routine_t routine(bodies, vbo_positions_render, vbo_radii_render, G);

    glfw.update();
    std::tie(width, height) = glfw.get_dimensions();
    callbacks.resize_callback(width, height);
    while(glfw.update()) {
        routine.compute();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, width, height);

        vp_ctl.apply_movement();
        renderer.render(bodies.get_count(), vp.get_matrix(), vp.position);
    }

    return 0;
}
