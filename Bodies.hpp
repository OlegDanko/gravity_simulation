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

    static float mass_to_radius(float val);
public:
//    std::ranges::zip_view<std::vector<glm::vec3>,
//                          std::vector<glm::vec3>,
//                          std::vector<float>,
//                          std::vector<float>>
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
    const std::vector<glm::vec3>& get_positions() const;
    const std::vector<glm::vec3>& get_velocities() const;
    const std::vector<float>& get_masses() const;
    const std::vector<float>& get_radii() const;
};
