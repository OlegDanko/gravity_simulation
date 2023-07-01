#pragma once

#include "ComputeShaderBase.hpp"

struct GravityComputeShader : ComputeShaderBase<5> {
    static const std::string code;

    GravityComputeShader();

    void set_position_in(VertexBufferObject& vbo);
    void set_velocity_in(VertexBufferObject& vbo);
    void set_mass_in(VertexBufferObject& vbo);
    void set_position_out(VertexBufferObject& vbo);
    void set_velocity_out(VertexBufferObject& vbo);

    void set_elements_count(GLint val);
    void set_G(GLfloat val);
};
