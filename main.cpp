#include <GL/glew.h>

#include <WindowContext/GLFWContext.hpp>
#include <GLContext.hpp>

#include "GLComputeShaders.hpp"

#include "Renderer.hpp"
#include "BodiesCalculator.hpp"
#include "BodiesCalculatorGlSim.hpp"

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

template<typename T>
std::enable_if_t<std::is_integral_v<T>, T> div_ceil(T a, T b) {
    return (a + b - 1) / b;
}

template<typename T>
T max(T t) {
    return t;
}

template<typename T, typename ...Ts>
T max(T t, const Ts... ts) {
    return std::max(t, max(ts...));
}


struct ResizeCallback : io::IWindowResizeListener {
    using callback_t = std::function<void(int, int)>;
    callback_t callback;
    ResizeCallback(callback_t cb) : callback(cb) {}

    void serve_window_resized(int width, int height) override {
        callback(width, height);
    }
};

struct GLBodyCalculator {
    Bodies& bodies;
    size_t chunk_size{128};

    ForceComputeShader force_compute;
    TotalForceComputeShader total_force_compute;
    UpdatePositionVelocityComputeShader update_params_compute;
    GravityComputeShader gravity_compute_shader;
    GLuint query;

    VertexBufferObject
        vbo_position_read,
        vbo_velocity_read,
        vbo_velocity_write,
        vbo_masses_read,
        vbo_force_chunk,
        vbo_force_total;

    size_t
        pos_buffer_size_f,
        vel_buffer_size_f,
        mas_buffer_size_f,
        force_chunk_size_f,
        force_total_size_f;

    VertexBufferObject& vbo_position_write;
    std::vector<GLfloat> zero_buf;

    GLBodyCalculator(Renderer& r)
        : bodies(r.bodies)
        , vbo_position_write(r.vbo_pos) {
        size_t chunks_count = div_ceil(bodies.get_count(), chunk_size);
        size_t elements_count = chunks_count * chunk_size;

        glGenQueries(1, &query);

        pos_buffer_size_f = elements_count * 4;
        vel_buffer_size_f = elements_count * 4;
        mas_buffer_size_f = elements_count;
        force_chunk_size_f = chunk_size * chunk_size * 4;
        force_total_size_f = elements_count * 4;

        zero_buf = std::vector<GLfloat>(max(force_chunk_size_f,
                                            force_total_size_f));

        vbo_position_read.upload(zero_buf, pos_buffer_size_f);
        vbo_velocity_read.upload(zero_buf, vel_buffer_size_f);
        vbo_velocity_write.upload(zero_buf, vel_buffer_size_f);
        vbo_masses_read.upload(zero_buf, mas_buffer_size_f);
        vbo_force_chunk.upload(zero_buf, force_chunk_size_f);
        vbo_force_total.upload(zero_buf, force_total_size_f);

        vbo_position_read.update(bodies.get_positions());
        vbo_velocity_read.update(bodies.get_velocities());
        vbo_masses_read.update(bodies.get_masses());
    }

    void calculate() {
        size_t chunks_count = div_ceil(bodies.get_count(), chunk_size);

        for(auto b_chunk_id : std::views::iota(size_t(0), chunks_count))
            for(auto a_chunk_id = 0; a_chunk_id <= b_chunk_id; ++a_chunk_id) {
                size_t a_offset = a_chunk_id * chunk_size;
                size_t b_offset = a_chunk_id * chunk_size;

                force_compute.use_program();

                force_compute.set_position_in(vbo_position_read);
                force_compute.set_mass_in(vbo_masses_read);
                force_compute.set_force_out(vbo_force_chunk);

                force_compute.set_G(0.000000001);
                force_compute.set_chunk_size(chunk_size);

                force_compute.set_a_offset(a_offset);
                force_compute.set_b_offset(b_offset);

                force_compute.dispatch(chunk_size*chunk_size, 1, 1);
                force_compute.barrier();

                total_force_compute.use_program();
                total_force_compute.set_force_in(vbo_force_chunk);
                total_force_compute.set_force_out(vbo_force_total);
                total_force_compute.set_chunk_size(chunk_size);

                total_force_compute.set_a_offset(a_offset);
                total_force_compute.set_b_offset(b_offset);

                total_force_compute.dispatch(chunk_size, 1, 1);
                total_force_compute.barrier();
            }

        update_params_compute.use_program();
        update_params_compute.set_position_in(vbo_position_read);
        update_params_compute.set_velocity_in(vbo_velocity_read);
        update_params_compute.set_mass_in(vbo_masses_read);
        update_params_compute.set_force_in(vbo_force_total);
        update_params_compute.set_position_out(vbo_position_write);
        update_params_compute.set_velocity_out(vbo_velocity_write);

        update_params_compute.dispatch(bodies.get_count(), 1, 1);
        update_params_compute.barrier();

    }

    void calculate_2() {
        gravity_compute_shader.use_program();
        gravity_compute_shader.set_position_in(vbo_position_read);
        gravity_compute_shader.set_velocity_in(vbo_velocity_read);
        gravity_compute_shader.set_mass_in(vbo_masses_read);
        gravity_compute_shader.set_position_out(vbo_position_write);
        gravity_compute_shader.set_velocity_out(vbo_velocity_write);

        gravity_compute_shader.set_elements_count(bodies.get_count());
        gravity_compute_shader.set_G(0.000000001);

        gravity_compute_shader.dispatch(bodies.get_count(), 1, 1);
        gravity_compute_shader.barrier();
    }

    void update() {
        std::swap(vbo_position_read, vbo_position_write);
        std::swap(vbo_velocity_read, vbo_velocity_write);

        vbo_position_read.update(bodies.get_positions());
        vbo_velocity_read.update(bodies.get_velocities());
        vbo_masses_read.update(bodies.get_masses());

        vbo_position_write.update(zero_buf, pos_buffer_size_f);
        vbo_velocity_write.update(zero_buf, vel_buffer_size_f);
        vbo_force_chunk.update(zero_buf, force_chunk_size_f);
        vbo_force_total.update(zero_buf, force_total_size_f);

        glBeginQuery(GL_TIME_ELAPSED, query);
//        calculate();
        calculate_2();
        glEndQuery(GL_TIME_ELAPSED);
        GLint64 elapsed;
        glGetQueryObjecti64v(query, GL_QUERY_RESULT, &elapsed);

        std::cout << elapsed / 1000000.0f << std::endl;
    }

    void cpy_params_gpu_to_cpu() {
        vbo_position_write.download(bodies.get_positions(), bodies.get_count());
        vbo_velocity_write.download(bodies.get_velocities(), bodies.get_count());
    }
};

void init_bodies(Bodies& bodies, size_t num) {
    glm::vec3 z_axis{0.0f, 0.0f, 1.0f};

    for(auto i : std::views::iota((size_t)0, num)) {
        (void)i;

        auto dist = sqrt(rand_0_1<float>());
        auto angle = rand_1_1<float>() * M_PIf;

        glm::vec4 position = (glm::rotate(angle, z_axis)
                              * glm::vec4{dist, 0.0f, 0.0f, 1.0f}) / 1.0f;

        glm::vec4 velocity{position.y, -position.x, rand_1_1<float>()/5.0f, 0.0f};
        velocity *= dist / 10000.0f;
        float mass = rand_0_1<float>()/sqrtf(dist) / 10.1f;

        bodies.add(position, velocity, mass);
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

    init_bodies(bodies, 4096);
//    init_bodies(bodies, 1024);
//    bodies.add({0.5, 0.0, 0.0, 0.0}, glm::vec4{0.0}, 1);
//    bodies.add({-0.5, 0.0, 0.0, 0.0}, glm::vec4{0.0}, 10);

    BodiesCalculator calc(bodies);
    Renderer renderer(bodies);
    GLBodyCalculator gl_calc(renderer);

    glfw.update();
    std::tie(width, height) = glfw.get_dimensions();


    auto start = std::chrono::steady_clock::now();
    while(glfw.update()) {
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        start = end;
        std::cout << elapsed.count() << std::endl;

//        calc.update();
        calc.update_collisions();
//        calc.update_velocities_lazy_parallel();
//        calc.update_positions();
//        calc.halt_escapers();

        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);


        gl_calc.update();
//        renderer.update();
        renderer.upload_radii();
        renderer.render();
        gl_calc.cpy_params_gpu_to_cpu();
    }

    return 0;
}
