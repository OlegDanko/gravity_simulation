#include <GL/glew.h>
#include <glm/gtx/norm.hpp>

#include <WindowContext/GLFWContext.hpp>
#include <GLContext.hpp>
#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>

#include <functional>
#include <ranges>

std::string vertex_code = R"(
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in float mass;

out float _mass;

void main()
{
    gl_Position = vec4(position, 1.0);
    _mass = mass;
}
)";

std::string fragment_code = R"(
#version 330 core

in float _mass;

out vec4 FragColor;
void main()
{
   FragColor = vec4(_mass, 1.0f - _mass, 1.0f, 1.0f);
//   FragColor = vec4(1.0f);
}
)";


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

struct Bodies {
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> velocity;
    std::vector<float> mass;
    std::vector<float> size;

    float mass_to_size(float val) {
        return val/1000.0f;
    }

    void add(const glm::vec3& pos,
             const glm::vec3& vel,
             float m) {
        position.push_back(pos);
        velocity.push_back(vel);
        mass.push_back(m);
        size.push_back(mass_to_size(m));
    }
};

struct BodiesCalculator {
    float G = 0.000000001;
    Bodies& bodies;
    std::vector<std::pair<size_t, size_t>> pairs;
    BodiesCalculator(Bodies& bodies) : bodies(bodies) {
        calculate_pairs(bodies.position.size());
    }

    void calculate_pairs(size_t count) {
        for(auto x : std::views::iota((size_t)0, count))
            for(auto y : std::views::iota(x+1, count))
                pairs.push_back({x, y});
    }

    glm::vec3 calc_force(glm::vec3& p1, float& m1, glm::vec3& p2, float m2) {
        auto F = G*m1*m2/glm::distance2(p1, p2);
        return glm::normalize(p2-p1) * F;
    }

    void update_velocities() {
        std::vector<glm::vec3> forces;
        std::transform(pairs.begin(), pairs.end(), std::back_inserter(forces), [&](auto& pair){
            auto [a, b] = pair;
            return calc_force(bodies.position.at(a),
                              bodies.mass.at(a),
                              bodies.position.at(b),
                              bodies.mass.at(b));
        });
        for(std::tuple<std::pair<size_t, size_t>&, glm::vec3&> pf
             : std::views::zip(pairs, forces)) {
            auto [a, b] = std::get<0>(pf);
            auto f = std::get<1>(pf);
            bodies.velocity.at(a) += f / bodies.mass.at(a);
            bodies.velocity.at(b) -= f / bodies.mass.at(b);
        }
    }

    void update_positions() {
        for(std::tuple<glm::vec3&, glm::vec3&> pv
             : std::views::zip(bodies.position, bodies.velocity)) {
            std::get<0>(pv) += std::get<1>(pv);
        }
    }

    void update() {
        update_velocities();
        update_positions();
    }
};

struct Renderer {
    Bodies& bodies;
    VertexArrayObject vao;
    VertexBufferObject vbo;

    size_t size;
    size_t capacity;

    size_t closest_pow_of_2(size_t val) {
        size_t p2 = 2;
        while(val > p2) {
            p2 *= 2;
        }
        return p2;
    }

    Renderer(Bodies& bodies) : bodies(bodies) {
        auto& positions = bodies.position;
        size = positions.size();
        capacity = closest_pow_of_2(size);
        std::vector<glm::vec3> p_buffer(capacity);
        std::copy(positions.begin(), positions.end(), p_buffer.begin());
        vbo.load(bodies.position);
        vao.add_array_buffer(vbo, 0, 3);
    }

    void update() {
        vbo.update(bodies.position);
    }

    void render() {
        vao.bind();
        glDrawArrays(GL_POINTS, 0, size);
        vao.unbind();
    }
};

int main()
{
    auto& glfw = io::GLFWContext::get();
    auto& gl = GLContext::get();
    (void)gl;

    auto [height, width] = glfw.get_dimensions();
    ResizeCallback on_resized([&](auto w, auto h) -> void { width = w; height = h; });
    glfw.set_listener(&on_resized);

    ShaderProgram program(Shader::make_vertex(vertex_code),
                          Shader::make_fragment(fragment_code));

    glPointSize(5);

    Bodies bodies;

    for(int i = 0; i < 100; i++) {
        bodies.add({rand_1_1<glm::vec2>()/5.0f, 0.0f}, {rand_1_1<glm::vec2>()/1000.0f, 0.0f}, 1.0);
    }

    BodiesCalculator calc(bodies);
    Renderer renderer(bodies);

    while(glfw.update()) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, width, height);

        program.use();

        calc.update();
        renderer.update();
        renderer.render();
    }

    return 0;
}
