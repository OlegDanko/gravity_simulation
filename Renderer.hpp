#pragma once
#include "Bodies.hpp"
#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>

class Renderer {
    VertexArrayObject vao;
    VertexBufferObject vbo_vertices;
    ShaderProgram program;
public:
    void prepare_vertices();

    Renderer(VertexBufferObject& vbo_positions, VertexBufferObject& vbo_radii);

    void render(size_t count);
};
