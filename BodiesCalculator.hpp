#pragma once
#include "Bodies.hpp"

#include <future>
#include <glm/gtx/norm.hpp>

class BodiesCalculator {
    float G = 0.000000001;
    Bodies& bodies;

    using pairs_vec_t = std::vector<std::pair<size_t, size_t>>;
    using pairs_vec_vec_t = std::vector<pairs_vec_t>;
    pairs_vec_vec_t pairs;

    auto pairs_view() {
        return pairs | std::views::take(bodies.get_count() - 1) | std::views::join;
    }
    auto pairs_count() {
        return bodies.get_count() * (bodies.get_count() + 1) / 2;
    }

    void calculate_pairs(size_t count) {
        for(auto x : std::views::iota((size_t)1, count)) {
            pairs.push_back({});
            for(auto y : std::views::iota((size_t)0, x))
                pairs.back().push_back({x, y});
        }
    }

    glm::vec3 calc_force(Bodies::Body a, Bodies::Body b) {
        auto dist2 = glm::distance2(a.position, b.position);
        if(dist2 == 0) return {};
        auto F = G * a.mass*b.mass / glm::distance2(a.position, b.position);
        return glm::normalize(b.position-a.position) * F;
    }

    void update_velocities_lazy_parallel() {
        std::vector<glm::vec3> forces{bodies.get_count()};
        std::vector<std::future<std::vector<glm::vec3>>> futures;

        auto chunks = pairs_view() | std::views::chunk(pairs_count() / 8 + 1);
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
            for(auto [f, f_] : std::views::zip(forces, new_forces)) {
                f += f_;
            }
        }

        for(auto [id, f] : forces | std::views::enumerate) {
            auto a = bodies.get(id);
            a.velocity += f / a.mass;
        }
    }

    void apply_force(Bodies::Body a, Bodies::Body b, const glm::vec3& force) {
        if(a.mass *b.mass  == 0)
            return;

        a.velocity += force / a.mass;
        b.velocity -= force / b.mass;
    }

    void update_velocities() {
        std::vector<glm::vec3> forces;

        for(auto [a, b] : pairs_view()) {
            forces.push_back(calc_force(bodies.get(a),
                                        bodies.get(b)));
        }


        for(std::tuple<std::pair<size_t, size_t>&, glm::vec3&> pf
             : std::views::zip(pairs_view(), forces)) {
            apply_force(bodies.get(std::get<0>(pf).first),
                        bodies.get(std::get<0>(pf).second),
                        std::get<1>(pf));
        }
    }

    void update_positions() {
        for(auto pvms : bodies.view())
            std::get<0>(pvms) += std::get<1>(pvms);
    }

    bool detect_collision(Bodies::Body a, Bodies::Body b) {
        if(a.mass*b.mass == 0)
            return false;

        auto dist = a.radius + b.radius;

        return dist*dist > glm::distance2(a.position, b.position);
    }

    void resolve_collision(Bodies::Body a, Bodies::Body b) {
        a.position += (b.position - a.position) * b.mass / (b.mass + a.mass);
        auto impulse = a.velocity*a.mass + b.velocity*b.mass;
        a.mass += b.mass;
        a.velocity = impulse/a.mass;

        b.position = glm::vec3{10.0f};
        b.velocity = glm::vec3{0.0f};
        b.mass = 0;
    }

    void detect_collisions_lazy_parallel() {
        pairs_vec_vec_t collisions;
        std::vector<std::future<pairs_vec_t>> futures;

        auto chunks = pairs_view() | std::views::chunk(pairs_count() / 8 + 1);
        for(auto chunk : chunks) {
            futures.push_back(std::async([this](auto chunk) -> pairs_vec_t{
                pairs_vec_t collisions;
                for(auto [a_, b_] : chunk) {
                    auto a = bodies.get(a_);
                    auto b = bodies.get(b_);
                    if(detect_collision(a, b))
                        collisions.push_back({a_, b_});
                }
                return collisions;
            }, chunk));
        }

        for(auto& f : futures) {
            collisions.push_back(f.get());
        }
        std::vector<size_t> updated;
        std::vector<size_t> removed;
        for(auto [a_, b_] : collisions | std::views::join){
            auto a = bodies.get(a_);
            auto b = bodies.get(b_);
            resolve_collision(a, b);
            updated.push_back(a_);
            removed.push_back(b_);
        }
        for(auto u : updated) {
            bodies.update(bodies.get(u));
        }
        for(auto r : removed) {
            bodies.remove(bodies.get(r));
        }
    }

    void detect_collisions() {
        std::vector<size_t> updated;
        std::vector<size_t> removed;
        for(auto [a_, b_] : pairs_view()) {
            auto a = bodies.get(a_);
            auto b = bodies.get(b_);
            if(!detect_collision(a, b))
                continue;
            resolve_collision(a, b);
            updated.push_back(a_);
            removed.push_back(b_);
        }
        for(auto u : updated) {
            bodies.update(bodies.get(u));
        }
        for(auto r : removed) {
            bodies.remove(bodies.get(r));
        }
    }

    void halt_escapers() {
        float edge = 10.0f;
        for(auto i : std::views::iota((size_t)0, bodies.get_count())) {
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
public:
    BodiesCalculator(Bodies& bodies)
        : bodies(bodies)
    {
        calculate_pairs(bodies.get_count());
    }

    void update() {
//        update_velocities_lazy_parallel();
        update_velocities();
        update_positions();
        detect_collisions_lazy_parallel();
//        detect_collisions();
        halt_escapers();
    }
};
