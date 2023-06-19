#include <GL/glew.h>

#include <WindowContext/GLFWContext.hpp>
#include <GLContext.hpp>

#include "Renderer.hpp"
#include "BodiesCalculator.hpp"

#include <glm/gtx/transform.hpp>
#include <functional>
#include <cmath>


template<typename T>
T rand_0_1();

template<>
float rand_0_1<float>() {
    return (float)rand() / (float)RAND_MAX;
}

template<typename T>
T rand_1_1();

template<>
float rand_1_1<float>() {
    return (float)rand() / (float)RAND_MAX * 2 - 1;
}

template<>
glm::vec2 rand_1_1<glm::vec2>() {
    return { rand_1_1<float>(), rand_1_1<float>() };
}
template<>
glm::vec3 rand_1_1<glm::vec3>() {
    return { rand_1_1<float>(), rand_1_1<float>(), rand_1_1<float>() };
}


struct ResizeCallback : io::IWindowResizeListener {
    using callback_t = std::function<void(int, int)>;
    callback_t callback;
    ResizeCallback(callback_t cb) : callback(cb) {}

    void serve_window_resized(int width, int height) override {
        callback(width, height);
    }
};

void init_bodies(Bodies& bodies, size_t num) {
    glm::vec3 z_axis{ 0.0f, 0.0f, 1.0f};

    for(auto i : std::views::iota((size_t)0, num)) {
        (void)i;

        auto dist = sqrt(rand_0_1<float>());
        auto angle = rand_1_1<float>() * M_PIf;

        glm::vec3 position = (glm::rotate(angle, z_axis)
                              * glm::vec4{dist, 0.0f, 0.0f, 1.0f}) / 0.9f;

        glm::vec3 speed{position.y, -position.x, rand_1_1<float>()/10.0f};
        speed *= dist / 10000.0f;
        float mass = rand_0_1<float>()/sqrtf(dist) / 100.0f;

        bodies.add(position, speed, mass);
    }
}

int main() {
    auto& glfw = io::GLFWContext::get();
    auto& gl = GLContext::get();
    (void)gl;

    int height, width;
    ResizeCallback on_resized([&](auto w, auto h) -> void { width = w; height = h; });
    glfw.set_listener(&on_resized);

    Bodies bodies;

    init_bodies(bodies, 15000);

    BodiesCalculator calc(bodies);
    Renderer renderer(bodies);

    glfw.update();
    std::tie(width, height) = glfw.get_dimensions();
    while(glfw.update()) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, width, height);

        calc.update();
        renderer.update();
        renderer.render();
    }

    return 0;
}
