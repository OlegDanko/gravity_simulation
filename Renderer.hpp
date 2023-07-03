#pragma once
#include "Bodies.hpp"
#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>

class Renderer {
    VertexArrayObject vao_circles;
    VertexArrayObject vao_points;
    VertexBufferObject vbo_vertices;
    ShaderProgram program_circles;
    ShaderProgram program_points;
public:
    void prepare_vertices();

    Renderer(VertexBufferObject& vbo_positions, VertexBufferObject& vbo_radii);

    void render(size_t count, const glm::mat4& VP, const glm::vec3& cam_pos);
};
