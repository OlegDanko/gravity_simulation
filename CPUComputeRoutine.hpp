#pragma once

#include "ComputeCPU.hpp"
#include "VertexArrayObject.hpp"

struct CPUComputeRoutine {
    Bodies& bodies;
    VertexBufferObject &pos_out, &rad_out;
    float G;

    CPUComputeRoutine(Bodies& bodies,
                      VertexBufferObject &positions_out,
                      VertexBufferObject &radii_out,
                      float G);

    void compute();
};
