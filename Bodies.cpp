#include "Bodies.hpp"

#include <glm/gtx/norm.hpp>

float Body::mass_to_radius(float val) {
    return std::pow(val/50.0f, 0.4f)/100.0f;
}

bool Body::operator==(const Body &that) const {
    return &position == &that.position;
}

bool Body::operator!=(const Body &that) const {
    return !(*this == that);
}

void Bodies::add(glm::vec4 p, glm::vec4 v, float m) {
    if(count == positions.size()) {
        positions.push_back({});
        velocities.push_back({});
        masses.push_back({});
        radii.push_back({});
    }
    auto last = get(count++);
    last.position = p;
    last.velocity = v;
    last.mass = m;
    update(last);
}

Body Bodies::get(size_t id) {
    return {
        positions.at(id),
        velocities.at(id),
        masses.at(id),
        radii.at(id)
    };
}

void Bodies::update(Body b) {
    b.radius = Body::mass_to_radius(b.mass);
    radius_max = std::max(0.0f, b.radius);
}

void Bodies::remove(Body b) {
    auto last = get(--count);
    if(last != b) {
        b.position = last.position;
        b.velocity = last.velocity;
        b.mass = last.mass;
        b.radius = last.radius;
    }
    last.position = glm::vec4{0.0f};
    last.velocity = glm::vec4{0.0f};
    last.mass = 0.0f;
    last.radius = 0.0f;
}

size_t Bodies::get_count() const {
    return count;
}

float Bodies::get_radius_max() const {
    return radius_max;
}

const std::vector<glm::vec4> &Bodies::get_positions() const {
    return positions;
}

std::vector<glm::vec4> &Bodies::get_positions() {
    return positions;
}

const std::vector<glm::vec4> &Bodies::get_velocities() const {
    return velocities;
}

std::vector<glm::vec4> &Bodies::get_velocities() {
    return velocities;
}

const std::vector<float> &Bodies::get_masses() const {
    return masses;
}

std::vector<float> &Bodies::get_masses() {
    return masses;
}

const std::vector<float> &Bodies::get_radii() const {
    return radii;
}

std::vector<float> &Bodies::get_radii() {
    return radii;
}
