#include "ComputeCPU.hpp"
#include "Utils.hpp"
#include "ComputeCPUFunctions.hpp"

void compute_collisions_cpu(Bodies &bodies) {
    auto candidates = get_collision_candidates(bodies.view(), 4.0f);
    auto collisions = detect_collisions(candidates | std::views::all);

    auto deref = [](auto &ptr) -> auto& { return *ptr; };
    auto [updated, removed] = resolve_collisions(collisions | std::views::transform(deref));
    for(auto u : updated) {
        bodies.update(u);
    }
    for(auto r : removed) {
        bodies.remove(r);
    }
}

void compute_gravity_cpu(Bodies &bodies, float G) {
    auto enum_bodies = bodies.view() | std::views::enumerate;

    UniquePairs pairs(enum_bodies);
    auto forces = calc_forces(pairs, bodies.get_count(), G);

    apply_force(bodies.view(), forces | std::views::all);
}

void compute_gravity_cpu_parallel(Bodies &bodies, float G, size_t thread_count) {
    auto enum_bodies = bodies.view() | std::views::enumerate;

    UniquePairs pairs(enum_bodies);
    auto forces = calc_forces(pairs, bodies.get_count(), thread_count, G);

    apply_force(bodies.view(), forces | std::views::all);
}
