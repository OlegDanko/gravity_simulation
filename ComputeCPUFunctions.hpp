#pragma once

#include "Utils.hpp"
#include "Bodies.hpp"
#include <unordered_set>
#include <unordered_map>
#include <numeric>
#include <future>
#include <glm/gtx/norm.hpp>

template<typename BODIES_VIEW>
std::vector<std::unordered_set<Body>> get_collision_candidates(BODIES_VIEW bodies,
                                                               float radius_max) {
    std::unordered_set<glm::ivec2> candidate_tiles;
    std::unordered_map<glm::ivec2, std::unordered_set<Body>> tile_body_id_map;
    float tile_size = radius_max * 1.5f;

    for(Body b : bodies) {
        int x_begin = floor((b.position.x - b.radius)/tile_size);
        int x_end = ceil((b.position.x + b.radius)/tile_size);
        int y_begin = floor((b.position.y - b.radius)/tile_size);
        int y_end = ceil((b.position.y + b.radius)/tile_size);

        for(auto [x, y] : std::views::cartesian_product(std::views::iota(x_begin, x_end),
                                                  std::views::iota(y_begin, y_end))) {
            glm::ivec2 tile{x, y};
            tile_body_id_map[tile].insert(b);
            if(tile_body_id_map[tile].size() > 1)
                candidate_tiles.insert(tile);
        }
    }

    std::vector<std::unordered_set<Body>> candidates;
    for(auto t : candidate_tiles) {
        candidates.push_back(std::move(tile_body_id_map[t]));
    }
    return candidates;
}

bool detect_collision(const Body& a, const Body& b) {
    auto dist_2 = glm::distance2(a.position, b.position);
    if(dist_2 == 0)
        return false;

    auto rad_sum = a.radius + b.radius;

    return rad_sum*rad_sum > dist_2;
}

using collision_chain = std::shared_ptr<std::unordered_set<Body>>;

template<typename BODIES_GROUPS>
std::unordered_set<collision_chain> detect_collisions(BODIES_GROUPS bodies_groups) {
    std::unordered_set<collision_chain> collision_chains;
    std::unordered_map<Body, collision_chain> collisions_per_body_map;

    for(auto& bodies_set : bodies_groups)
        for(auto [a, b] : UniquePairs(bodies_set)) {

            if(!detect_collision(a, b))
                continue;

            collision_chain chain;

            auto chain_a = collisions_per_body_map[a];
            auto chain_b = collisions_per_body_map[b];

            if(!chain_a && !chain_b) {
                chain = std::make_shared<std::unordered_set<Body>>();
                collision_chains.insert(chain);
                collisions_per_body_map[a] = chain;
                collisions_per_body_map[b] = chain;
            } else if (!chain_a) {
                collisions_per_body_map[a] = chain_b;
                chain = chain_b;
            } else if(!chain_b) {
                collisions_per_body_map[b] = chain_a;
                chain = chain_a;
            } else {
                chain = chain_a;
                if(chain_a != chain_b) {
                    chain_a->insert(chain_b->begin(), chain_b->end());
                    collisions_per_body_map[b] = chain_a;
                    collision_chains.erase(chain_b);
                }
            }
            chain->insert(a);
            chain->insert(b);
        }
    return collision_chains;
}

template<typename BODIES_VIEW>
void resolve_collision(BODIES_VIEW bodies) {
    auto [position, velocity, mass] = std::accumulate(
        bodies.begin(),
        bodies.end(),
        std::tuple<glm::vec4, glm::vec4, float>{},
        [](auto vals, const Body& b){
            std::get<0>(vals) += b.position * b.mass;
            std::get<1>(vals) += b.velocity * b.mass;
            std::get<2>(vals) += b.mass;
            return std::move(vals);
        });
    position /= mass;
    velocity /= mass;

    auto b = *bodies.begin();
    b.position = position;
    b.velocity = velocity;
    b.mass = mass;
}

template<typename BODIES_GROUPS>
std::pair<std::vector<Body>, std::vector<Body>> resolve_collisions(BODIES_GROUPS collision_chains) {
    std::vector<Body> updated;
    std::vector<Body> removed;

    for(auto bodies : collision_chains) {
        resolve_collision(bodies);
        updated.push_back(*bodies.begin());

        for(auto r : bodies | std::views::drop(1))
            removed.push_back(r);
    }

    return {updated, removed};
}

glm::vec4 calc_force(Body a, Body b, float G) {
    auto dist2 = glm::distance2(a.position, b.position);
    if(dist2 == 0) return {};
    auto f = G * a.mass*b.mass / glm::distance2(a.position, b.position);
    return glm::normalize(b.position-a.position) * f;
}

template<typename ENUM_BODIES_PAIRS>
std::vector<glm::vec4> calc_forces(ENUM_BODIES_PAIRS pairs, size_t bodies_count, float G) {
    std::vector<glm::vec4> forces(bodies_count);
    for(auto [a, b] : pairs) {
        auto [a_id, a_body] = a;
        auto [b_id, b_body] = b;
        auto f = calc_force(a_body, b_body, G);
        forces.at(a_id) += f;
        forces.at(b_id) -= f;
    }

    return forces;
}

template<typename ENUM_BODIES_PAIRS>
std::vector<glm::vec4> calc_forces(ENUM_BODIES_PAIRS pairs,
                                   size_t bodies_count,
                                   size_t threads_count,
                                   float G) {
    std::vector<glm::vec4> forces(bodies_count);
    auto chunk_size = div_ceil(pairs.size(), threads_count);

    std::vector<std::future<std::vector<glm::vec4>>> futures;
    for(auto chunk_id : std::views::iota((size_t)0, threads_count)) {
        auto chunk_start = chunk_id * chunk_size;
        SimpleRange sub(pairs.it_at(chunk_start),
                        pairs.it_at(chunk_start + chunk_size));
        futures.push_back(std::async([sub, bodies_count, G](){
            return calc_forces(sub, bodies_count, G);
        }));
    }

    for(auto& ftr : futures) {
        auto new_forces = ftr.get();
        for(auto [f, f_] : std::views::zip(forces, new_forces)) {
            f += f_;
        }
    }
    return forces;
}

template<typename BODIES_VIEW, typename FORCES_VIEW>
void apply_force(BODIES_VIEW bodies, FORCES_VIEW forces) {
    for(auto [body, force] : std::views::zip(bodies, forces)) {
        auto delta_v = force / body.mass;
        body.velocity += delta_v;
        body.position += body.velocity;
    }
}
