#include "ComputeGPU.hpp"

GravityComputeGPU::GravityComputeGPU(VertexBufferObject &position_in, VertexBufferObject &velocity_in, VertexBufferObject &mass_in, VertexBufferObject &position_out, VertexBufferObject &velocity_out) {
    shader.set_position_in(position_in);
    shader.set_velocity_in(velocity_in);
    shader.set_mass_in(mass_in);
    shader.set_position_out(position_out);
    shader.set_velocity_out(velocity_out);
}

void GravityComputeGPU::calculate(size_t bodies_count, float G) {
    shader.use_program();
    shader.set_elements_count(bodies_count);
    shader.set_G(G);
    shader.dispatch(bodies_count, 1, 1);
    shader.barrier();
}
