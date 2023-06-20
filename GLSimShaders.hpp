#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

template<typename T>
using texture1d_t = std::vector<T>;
template<typename T>
using texture2d_t = std::vector<texture1d_t<T>>;

struct vec3_pair {
    glm::vec3 a;
    glm::vec3 b;
};

using delta_v_matrix_t = std::vector<std::vector<std::vector<vec3_pair>>>;

using position_tex_t = texture1d_t<glm::vec3>;
using velocity_tex_t = texture1d_t<glm::vec3>;
using mass_tex_t = texture1d_t<float>;
using radius_tex_t = texture1d_t<float>;
using forces_tile_t = texture2d_t<glm::vec3>;


struct GLSimShaders {
    static void calc_forces(
        const std::vector<std::pair<size_t, size_t>>& elements_pairs,
        const position_tex_t& pos_a_in,
        const position_tex_t& pos_b_in,
        const mass_tex_t& mass_a_in,
        const mass_tex_t& mass_b_in,
        float G,
        forces_tile_t& forces_out
        );

    static void calc_velocities(
        const std::vector<size_t>& elements_ids,
        const mass_tex_t& mass_a_in,
        const mass_tex_t& mass_b_in,
        forces_tile_t& forces_in,
        size_t chunk_size,
        size_t a_chunk_id,
        size_t b_chunk_id,
        delta_v_matrix_t& delta_v_out
        );

    static void update_pos_vel(
        const std::vector<size_t>& elements_ids,
        const delta_v_matrix_t& delta_v_in,
        size_t chunks_count,
        size_t chunk_id,
        const position_tex_t& pos_in,
        const velocity_tex_t& vel_in,
        position_tex_t& pos_out,
        velocity_tex_t& vel_out
        );
};
