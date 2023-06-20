#include <GL/glew.h>

#include <WindowContext/GLFWContext.hpp>
#include <GLContext.hpp>

#include "Renderer.hpp"
#include "BodiesCalculator.hpp"
#include "BodiesCalculatorGlSim.hpp"

#include <glm/gtx/transform.hpp>
#include <functional>
#include <cmath>

std::string force_calc_vertex_code_ = R"(
#version 430 core

layout(location = 0) uniform samplerBuffer pos_tex;
layout(location = 1) uniform samplerBuffer mass_tex;
layout(location = 2) uniform int a_offset;
layout(location = 3) uniform int b_offset;
layout(location = 4) uniform float pixel_offset;
layout(location = 5) uniform float pixel_size;

out vec3 force_vec;

float G = 0.000000001;

void main()
{
    highp int row = int((sqrt(8*gl_VertexID + 1) - 1) / 2);
    int col = gl_VertexID - row * (row + 1) / 2;
    int a_id = a_offset + col;
    int b_id = b_offset + row;
    vec2 coord = vec2(pixel_offset + pixel_size*row, pixel_offset + pixel_size*col);

    vec3 a_pos = texelFetch(pos_tex, a_id).xyz;
    vec3 b_pos = texelFetch(pos_tex, b_id).xyz;

    vec3 v = a_pos - b_pos;
    float dist2 = v.x*v.x + v.y*v.y + v.z*v.z;

    if(dist2 == 0.0f) {
        gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
        force_vec = vec3(0.0);
        return;
    }

    float a_mass = texelFetch(mass_tex, a_id).x;
    float b_mass = texelFetch(mass_tex, b_id).x;

    float force = G * a_mass*b_mass / dist2;

    force_vec = normalize(b_pos - a_pos) * force;
//    if(row%2 == col%2)
//        force_vec = vec3(1.0);
//    else
//        force_vec = vec3(0.0);

    gl_Position = vec4(coord, 0.0, 1.0);
}

)";

std::string force_calc_fragment_code_ = R"(
#version 430 core

out vec4 force_out;
in vec3 force_vec;

void main()
{
//    force_out = vec4(force_vec, 1.0);
    force_out = vec4(abs(force_vec) * 100000000.0f, 1.0);
}
)";

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
                              * glm::vec4{dist, 0.0f, 0.0f, 1.0f}) / 1.0f;

        glm::vec3 speed{position.y, -position.x, rand_1_1<float>()/10.0f};
        speed *= dist / 10000.0f;
        float mass = rand_0_1<float>()/sqrtf(dist) / 1.0f;

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

    init_bodies(bodies, 1024);

//    BodiesCalculatorGlSim calc_gl(bodies);
    BodiesCalculator calc(bodies);
    Renderer renderer(bodies);

    VertexArrayObject vao;
//    VertexBufferObject vbo;
//    std::vector<int> elements(1024);
//    vbo.load(elements);
//    vao.add_array_buffer(vbo, 0, 1);



    VertexBufferObject vbo_vel, vbo_mass;
    vbo_vel.load(bodies.get_velocities());
    vbo_mass.load(bodies.get_masses());

    GLuint tex[3];
    glGenTextures(3, tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0] );
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, renderer.vbo_pos.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, tex[1] );
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, vbo_vel.id);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, tex[2] );
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, vbo_mass.id);

    Shader v(force_calc_vertex_code_, GL_VERTEX_SHADER);
    Shader f(force_calc_fragment_code_, GL_FRAGMENT_SHADER);
    ShaderProgram force_calc_program(v, f);

    force_calc_program.set_uniform(0, 0);
    force_calc_program.set_uniform(1, 3);

    auto setup_force_calc = [&](size_t count, size_t a_chunk_id, size_t b_chunk_id){
        glViewport(0, 0, count, count);
//        glViewport(0, 0, 1024, 1024);
//        glPointSize(1024/count + 1);
        force_calc_program.set_uniform(2, int(a_chunk_id*count));
        force_calc_program.set_uniform(3, int(b_chunk_id*count));
        auto pixel_size =  2.0f / count;
        auto pixel_offset = -1.0f + pixel_size/2.0f;
        force_calc_program.set_uniform(4, pixel_offset);
        force_calc_program.set_uniform(5, pixel_size);

        vao.bind();
        glDrawArrays(GL_POINTS, 0, count*(count+1)/2);
        vao.unbind();
    };

    renderer.program.use();
    renderer.program.set_uniform(0, 0);
    renderer.program.set_uniform(1, 1);
    renderer.program.set_uniform(2, 2);

    glfw.update();
    std::tie(width, height) = glfw.get_dimensions();

    while(glfw.update()) {
        calc.update();
//        calc.update_collisions();
//        calc_gl.copy_from_bodies();
//        calc_gl.update();
//        calc_gl.copy_to_bodies();
        vbo_vel.update(bodies.get_velocities(), bodies.get_count());
        vbo_mass.update(bodies.get_masses(), bodies.get_count());

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPointSize(1);

        force_calc_program.use();
        renderer.update();

        setup_force_calc(bodies.get_count(), 0, 0);
        glViewport(0, 0, width, height);
        renderer.render();
    }


    return 0;
}
