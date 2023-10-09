#include "GravityComputeShader.hpp"

const std::string GravityComputeShader::code  = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer position_in;
uniform layout(rgba32f, binding = 1) readonly imageBuffer velocity_in;
uniform layout(r32f,    binding = 2) readonly imageBuffer mass_in;
uniform layout(rgba32f, binding = 3) writeonly imageBuffer position_out;
uniform layout(rgba32f, binding = 4) writeonly imageBuffer velocity_out;

layout(location = 0) uniform int elements_count;
layout(location = 1) uniform float G;

vec3 calc_force(vec3 pos_a, vec3 vel_a, float mass_a,
                vec3 pos_b, vec3 vec_b, float mass_b);

vec3 calc_force(vec3 pos_a, vec3 vel_a, float mass_a,
                vec3 pos_b, vec3 vec_b, float mass_b) {
        vec3 pos_diff = pos_b - pos_a;
        float dist2 = pos_diff.x * pos_diff.x
                    + pos_diff.y * pos_diff.y
                    + pos_diff.z * pos_diff.z;

        if(dist2 == 0.0f) {
            return vec3(0.0);
        }
        float force = G * mass_a * mass_b / dist2;
        return normalize(pos_diff) * force;
}

void main() {
    int id = int(gl_GlobalInvocationID.x);

    vec3 force = vec3(0.0);

    vec3 pos = imageLoad(position_in, id).xyz;
    vec3 vel = imageLoad(velocity_in, id).xyz;
    float mass = imageLoad(mass_in, id).x;

    int i;
    for(i = 0; i < id; ++i) {
        vec3 pos_ = imageLoad(position_in, i).xyz;
        vec3 vel_ = imageLoad(velocity_in, i).xyz;
        float mass_ = imageLoad(mass_in, i).x;
        force += calc_force(pos, vel, mass, pos_, vel_, mass_);
    }
    for(i = id + 1; i < elements_count; ++i) {
        vec3 pos_ = imageLoad(position_in, i).xyz;
        vec3 vel_ = imageLoad(velocity_in, i).xyz;
        float mass_ = imageLoad(mass_in, i).x;
        force += calc_force(pos, vel, mass, pos_, vel_, mass_);
    }

    vec3 vel_new = vel + force / mass;
    vec3 pos_new = pos + vel_new;

    imageStore(position_out, id, vec4(pos_new, 0.0));
    imageStore(velocity_out, id, vec4(vel_new, 0.0));
}
)";

GravityComputeShader::GravityComputeShader() : base_t(code) {}

void GravityComputeShader::set_position_in(ArrayBufferObject &vbo) {
    set_buffer<glm::vec4>(vbo, 0, GL_READ_ONLY);
}

void GravityComputeShader::set_velocity_in(ArrayBufferObject &vbo) {
    set_buffer<glm::vec4>(vbo, 1, GL_READ_ONLY);
}

void GravityComputeShader::set_mass_in(ArrayBufferObject &vbo) {
    set_buffer<GLfloat>(vbo, 2, GL_READ_ONLY);
}

void GravityComputeShader::set_position_out(ArrayBufferObject &vbo) {
    set_buffer<glm::vec4>(vbo, 3, GL_WRITE_ONLY);
}

void GravityComputeShader::set_velocity_out(ArrayBufferObject &vbo) {
    set_buffer<glm::vec4>(vbo, 4, GL_WRITE_ONLY);
}

void GravityComputeProgramConfig::set_elements_count(GLint val) { program.set_uniform(0, val); }

void GravityComputeProgramConfig::set_G(GLfloat val) { program.set_uniform(1, val); }
