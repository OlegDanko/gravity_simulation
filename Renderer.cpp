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

void Renderer::prepare_vertices() {
    glm::vec4 left{1.0, 0.0, 0.0, 1.0f};
    std::vector<glm::vec3> vertices;
    for(auto i : vws::iota(0, 12)) {
        auto mat = glm::rotate(M_PIf*i/6,glm::vec3{ 0.0f, 0.0f, 1.0f});
        auto vec = mat*left;
        vertices.push_back(glm::vec3(vec));
    }

    vbo_vertices.upload(vertices);
}

Renderer::Renderer(VertexBufferObject &vbo_positions, VertexBufferObject &vbo_radii)
    : program(Shader::make_vertex(vertex_code),
              Shader::make_fragment(fragment_code)) {
    prepare_vertices();
    vao.add_array_buffer(vbo_positions, 0, 4, 0, 0, 1);
    vao.add_array_buffer(vbo_radii, 1, 1, 0, 0, 1);
    vao.add_array_buffer(vbo_vertices, 2, 3);
}

void Renderer::render(size_t count) {
    program.use();
    vao.bind();
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 12, count);
    vao.unbind();
}
