#pragma once

#include "ComputeCPU.hpp"
#include "gl_context/VertexArrayObject.hpp"

struct CPUComputeRoutine {
    Bodies& bodies;
    ArrayBufferObject &pos_out, &rad_out;
    float G;
public:
    CPUComputeRoutine(Bodies& bodies,
                      ArrayBufferObject &positions_out,
                      ArrayBufferObject &radii_out,
                      float G);

    void compute();
};
