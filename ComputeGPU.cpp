#include "ComputeGPU.hpp"

void GravityComputeGPU::set_vbos(ArrayBufferObject& position_in,
                                 ArrayBufferObject& velocity_in,
                                 ArrayBufferObject& mass_in,
                                 ArrayBufferObject& position_out,
                                 ArrayBufferObject& velocity_out) {
    shader.set_position_in(position_in);
    shader.set_velocity_in(velocity_in);
    shader.set_mass_in(mass_in);
    shader.set_position_out(position_out);
    shader.set_velocity_out(velocity_out);
}

void GravityComputeGPU::calculate(size_t bodies_count, float G) {
    if(auto program = shader.use_program(); true) {
        program.set_G(G);
        program.set_elements_count(bodies_count);
        shader.dispatch(bodies_count, 1, 1);

    }
    shader.barrier();
}
