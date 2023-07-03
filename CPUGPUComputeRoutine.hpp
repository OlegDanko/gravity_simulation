#pragma once

#include "ComputeGPU.hpp"
#include "ComputeCPU.hpp"

struct CPUGPUComputeRoutine {
    Bodies& bodies;
    VertexBufferObject &vbo_positions_out, &rad_out;
    float G;

    VertexBufferObject
        vbo_position_calc_in,
        vbo_velocities_calc_in,
        vbo_mass_calc_in,
        vbo_velocities_calc_out;

    GravityComputeGPU gravity_compute;

    CPUGPUComputeRoutine(Bodies& bodies,
                         VertexBufferObject &positions_out,
                         VertexBufferObject  &radii_out,
                         float G);

    void compute();
};
