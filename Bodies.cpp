#include "Bodies.hpp"

#include <glm/gtx/norm.hpp>

float Bodies::mass_to_radius(float val) {
    return std::pow(val/50, 0.4)/100;
}

Bodies::Body Bodies::get(size_t i) {
    return {i,
            positions.at(i),
            velocities.at(i),
            masses.at(i),
            radii.at(i) };
}

void Bodies::update(Body b) {
    b.radius = mass_to_radius(b.mass);
    radius_max = std::max(radius_max, b.radius);
}

void Bodies::remove(Body b) {
    auto last = get(--count);
    b.position = glm::vec3{1000.0f};
    b.velocity = {};
    b.mass = {};
    b.radius = {};
    std::swap(b.position, last.position);
    std::swap(b.velocity, last.velocity);
    std::swap(b.mass, last.mass);
    std::swap(b.radius, last.radius);
}

void Bodies::add(const glm::vec3 &pos, const glm::vec3 &vel, float m) {
    positions.push_back(pos);
    velocities.push_back(vel);
    masses.push_back(m);
    radii.push_back(mass_to_radius(m));
    radius_max = std::max(radius_max, radii.back());
    count++;
}

size_t Bodies::get_count() const { return count; }

float Bodies::get_radius_max() const { return radius_max; }

const std::vector<glm::vec3> &Bodies::get_positions() const { return positions; }
const std::vector<glm::vec3> &Bodies::get_velocities() const { return velocities; }
const std::vector<float> &Bodies::get_masses() const { return masses; }
const std::vector<float> &Bodies::get_radii() const { return radii; }
std::vector<glm::vec3> &Bodies::get_positions() { return positions; }
std::vector<glm::vec3> &Bodies::get_velocities() { return velocities; }
std::vector<float> &Bodies::get_masses() { return masses; }
std::vector<float> &Bodies::get_radii() { return radii; }
