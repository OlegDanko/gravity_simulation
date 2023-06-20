#pragma once
#include "Bodies.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <glm/gtx/norm.hpp>

namespace vws = std::views;

class BodiesCalculator {
    float G = 0.000000001;
    Bodies& bodies;

    using pairs_vec_t = std::vector<std::pair<size_t, size_t>>;
    using pairs_vec_vec_t = std::vector<pairs_vec_t>;
    pairs_vec_vec_t pairs;

    auto pairs_view() {
        return pairs | vws::take(bodies.get_count() - 1) | vws::join;
    }
    size_t pairs_count();

    void calculate_pairs(size_t count);

    glm::vec3 calc_force(Bodies::Body a, Bodies::Body b);

    void update_velocities_lazy_parallel();

    void apply_force(Bodies::Body a, Bodies::Body b, const glm::vec3& force);

    bool detect_collision(Bodies::Body a, Bodies::Body b);

    template<typename IT>
    void resolve_collision(IT begin, IT end) {
        auto pos_mass_accum = [this](glm::vec3 pm, const size_t id){
            auto b = bodies.get(id);
            return pm + b.position*b.mass;
        };
        auto momentum_accum = [this](glm::vec3 m, const size_t id){
            auto b = bodies.get(id);
            return m + b.velocity*b.mass;
        };
        auto mass_accum = [this](float m, const size_t id){
            auto b = bodies.get(id);
            return m + b.mass;
        };

        auto mass = std::accumulate(begin, end, 0.0f, mass_accum);
        auto position = std::accumulate(begin, end, glm::vec3{0.0f}, pos_mass_accum) / mass;
        auto velocity = std::accumulate(begin, end, glm::vec3{0.0f}, momentum_accum) / mass;

        Bodies::Body a = bodies.get(*begin);
        a.position = position;
        a.velocity = velocity;
        a.mass = mass;
    }

    void resolve_collision(Bodies::Body a, Bodies::Body b);

    std::vector<std::unordered_set<size_t>> get_collision_candidates();

    using collision_chain = std::shared_ptr<std::unordered_set<size_t>>;
    std::unordered_set<collision_chain>
    detect_collisions(std::vector<std::unordered_set<size_t>> tiles);

    void resolve_collisions(std::unordered_set<collision_chain> collision_chains);


    void halt_escapers();
public:
    BodiesCalculator(Bodies& bodies);
    void update_collisions();
    void update_velocities();
    void update_positions();
    void update();
};
