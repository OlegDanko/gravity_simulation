#include "BodiesCalculatorGlSim.hpp"

BodiesCalculatorGlSim::BodiesCalculatorGlSim(Bodies &bodies) : bodies(bodies) {
    // round up
    auto chunks_count = (bodies.get_count() + chunk_size - 1) / chunk_size;

    std::fill_n(std::back_inserter(chunks), chunks_count, Chunk{});

    for(auto& chunk : chunks) {
        resizer<position_tex_t>::resize(chunk.position.read, chunk_size);
        resizer<position_tex_t>::resize(chunk.position.write, chunk_size);
        resizer<velocity_tex_t>::resize(chunk.velocity.read, chunk_size);
        resizer<velocity_tex_t>::resize(chunk.velocity.write, chunk_size);
        resizer<mass_tex_t>::resize(chunk.mass.read, chunk_size);
        resizer<mass_tex_t>::resize(chunk.mass.write, chunk_size);
        resizer<delta_v_matrix_t>::resize(velocities_delta,
                                          chunks_count,
                                          chunks_count,
                                          chunk_size);
    }

    resizer<forces_tile_t>::resize(forces_tile, chunk_size, chunk_size);


    size_t i = 0;
    std::generate_n(std::back_inserter(elements), chunk_size, [&]{ return i++; });

    for(auto a : std::views::iota((size_t)0, chunk_size))
        for(auto b : std::views::iota((size_t)0, a+1))
            elements_pairs.push_back({a, b});

    copy_from_bodies();
}

void BodiesCalculatorGlSim::copy_from_bodies() {
    copy_bodies<copy_from>();
}

void BodiesCalculatorGlSim::copy_to_bodies() {
    copy_bodies<copy_to>();
}

void BodiesCalculatorGlSim::update() {
    auto chunks_count = (bodies.get_count() + chunk_size - 1) / chunk_size;
    auto chunks_enum = std::views::enumerate(chunks) | std::views::take(chunks_count);
    auto chunk_matrix = std::views::cartesian_product(chunks_enum, chunks_enum);

    for(auto [a, b] : chunk_matrix) {
        auto& [a_id, a_ch] = a;
        auto& [b_id, b_ch] = b;
        GLSimShaders::calc_forces(elements_pairs,
                                  a_ch.position.read,
                                  b_ch.position.read,
                                  a_ch.mass.read,
                                  b_ch.mass.read,
                                  0.000000001,
                                  forces_tile);

        GLSimShaders::calc_velocities(elements,
                                      a_ch.mass.read,
                                      b_ch.mass.read,
                                      forces_tile,
                                      chunk_size,
                                      a_id,
                                      b_id,
                                      velocities_delta);
    }
    for(auto [id, ch] : chunks_enum) {
        GLSimShaders::update_pos_vel(elements,
                                     velocities_delta,
                                     chunks.size(),
                                     id,
                                     ch.position.read,
                                     ch.velocity.read,
                                     ch.position.write,
                                     ch.velocity.write);
        ch.position.swap();
        ch.velocity.swap();
    }
}
