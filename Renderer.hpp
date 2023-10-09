#pragma once
#include "Bodies.hpp"
#include <gl_context/ShaderProgram.hpp>
#include <gl_context/VertexArrayObject.hpp>

class Renderer {
    VertexArrayObject vao_circles;
    VertexArrayObject vao_points;
    ArrayBufferObject vbo_vertices;
    ShaderProgram program_circles;
    ShaderProgram program_points;
public:
    void prepare_vertices();

    Renderer(ArrayBufferObject& vbo_positions, ArrayBufferObject& vbo_radii);

    void render(size_t count, const glm::mat4& VP, const glm::vec3& cam_pos);
};
