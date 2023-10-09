#pragma once

#include "ComputeGPU.hpp"
#include "ComputeCPU.hpp"

struct CPUGPUComputeRoutine {
    Bodies& bodies;
    ArrayBufferObject &vbo_positions_out, &rad_out;
    float G;

    ArrayBufferObject
        vbo_position_calc_in,
        vbo_velocities_calc_in,
        vbo_mass_calc_in,
        vbo_velocities_calc_out;

    GravityComputeGPU gravity_compute;

    CPUGPUComputeRoutine(Bodies& bodies,
                         ArrayBufferObject &positions_out,
                         ArrayBufferObject  &radii_out,
                         float G);

    void compute();
};
