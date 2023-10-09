#pragma once

#include "GravityComputeShader.hpp"

class GravityComputeGPU {
    VertexArrayObject vao;
    GravityComputeShader shader;
public:
    void set_vbos(ArrayBufferObject& position_in,
                  ArrayBufferObject& velocity_in,
                  ArrayBufferObject& mass_in,
                  ArrayBufferObject& position_out,
                  ArrayBufferObject& velocity_out);

    void calculate(size_t bodies_count, float G);
};
