#include "CPUComputeRoutine.hpp"

CPUComputeRoutine::CPUComputeRoutine(Bodies &bodies, VertexBufferObject &positions_out, VertexBufferObject &radii_out, float G)
    : bodies(bodies)
    , pos_out(positions_out)
    , rad_out(radii_out)
    , G(G) {}

void CPUComputeRoutine::compute() {
    compute_collisions_cpu(bodies);
    rad_out.update(bodies.get_radii());

    compute_gravity_cpu_parallel(bodies, G, 8);
    pos_out.update(bodies.get_positions());
}
