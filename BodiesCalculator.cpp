#include "BodiesCalculator.hpp"

#include <future>
#include <iostream>

template<>
struct std::hash<glm::ivec2> {
    size_t operator()(const glm::ivec2& vec) const {
        union {
            glm::ivec2 v;
            size_t s;
        } u;
        u.v = vec;
        return u.s;
    }
};

size_t BodiesCalculator::pairs_count() {
    return bodies.get_count() * (bodies.get_count() + 1) / 2;
}

void BodiesCalculator::calculate_pairs(size_t count) {
    for(auto x : vws::iota((size_t)1, count)) {
        pairs.push_back({});
        for(auto y : vws::iota((size_t)0, x))
            pairs.back().push_back({x, y});
    }
}

glm::vec3 BodiesCalculator::calc_force(Bodies::Body a, Bodies::Body b) {
    auto dist2 = glm::distance2(a.position, b.position);
    if(dist2 == 0) return {};
    auto F = G * a.mass*b.mass / glm::distance2(a.position, b.position);
    return glm::normalize(b.position-a.position) * F;
}

void BodiesCalculator::update_velocities_lazy_parallel() {
    std::vector<glm::vec3> forces{bodies.get_count()};
    std::vector<std::future<std::vector<glm::vec3>>> futures;

    auto chunks = pairs_view() | vws::chunk(pairs_count() / 8 + 1);
    for(auto chunk : chunks) {
        futures.push_back(std::async([this](auto chunk) -> std::vector<glm::vec3> {
            std::vector<glm::vec3> forces{bodies.get_count()};
            for(auto [a, b] : chunk) {
                auto f = calc_force(bodies.get(a),
                                    bodies.get(b));
                forces.at(a) += f;
                forces.at(b) -= f;
            }
            return forces;
        }, chunk));
    }

    for(auto& f : futures) {
        auto new_forces = f.get();
        for(auto [f, f_] : vws::zip(forces, new_forces)) {
            f += f_;
        }
    }

    for(auto [id, f] : forces | vws::enumerate) {
        auto a = bodies.get(id);
        a.velocity += f / a.mass;
    }
}

void BodiesCalculator::apply_force(Bodies::Body a, Bodies::Body b, const glm::vec3 &force) {
    if(a.mass *b.mass  == 0)
        return;

    a.velocity += force / a.mass;
    b.velocity -= force / b.mass;
}

void BodiesCalculator::update_velocities() {
    std::vector<glm::vec3> forces;

    for(auto [a, b] : pairs_view()) {
        forces.push_back(calc_force(bodies.get(a),
                                    bodies.get(b)));
    }

    for(std::tuple<std::pair<size_t, size_t>&, glm::vec3&> pf
         : vws::zip(pairs_view(), forces)) {
        apply_force(bodies.get(std::get<0>(pf).first),
                    bodies.get(std::get<0>(pf).second),
                    std::get<1>(pf));
    }
}

void BodiesCalculator::update_positions() {
    for(auto pvms : bodies.view())
        std::get<0>(pvms) += std::get<1>(pvms);
}

bool BodiesCalculator::detect_collision(Bodies::Body a, Bodies::Body b) {
    if(a.mass*b.mass == 0)
        return false;

    auto dist = a.radius + b.radius;

    return dist*dist > glm::distance2(a.position, b.position);
}

void BodiesCalculator::resolve_collision(Bodies::Body a, Bodies::Body b) {
    a.position += (b.position - a.position) * b.mass / (b.mass + a.mass);
    auto impulse = a.velocity*a.mass + b.velocity*b.mass;
    a.mass += b.mass;
    a.velocity = impulse/a.mass;

    b.position = glm::vec3{10.0f};
    b.velocity = glm::vec3{0.0f};
    b.mass = 0;
}

std::vector<std::unordered_set<size_t> > BodiesCalculator::get_collision_candidates() {
    std::unordered_set<glm::ivec2> candidate_tiles;
    std::unordered_map<glm::ivec2, std::unordered_set<size_t>> tile_body_id_map;
    float tile_size = bodies.get_radius_max()*1.5f;

    for(auto i : vws::iota((size_t)0, bodies.get_count())) {
        auto b = bodies.get(i);
        int x_begin = floor((b.position.x - b.radius)/tile_size);
        int x_end = ceil((b.position.x + b.radius)/tile_size);
        int y_begin = floor((b.position.y - b.radius)/tile_size);
        int y_end = ceil((b.position.y + b.radius)/tile_size);

        for(auto [x, y] : vws::cartesian_product(vws::iota(x_begin, x_end),
                                                  vws::iota(y_begin, y_end))) {
            glm::ivec2 tile{x, y};
            tile_body_id_map[tile].insert(i);
            if(tile_body_id_map[tile].size() > 1)
                candidate_tiles.insert(tile);
        }
    }

    std::vector<std::unordered_set<size_t>> candidates;
    for(auto t : candidate_tiles) {
        candidates.push_back(std::move(tile_body_id_map[t]));
    }
    return candidates;
}

std::unordered_set<BodiesCalculator::collision_chain> BodiesCalculator::detect_collisions(std::vector<std::unordered_set<size_t> > tiles) {
    std::unordered_set<collision_chain> collision_chains;
    std::unordered_map<size_t, collision_chain> collisions_per_id_map;

    for(auto& ids_set : tiles)
        for(auto [a, b] : vws::cartesian_product(ids_set, ids_set)) {
            if(a >= b) continue;
            if(!detect_collision(bodies.get(a), bodies.get(b)))
                continue;

            collision_chain chain;

            auto chain_a = collisions_per_id_map[a];
            auto chain_b = collisions_per_id_map[b];

            if(!chain_a && !chain_b) {
                chain = std::make_shared<std::unordered_set<size_t>>();
                collision_chains.insert(chain);
                collisions_per_id_map[a] = chain;
                collisions_per_id_map[b] = chain;
            } else if (!chain_a) {
                collisions_per_id_map[a] = chain_b;
                chain = chain_b;
            } else if(!chain_b) {
                collisions_per_id_map[b] = chain_a;
                chain = chain_a;
            } else {
                chain = chain_a;
                if(chain_a != chain_b) {
                    chain_a->insert(chain_b->begin(), chain_b->end());
                    collisions_per_id_map[b] = chain_a;
                    collision_chains.erase(chain_b);
                }
            }
            chain->insert(a);
            chain->insert(b);
        }
    return collision_chains;
}

void BodiesCalculator::resolve_collisions(std::unordered_set<collision_chain> collision_chains) {
    std::vector<size_t> updated;
    std::vector<size_t> removed;

    for(auto chain : collision_chains) {
        resolve_collision(chain->begin(), chain->end());
        updated.push_back(*chain->begin());

        for(auto it = std::next(chain->begin()); it != chain->end(); ++it)
            removed.push_back(*it);
    }

    for(auto u : updated) {
        bodies.update(bodies.get(u));
    }
    for(auto r : removed) {
        bodies.remove(bodies.get(r));
    }
}

void BodiesCalculator::update_collisions() {
    resolve_collisions(detect_collisions(get_collision_candidates()));
}

void BodiesCalculator::halt_escapers() {
    float edge = 10.0f;
    for(auto i : vws::iota((size_t)0, bodies.get_count())) {
        auto a = bodies.get(i);
        bool flag = false;
        if(a.position.x > edge) {
            a.position.x = edge;
            flag = true;
        }
        if(a.position.x < -edge) {
            a.position.x = -edge;
            flag = true;
        }
        if(a.position.y > edge) {
            a.position.y = edge;
            flag = true;
        }
        if(a.position.y < -edge) {
            a.position.y = -edge;
            flag = true;
        }

        if(flag)
            a.velocity = glm::vec3{0.0f};
    }
}

BodiesCalculator::BodiesCalculator(Bodies &bodies)
    : bodies(bodies)
{
    calculate_pairs(bodies.get_count());
}

void BodiesCalculator::update() {
    update_collisions();
    update_velocities_lazy_parallel();
    //        update_velocities();
    update_positions();
    halt_escapers();
}
