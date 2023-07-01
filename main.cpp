#include <GL/glew.h>
#include <WindowContext/GLFWContext.hpp>
#include <GLContext.hpp>
#include <glm/gtx/transform.hpp>

#include "ComputeGPU.hpp"
#include "ComputeCPU.hpp"
#include "Renderer.hpp"

#include "Utils.hpp"

struct CPUComputeRoutine {
    Bodies& bodies;
    VertexBufferObject &pos_out, &rad_out;
    float G;

    CPUComputeRoutine(Bodies& bodies,
                      VertexBufferObject &positions_out,
                      VertexBufferObject  &radii_out,
                      float G)
        : bodies(bodies)
        , pos_out(positions_out)
        , rad_out(radii_out)
        , G(G) {}

    void compute() {
        compute_collisions_cpu(bodies);
        rad_out.update(bodies.get_radii());

        compute_gravity_cpu_parallel(bodies, G, 8);
        pos_out.update(bodies.get_positions());
    }
};
struct CPUGPUComputeRoutine {
    Bodies& bodies;
    VertexBufferObject &vbo_positions_out, &rad_out;
    float G;

    VertexBufferObject
        vbo_position_calc_in,
        vbo_velocities_calc_in,
        vbo_mass_calc_in,
        vbo_velocities_calc_out;

    GravityComputeGPU gravity_compute;

    CPUGPUComputeRoutine(Bodies& bodies,
                         VertexBufferObject &positions_out,
                         VertexBufferObject  &radii_out,
                         float G)
        : bodies(bodies)
        , vbo_positions_out(positions_out)
        , rad_out(radii_out)
        , G(G) {
        vbo_position_calc_in.init<glm::vec4>(bodies.get_count());
        vbo_velocities_calc_in.init<glm::vec4>(bodies.get_count());
        vbo_mass_calc_in.init<float>(bodies.get_count());
        vbo_velocities_calc_out.init<glm::vec4>(bodies.get_count());
        gravity_compute.set_vbos(vbo_position_calc_in,
                                 vbo_velocities_calc_in,
                                 vbo_mass_calc_in,
                                 vbo_positions_out,
                                 vbo_velocities_calc_out);
    }

    void compute() {
        compute_collisions_cpu(bodies);
        rad_out.update(bodies.get_radii());

        vbo_position_calc_in.update(bodies.get_positions(), bodies.get_count());
        vbo_velocities_calc_in.update(bodies.get_velocities(), bodies.get_count());
        vbo_mass_calc_in.update(bodies.get_masses(), bodies.get_count());

        gravity_compute.calculate(bodies.get_count(), G);

        vbo_positions_out.download(bodies.get_positions(), bodies.get_count());
        vbo_velocities_calc_out.download(bodies.get_velocities(), bodies.get_count());
    }
};

template<typename Bodies_t>
void init_bodies(Bodies_t& bodies, size_t num) {
    glm::vec3 z_axis{0.0f, 0.0f, 1.0f};

    for(auto i : std::views::iota((size_t)0, num)) {
        (void)i;

        auto dist = sqrt(rand_0_1<float>());
        auto angle = rand_1_1<float>() * M_PIf;

        glm::vec4 position = (glm::rotate(angle, z_axis)
                              * glm::vec4{dist, 0.0f, 0.0f, 1.0f}) / 1.0f;

        glm::vec4 velocity{position.y, -position.x, rand_1_1<float>()/5.0f, 0.0f};
        velocity *= dist / 1000.0f;
        float mass = rand_0_1<float>() / 4.1f;

        bodies.add(position, velocity, mass);
    }
}

struct ResizeCallback : io::IWindowResizeListener {
    using callback_t = std::function<void(int, int)>;
    callback_t callback;
    ResizeCallback(callback_t cb) : callback(cb) {}

    void serve_window_resized(int width, int height) override {
        callback(width, height);
    }
};

int main() {
    auto& glfw = io::GLFWContext::get();
    auto& gl = GLContext::get();
    (void)gl;

    int height, width;
    ResizeCallback on_resized([&](auto w, auto h) { width = w; height = h; });
    glfw.set_listener(&on_resized);

    Bodies bodies;

    init_bodies(bodies, 4096);
//    init_bodies(bodies, 1024);
//    bodies.add({0.1, 0.0, 0.0, 0.0}, glm::vec4{0.0}, 1);
//    bodies.add({-0.1, 0.0, 0.0, 0.0}, glm::vec4{0.0}, 10);

    VertexBufferObject
        vbo_positions_render,
        vbo_radii_render;

    vbo_positions_render.init<glm::vec4>(bodies.get_count());
    vbo_radii_render.init<float>(bodies.get_count());

    Renderer renderer(vbo_positions_render, vbo_radii_render);

    float G = 0.000000001f;
//    CPUComputeRoutine routine(bodies, vbo_positions_render, vbo_radii_render, G);
    CPUGPUComputeRoutine routine(bodies, vbo_positions_render, vbo_radii_render, G);

    glfw.update();
    std::tie(width, height) = glfw.get_dimensions();


    while(glfw.update()) {
        routine.compute();

        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        renderer.render(bodies.get_count());
    }

    return 0;
}
