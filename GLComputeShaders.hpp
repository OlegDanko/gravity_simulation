#pragma once

#include <string>
#include <GLContext.hpp>

#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>

std::string force_compute_shader = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer position_in;
uniform layout(r32f, binding = 1) readonly imageBuffer mass_in;
uniform layout(rgba32f, binding = 2) writeonly imageBuffer force_out;

layout(location = 0) uniform float G;
layout(location = 1) uniform int a_offset;
layout(location = 2) uniform int b_offset;

void main() {
    int id = int(gl_GlobalInvocationID.x);
    highp int row = int((sqrt(8*id + 1) - 1) / 2);
    int col = id - row * (row + 1) / 2;

    int a_id = a_offset + col;
    int b_id = b_offset + row;

    vec3 a_pos = imageLoad(position_in, a_id).xyz;
    vec3 b_pos = imageLoad(position_in, b_id).xyz;

    vec3 pos_diff = a_pos - b_pos;
    float dist2 = pos_diff.x * pos_diff.x
                + pos_diff.y * pos_diff.y
                + pos_diff.z * pos_diff.z;

    if(dist2 == 0.0f) {
        imageStore(force_out, id, vec4(0.0, 0.0, 0.0, 0.0));
        return;
    }

    float a_mass = imageLoad(mass_in, a_id).x;
    float b_mass = imageLoad(mass_in, b_id).x;

    float force = G * a_mass*b_mass / dist2;

    vec3 force_vec = normalize(pos_diff) * force;

    imageStore(force_out, id, vec4(force_vec, 0.0));
}
)";

int test_force_compute_shader() {
    VertexBufferObject vbo_pos, vbo_mass, vbo_force;
    std::vector<glm::vec4> elements_pos {
        {0.0f, 0.0f, 0.0f, 0.0f },
        {1.0f, 0.0f, 0.0f, 0.0f },
        {2.0f, 0.0f, 0.0f, 0.0f },
        {5.0f, 0.0f, 0.0f, 0.0f }
    };
    vbo_pos.upload(elements_pos);

    std::vector<float> elements_mass{ 1.0, 2.0, 3.0, 4.0 };
    vbo_mass.upload(elements_mass);

    std::vector<glm::vec4> elements_force(10);
    std::fill_n(std::back_inserter(elements_force), 10, glm::vec4(0.0f));
    vbo_force.upload(elements_force);

    GLuint tex[3];
    glGenTextures(3, tex);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_pos.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[1]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, vbo_mass.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[2]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_force.id);

    Shader v(force_compute_shader, GL_COMPUTE_SHADER);
    ShaderProgram program(v);
    program.use();
    program.set_uniform(0, 1.0f);
    program.set_uniform(1, 0);
    program.set_uniform(2, 0);

    glBindImageTexture(0, tex[0], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, tex[1], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_R32F);
    glBindImageTexture(2, tex[2], 0, GL_FALSE, 0,  GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(10, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<glm::vec4> elements_out(10);
    vbo_force.download(elements_out);
    for(auto e : elements_out) {
        std::cout << e.x << " " << e.y << " " << e.z << " " << e.a << "\n";
    }
    std::cout << std::endl;

    return 0;
}

std::string total_force_compute_shader = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer force_in;
uniform layout(rgba32f, binding = 1) writeonly imageBuffer force_a_out;
uniform layout(rgba32f, binding = 2) writeonly imageBuffer force_b_out;

layout(location = 0) uniform int a_offset;
layout(location = 1) uniform int b_offset;
layout(location = 2) uniform int out_offset;
layout(location = 3) uniform int chunk_size;

int row_col_to_id(int r, int c);

int row_col_to_id(int r, int c) {
    return (r+1)*r/2 + c;
}

void main() {
    int id = int(gl_GlobalInvocationID.x);

    int a_id = a_offset + id;
    int b_id = b_offset + id;

    vec3 force_a = vec3(0.0);
    for(int col = id, row = id; row < chunk_size; row++) {
        force_a += imageLoad(force_in, row_col_to_id(row, col)).xyz;
    }

    vec3 force_b = vec3(0.0);
    for(int row = id, col = 0; col <= id; col++) {
        force_b += imageLoad(force_in, row_col_to_id(row, col)).xyz;
    }
    imageStore(force_a_out, out_offset + id, vec4(force_a, 0.0));
    imageStore(force_b_out, out_offset + id, vec4(force_b, 0.0));
}
)";

int test_total_force_compute_shader() {
    VertexBufferObject vbo_forces, vbo_fa_out, vbo_fb_out;
    std::vector<glm::vec4> elements_force {
        glm::vec4{1.0f},
        glm::vec4{2.0f}, glm::vec4{3.0f},
        glm::vec4{4.0f}, glm::vec4{5.0f}, glm::vec4{6.0f},
        glm::vec4{7.0f}, glm::vec4{8.0f}, glm::vec4{9.0f}, glm::vec4{10.0f},
    };
    vbo_forces.upload(elements_force);

    std::vector<glm::vec4> elements_f_out;
    std::fill_n(std::back_inserter(elements_f_out), 4, glm::vec4(0.0f));
    vbo_fa_out.upload(elements_f_out);
    vbo_fb_out.upload(elements_f_out);

    GLuint tex[3];
    glGenTextures(3, tex);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_forces.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[1]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_fa_out.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[2]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_fb_out.id);

    Shader v(total_force_compute_shader, GL_COMPUTE_SHADER);
    ShaderProgram program(v);
    program.use();
    program.set_uniform(0, 0);
    program.set_uniform(1, 0);
    program.set_uniform(2, 0);
    program.set_uniform(3, 4);

    glBindImageTexture(0, tex[0], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, tex[1], 0, GL_FALSE, 0,  GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(2, tex[2], 0, GL_FALSE, 0,  GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(4, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<glm::vec4> elements_out(4);
    vbo_fa_out.download(elements_out);
    std::cout << "A:" << std::endl;
    for(auto e : elements_out) {
        std::cout << e.x << " " << e.y << " " << e.z << " " << e.a << "\n";
    }
    std::cout << "B:" << std::endl;
    vbo_fb_out.download(elements_out);
    for(auto e : elements_out) {
        std::cout << e.x << " " << e.y << " " << e.z << " " << e.a << "\n";
    }
    std::cout << std::endl;

    return 0;
}

std::string update_position_velocity_compute_shader = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer position_in;
uniform layout(rgba32f, binding = 1) readonly imageBuffer velocity_in;
uniform layout(r32f,    binding = 2) readonly imageBuffer mass_in;
uniform layout(rgba32f, binding = 3) readonly imageBuffer force_a_in;
uniform layout(rgba32f, binding = 4) readonly imageBuffer force_b_in;
uniform layout(rgba32f, binding = 5) writeonly imageBuffer position_out;
uniform layout(rgba32f, binding = 6) writeonly imageBuffer velocity_out;

layout(location = 0) uniform int chunk_size;
layout(location = 1) uniform int chunks_count;

void main() {
    int id = int(gl_GlobalInvocationID.x);

    vec3 force_a = vec3(0.0);
    vec3 force_b = vec3(0.0);
    for(int chunk = 0; chunk < chunks_count; chunk++) {
        int chunk_offset = chunk * chunk_size;
        force_a += imageLoad(force_a_in, chunk_offset + id).xyz;
        force_b += imageLoad(force_b_in, chunk_offset + id).xyz;
    }
    vec3 force_total = force_a - force_b;
    vec3 delta_v = force_total / imageLoad(mass_in, id).x;
    vec3 v_new = imageLoad(velocity_in, id).xyz + delta_v;
    vec3 p_new = imageLoad(position_in, id).xyz + v_new;
    imageStore(position_out, id, vec4(p_new, 1.0));
    imageStore(velocity_out, id, vec4(v_new, 1.0));
}
)";

int test_update_position_velocity_shader() {
    VertexBufferObject vbo_position_in,
                       vbo_velocity_in,
                       vbo_mass_in,
                       vbo_force_a_in,
                       vbo_force_b_in,
                       vbo_position_out,
                       vbo_velocity_out;

    std::vector<glm::vec4> elements_position {
        glm::vec4(1.0),
        glm::vec4(2.0),
        glm::vec4(3.0),
        glm::vec4(4.0),
    };
    vbo_position_in.upload(elements_position);
    std::vector<glm::vec4> elements_velocity {
        glm::vec4(5.0),
        glm::vec4(6.0),
        glm::vec4(7.0),
        glm::vec4(8.0),
    };
    vbo_velocity_in.upload(elements_velocity);
    std::vector<float> elements_mass {
        1.0f,
        2.0f,
        3.0f,
        4.0f,
    };
    vbo_mass_in.upload(elements_mass);
    std::vector<glm::vec4> elements_force_a {
        glm::vec4(1.0),
        glm::vec4(2.0),
        glm::vec4(3.0),
        glm::vec4(4.0),
    };
    vbo_force_a_in.upload(elements_force_a);
    std::vector<glm::vec4> elements_force_b {
        glm::vec4(4.0),
        glm::vec4(3.0),
        glm::vec4(2.0),
        glm::vec4(1.0),
    };
    vbo_force_b_in.upload(elements_force_b);

    std::vector<glm::vec4> elements_out;
    std::fill_n(std::back_inserter(elements_out), 4, glm::vec4(0.0f));
    vbo_position_out.upload(elements_out);
    vbo_velocity_out.upload(elements_out);

    GLuint tex[7];
    glGenTextures(7, tex);
    glBindTexture(GL_TEXTURE_BUFFER, tex[0]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_position_in.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[1]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_velocity_in.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[2]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, vbo_mass_in.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[3]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_force_a_in.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[4]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_force_b_in.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[5]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_position_out.id);

    glBindTexture(GL_TEXTURE_BUFFER, tex[6]);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vbo_velocity_out.id);

    Shader v(update_position_velocity_compute_shader, GL_COMPUTE_SHADER);
    ShaderProgram program(v);
    program.use();
    program.set_uniform(0, 4);
    program.set_uniform(1, 1);

    glBindImageTexture(0, tex[0], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, tex[1], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(2, tex[2], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_R32F);
    glBindImageTexture(3, tex[3], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(4, tex[4], 0, GL_FALSE, 0,  GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(5, tex[5], 0, GL_FALSE, 0,  GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(6, tex[6], 0, GL_FALSE, 0,  GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(4, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::cout << "Positions:" << std::endl;
    vbo_position_out.download(elements_out);
    for(auto e : elements_out) {
        std::cout << e.x << " " << e.y << " " << e.z << " " << e.a << "\n";
    }
    std::cout << "Velocities:" << std::endl;
    vbo_velocity_out.download(elements_out);
    for(auto e : elements_out) {
        std::cout << e.x << " " << e.y << " " << e.z << " " << e.a << "\n";
    }

    return 0;
}
