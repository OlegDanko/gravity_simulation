#include "GLSimShaders.hpp"

void GLSimShaders::calc_forces(const std::vector<std::pair<size_t, size_t> > &elements_pairs, const position_tex_t &pos_a_in, const position_tex_t &pos_b_in, const mass_tex_t &mass_a_in, const mass_tex_t &mass_b_in, float G, forces_tile_t &forces_out) {
    for(auto [a, b] : elements_pairs) {
        auto a_pos = pos_a_in.at(a);
        auto b_pos = pos_b_in.at(b);
        auto distance2 = glm::distance2(a_pos, b_pos);

        if(distance2 == 0) {
            forces_out.at(a).at(b) = glm::vec3{0.0f};
            continue;
        }

        auto a_mass = mass_a_in.at(a);
        auto b_mass = mass_b_in.at(b);
        auto force = G * a_mass*b_mass / distance2;

        auto a_to_b_vec = glm::normalize(b_pos-a_pos);
        auto force_vec = a_to_b_vec*force;

        forces_out.at(a).at(b) = force_vec;
    }
}

void GLSimShaders::calc_velocities(const std::vector<size_t> &elements_ids, const mass_tex_t &mass_a_in, const mass_tex_t &mass_b_in, forces_tile_t &forces_in, size_t chunk_size, size_t a_chunk_id, size_t b_chunk_id, delta_v_matrix_t &delta_v_out) {
    for(auto id : elements_ids) {
        auto a_mass = mass_a_in.at(id);
        auto b_mass = mass_b_in.at(id);

        glm::vec3 force_a{0.0};
        glm::vec3 force_b{0.0};

        for(size_t i = 0; i < chunk_size; i++) {
            force_a += forces_in.at(id).at(i);
            force_b += forces_in.at(i).at(id);
        }

        auto v_a = a_mass > 0.0f ? force_a / a_mass : glm::vec3{0.0f};
        auto v_b = b_mass > 0.0f ? force_b / b_mass : glm::vec3{0.0f};

        delta_v_out
            .at(a_chunk_id)
            .at(b_chunk_id)
            .at(id) = {v_a, v_b};
    }
}

void GLSimShaders::update_pos_vel(const std::vector<size_t> &elements_ids, const delta_v_matrix_t &delta_v_in, size_t chunks_count, size_t chunk_id, const position_tex_t &pos_in, const velocity_tex_t &vel_in, position_tex_t &pos_out, velocity_tex_t &vel_out) {
    for(auto id : elements_ids) {
        glm::vec3 delta_v_plus{}, delta_v_minus{};
        for(size_t chunk_other_id = 0; chunk_other_id  < chunks_count; chunk_other_id++) {
            delta_v_plus += delta_v_in
                                .at(chunk_id)
                                .at(chunk_other_id)
                                .at(id).a;
            delta_v_minus += delta_v_in
                                 .at(chunk_other_id)
                                 .at(chunk_id)
                                 .at(id).b;
        }

        auto v = vel_in.at(id) + delta_v_plus - delta_v_minus;

        vel_out.at(id) = v;
        pos_out.at(id) = pos_in.at(id) + v;
    }
}
