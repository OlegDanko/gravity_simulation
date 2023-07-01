#pragma once

#include "GravityComputeShader.hpp"

class GravityComputeGPU {
    VertexArrayObject vao;
    GravityComputeShader shader;
public:
    GravityComputeGPU(VertexBufferObject& position_in,
                      VertexBufferObject& velocity_in,
                      VertexBufferObject& mass_in,
                      VertexBufferObject& position_out,
                      VertexBufferObject& velocity_out);

    void calculate(size_t bodies_count, float G);
};
