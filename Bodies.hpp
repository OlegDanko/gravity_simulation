#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <ranges>

struct Body {
    static float mass_to_radius(float val);
    glm::vec4 &position;
    glm::vec4 &velocity;
    float &mass;
    float &radius;
    bool operator==(const Body &that) const;
    bool operator!=(const Body &that) const;
};

template<> struct std::hash<Body> {
    size_t operator()(const Body& b) const { return (size_t)&b.position; }
};

class Bodies {
    std::vector<glm::vec4> positions;
    std::vector<glm::vec4> velocities;
    std::vector<float> masses;
    std::vector<float> radii;
    size_t count{0};
    float radius_max = 0.0f;
public:
    auto view() {
        auto make_body = [](auto& p, auto& v, auto& m, auto& r) {
            return Body{p, v, m, r};
        };

        return std::views::zip_transform(make_body,
                                         positions,
                                         velocities,
                                         masses,
                                         radii)
               | std::views::take(count);
    }

    void add(glm::vec4 p, glm::vec4 v, float m);
    Body get(size_t id);
    void update(Body b);
    void remove(Body b);
    size_t get_count() const;
    float get_radius_max() const;
    const std::vector<glm::vec4>& get_positions() const;
    std::vector<glm::vec4>& get_positions();
    const std::vector<glm::vec4>& get_velocities() const;
    std::vector<glm::vec4>& get_velocities();
    const std::vector<float>& get_masses() const;
    std::vector<float>& get_masses();
    const std::vector<float>& get_radii() const;
    std::vector<float>& get_radii();
};
