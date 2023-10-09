#include "CPUGPUComputeRoutine.hpp"

CPUGPUComputeRoutine::CPUGPUComputeRoutine(Bodies &bodies, ArrayBufferObject &positions_out, ArrayBufferObject &radii_out, float G)
    : bodies(bodies)
    , vbo_positions_out(positions_out)
    , rad_out(radii_out)
    , G(G) {
    vbo_position_calc_in.bind().init<glm::vec4>(bodies.get_count());
    vbo_velocities_calc_in.bind().init<glm::vec4>(bodies.get_count());
    vbo_mass_calc_in.bind().init<float>(bodies.get_count());
    vbo_velocities_calc_out.bind().init<glm::vec4>(bodies.get_count());
    gravity_compute.set_vbos(vbo_position_calc_in,
                             vbo_velocities_calc_in,
                             vbo_mass_calc_in,
                             vbo_positions_out,
                             vbo_velocities_calc_out);
}

void CPUGPUComputeRoutine::compute() {
    compute_collisions_cpu(bodies);
    rad_out.bind().update(bodies.get_radii());

    vbo_position_calc_in.bind().update(bodies.get_positions(), bodies.get_count());
    vbo_velocities_calc_in.bind().update(bodies.get_velocities(), bodies.get_count());
    vbo_mass_calc_in.bind().update(bodies.get_masses(), bodies.get_count());

    gravity_compute.calculate(bodies.get_count(), G);

    vbo_positions_out.bind().download(bodies.get_positions(), bodies.get_count());
    vbo_velocities_calc_out.bind().download(bodies.get_velocities(), bodies.get_count());
}
