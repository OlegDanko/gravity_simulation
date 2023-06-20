#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <ranges>

class Bodies {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> velocities;
    std::vector<float> masses;
    std::vector<float> radii;

    size_t count{0};
    float radius_max = {0.0f};

    static float mass_to_radius(float val);
public:
    auto view() {
        return std::views::zip(positions, velocities, masses, radii)
               | std::views::take(count);
    }

    struct Body{
        size_t i;
        glm::vec3& position;
        glm::vec3& velocity;
        float& mass;
        float& radius;
    };

    Body get(size_t i);

    void update(Body b);

    void remove(Body b);

    void add(const glm::vec3& pos,
             const glm::vec3& vel,
             float m);

    size_t get_count() const;
    float get_radius_max() const;
    const std::vector<glm::vec3>& get_positions() const;
    const std::vector<glm::vec3>& get_velocities() const;
    const std::vector<float>& get_masses() const;
    const std::vector<float>& get_radii() const;
    std::vector<glm::vec3>& get_positions();
    std::vector<glm::vec3>& get_velocities();
    std::vector<float>& get_masses();
    std::vector<float>& get_radii();
};
