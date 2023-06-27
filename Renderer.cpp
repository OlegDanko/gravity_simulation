#include "Renderer.hpp"

#include <glm/gtx/transform.hpp>

namespace vws = std::views;

std::string vertex_code = R"(
#version 430 core

layout (location = 0) in vec4 position;
layout (location = 1) in float size;
layout (location = 2) in vec3 vert;

void main() {
    gl_Position = vec4(position.xyz + vert*size, 1.0);
}
)";

std::string fragment_code = R"(
#version 430 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(vec3(0.0f), 1.0f);
}
)";

Renderer::Renderer(Bodies &bodies)
    : bodies(bodies)
    , program(Shader::make_vertex(vertex_code),
              Shader::make_fragment(fragment_code))

{
    vbo_pos.upload(bodies.get_positions());
    vbo_size.upload(bodies.get_radii());

    glm::vec4 left{1.0, 0.0, 0.0, 1.0f};
    std::vector<float> vertices;
    for(auto i : vws::iota(0, 12)) {
        auto mat = glm::rotate(M_PIf*i/6,glm::vec3{ 0.0f, 0.0f, 1.0f});
        auto vec = mat*left;
        vertices.push_back(vec.x);
        vertices.push_back(vec.y);
        vertices.push_back(vec.z);
    }

    vbo_vert.upload(vertices);

    vao.add_array_buffer(vbo_pos, 0, 4, 0, 0, 1);
    vao.add_array_buffer(vbo_size, 1, 1, 0, 0, 1);
    vao.add_array_buffer(vbo_vert, 2, 3);
}

void Renderer::upload_positions() {
    vbo_pos.update(bodies.get_positions(), bodies.get_count());
}

void Renderer::upload_radii() {
    vbo_size.update(bodies.get_radii(), bodies.get_count());
}

void Renderer::update() {
    upload_positions();
    upload_radii();
}

void Renderer::render() {
    program.use();
    vao.bind();
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 12, bodies.get_count());
    vao.unbind();
}
