#pragma once
#include "Bodies.hpp"
#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>

class Renderer {
    Bodies& bodies;
    VertexArrayObject vao;
    VertexBufferObject vbo_pos, vbo_size, vbo_vert;
    ShaderProgram program;
public:
    Renderer(Bodies& bodies);

    void update();

    void render();
};
