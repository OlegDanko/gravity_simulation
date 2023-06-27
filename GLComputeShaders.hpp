#pragma once

#include <string>
#include <ranges>
#include <GLContext.hpp>

#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>
#include <array>

template<typename T>
struct type_to_format;

template<>
struct type_to_format<GLfloat> {
    static constexpr GLuint val = GL_R32F;
};

template<>
struct type_to_format<glm::vec4> {
    static constexpr GLuint val = GL_RGBA32F;
};

template <typename T>
constexpr GLuint type_to_format_v = type_to_format<T>::val;

template<GLuint type>
struct Texture {
    GLuint id;
    Texture() {
        glGenTextures(1, &id);
    }
    ~Texture() {
        glDeleteTextures(1, &id);
    }
    void bind() {
        glBindTexture(type, id);
    }
    std::enable_if_t<type == GL_TEXTURE_BUFFER, void>
    tex_buffer(VertexBufferObject& vbo, GLuint format) {
        glTexBuffer(GL_TEXTURE_BUFFER, format, vbo.id);
    }
    void bind_image(GLuint bind_id,
                    GLuint access,
                    GLuint format,
                    GLuint level = 0,
                    GLuint layered = GL_FALSE,
                    GLuint layer = 0) {
        glBindImageTexture(bind_id, id, level, layered, layer, access, format);
    }
};

template<size_t VBO_COUNT>
struct ComputeShaderBase {
    std::array<Texture<GL_TEXTURE_BUFFER>, VBO_COUNT> tex;
    ShaderProgram program;
    template<typename T>
    void set_buffer(VertexBufferObject& vbo, size_t id, GLuint access) {
        auto format = type_to_format_v<T>;
        tex.at(id).bind();
        tex.at(id).tex_buffer(vbo, format);
        tex.at(id).bind_image(id, access, format);
    }
    ComputeShaderBase(const std::string& code)
        : program(Shader(code, GL_COMPUTE_SHADER)) {}

    void use_program() {
        program.use();
    }
    void dispatch(GLuint x, GLuint y, GLuint z) {
        glDispatchCompute(x, y, z);
    }
    void barrier() {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
};

struct ForceComputeShader : ComputeShaderBase<3> {
    static const std::string code;

    ForceComputeShader() : ComputeShaderBase<3>(code) {}

    void set_position_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 0, GL_READ_ONLY); }
    void set_mass_in(VertexBufferObject& vbo) { set_buffer<GLfloat>(vbo, 1, GL_READ_ONLY); }
    void set_force_out(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 2, GL_WRITE_ONLY); }

    void set_G(GLfloat val) { program.set_uniform(0, val); }
    void set_a_offset(GLint val) { program.set_uniform(1, val); }
    void set_b_offset(GLint val) { program.set_uniform(2, val); }
    void set_chunk_size(GLint val) { program.set_uniform(3, val); }
};

const std::string ForceComputeShader::code = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer position_in;
uniform layout(r32f, binding = 1) readonly imageBuffer mass_in;
uniform layout(rgba32f, binding = 2) writeonly imageBuffer force_out;

layout(location = 0) uniform float G;
layout(location = 1) uniform int a_offset;
layout(location = 2) uniform int b_offset;
layout(location = 3) uniform int chunk_size;

void main() {
    int id = int(gl_GlobalInvocationID.x);
    int row = id / chunk_size;
    int col = id % chunk_size;

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

    std::vector<glm::vec4> elements_force;
    std::fill_n(std::back_inserter(elements_force), 16, glm::vec4(0.0f));
    vbo_force.upload(elements_force);

    ForceComputeShader shader_program;
    shader_program.use_program();
    shader_program.set_position_in(vbo_pos);
    shader_program.set_mass_in(vbo_mass);
    shader_program.set_force_out(vbo_force);

    shader_program.set_G(1.0f);
    shader_program.set_a_offset(0);
    shader_program.set_b_offset(0);
    shader_program.set_chunk_size(4);

    shader_program.dispatch(16, 1, 1);
    shader_program.barrier();

    std::vector<glm::vec4> elements_out(16);
    vbo_force.download(elements_out);
    for(auto [id, e] : elements_out | std::views::enumerate) {
        std::cout << e.x << " ";// << e.y << " " << e.z << " " << e.a << "\n";
        if(id % 4 == 3)
            std::cout << std::endl;
    }
    std::cout << std::endl;

    return 0;
}

struct TotalForceComputeShader : ComputeShaderBase<3> {
    static const std::string code;

    TotalForceComputeShader() : ComputeShaderBase<3>(code) {}

    void set_force_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 0, GL_READ_ONLY); }
    void set_force_out(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 1, GL_READ_WRITE); }

    void set_a_offset(GLuint val) { program.set_uniform(0, val); }
    void set_b_offset(GLuint val) { program.set_uniform(1, val); }
    void set_chunk_size(GLuint val) { program.set_uniform(2, val); }
};

const std::string TotalForceComputeShader::code = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer force_in;
uniform layout(rgba32f, binding = 1) imageBuffer force_out;

layout(location = 0) uniform int a_offset;
layout(location = 1) uniform int b_offset;
layout(location = 2) uniform int chunk_size;

void main() {
    int id = int(gl_GlobalInvocationID.x);

    int a_id = a_offset + id;
    int b_id = b_offset + id;

    vec3 force_a = vec3(0.0);
    vec3 force_b = vec3(0.0);

    for(int i = 0; i < chunk_size; i++) {
        force_a += imageLoad(force_in, i * chunk_size + id).xyz;
        force_b += imageLoad(force_in, id * chunk_size + i).xyz;
    }

    vec3 force_a_prev = imageLoad(force_out, a_id).xyz;
    imageStore(force_out, a_id, vec4(force_a_prev - force_a, 0.0));

    if(a_id == b_id) return;

    vec3 force_b_prev = imageLoad(force_out, b_id).xyz;
    imageStore(force_out, b_id, vec4(force_b_prev + force_b, 0.0));
}
)";

int test_total_force_compute_shader() {
    VertexBufferObject vbo_forces_in, vbo_forces_out;
    std::vector<glm::vec4> elements_force {
        glm::vec4{1.0f}, glm::vec4{2.0f}, glm::vec4{3.0f}, glm::vec4{4.0f},
        glm::vec4{5.0f}, glm::vec4{6.0f}, glm::vec4{7.0f}, glm::vec4{8.0f},
        glm::vec4{9.0f}, glm::vec4{1.0f}, glm::vec4{2.0f}, glm::vec4{3.0f},
        glm::vec4{4.0f}, glm::vec4{5.0f}, glm::vec4{6.0f}, glm::vec4{7.0f},
    };
    vbo_forces_in.upload(elements_force);
    std::vector<glm::vec4> elements_force_current {
        glm::vec4{100.0f},
        glm::vec4{200.0f},
        glm::vec4{300.0f},
        glm::vec4{400.0f},
        glm::vec4{-500.0f},
        glm::vec4{-600.0f},
        glm::vec4{-700.0f},
        glm::vec4{-800.0f}
    };
    vbo_forces_out.upload(elements_force_current);

    TotalForceComputeShader shader_program;
    shader_program.use_program();
    shader_program.set_force_in(vbo_forces_in);
    shader_program.set_force_out(vbo_forces_out);

    shader_program.set_a_offset(0);
    shader_program.set_b_offset(4);
    shader_program.set_chunk_size(4);

    shader_program.dispatch(4, 1, 1);
    shader_program.barrier();

    std::vector<glm::vec4> elements_out(8);
    vbo_forces_out.download(elements_out);
    for(auto e : elements_out) {
        std::cout << e.x /*<< " " << e.y << " " << e.z << " " << e.a*/ << "\n";
    }
    std::cout << std::endl;

    return 0;
}

struct UpdatePositionVelocityComputeShader : ComputeShaderBase<7> {
    static const std::string code;

    UpdatePositionVelocityComputeShader() : ComputeShaderBase<7>(code) {}

    void set_position_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 0, GL_READ_ONLY); }
    void set_velocity_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 1, GL_READ_ONLY); }
    void set_mass_in(VertexBufferObject& vbo) { set_buffer<GLfloat>(vbo, 2, GL_READ_ONLY); }
    void set_force_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 3, GL_READ_ONLY); }
    void set_position_out(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 4, GL_WRITE_ONLY); }
    void set_velocity_out(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 5, GL_WRITE_ONLY); }
};

const std::string UpdatePositionVelocityComputeShader::code  = R"(
#version 430 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform layout(rgba32f, binding = 0) readonly imageBuffer position_in;
uniform layout(rgba32f, binding = 1) readonly imageBuffer velocity_in;
uniform layout(r32f,    binding = 2) readonly imageBuffer mass_in;
uniform layout(rgba32f, binding = 3) readonly imageBuffer force_in;
uniform layout(rgba32f, binding = 4) writeonly imageBuffer position_out;
uniform layout(rgba32f, binding = 5) writeonly imageBuffer velocity_out;

void main() {
    int id = int(gl_GlobalInvocationID.x);

    vec3 delta_v = imageLoad(force_in, id).xyz / imageLoad(mass_in, id).x;
    vec3 v_new = imageLoad(velocity_in, id).xyz + delta_v;
    vec3 p_new = imageLoad(position_in, id).xyz + v_new;
    imageStore(position_out, id, vec4(p_new, 0.0));
    imageStore(velocity_out, id, vec4(v_new, 0.0));
}
)";

int test_update_position_velocity_shader() {
    VertexBufferObject vbo_position_in,
                       vbo_velocity_in,
                       vbo_mass_in,
                       vbo_force_in,
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
    std::vector<glm::vec4> elements_force {
        glm::vec4(1.0),
        glm::vec4(2.0),
        glm::vec4(3.0),
        glm::vec4(4.0),
    };
    vbo_force_in.upload(elements_force);

    std::vector<glm::vec4> elements_out;
    std::fill_n(std::back_inserter(elements_out), 4, glm::vec4(0.0f));
    vbo_position_out.upload(elements_out);
    vbo_velocity_out.upload(elements_out);

    UpdatePositionVelocityComputeShader shader_program;
    shader_program.use_program();
    shader_program.set_position_in(vbo_position_in);
    shader_program.set_position_in(vbo_velocity_in);
    shader_program.set_position_in(vbo_mass_in);
    shader_program.set_position_in(vbo_force_in);
    shader_program.set_position_in(vbo_position_out);
    shader_program.set_position_in(vbo_velocity_out);

    shader_program.dispatch(4, 1, 1);
    shader_program.barrier();

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

struct GravityComputeShader : ComputeShaderBase<5> {
    static const std::string code;

    GravityComputeShader() : ComputeShaderBase<5>(code) {}

    void set_position_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 0, GL_READ_ONLY); }
    void set_velocity_in(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 1, GL_READ_ONLY); }
    void set_mass_in(VertexBufferObject& vbo) { set_buffer<GLfloat>(vbo, 2, GL_READ_ONLY); }
    void set_position_out(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 3, GL_WRITE_ONLY); }
    void set_velocity_out(VertexBufferObject& vbo) { set_buffer<glm::vec4>(vbo, 4, GL_WRITE_ONLY); }

    void set_elements_count(GLint val) { program.set_uniform(0, val); }
    void set_G(GLfloat val) { program.set_uniform(1, val); }

};

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
//        return normalize(pos_diff) * G * mass_a * mass_b / dist2;
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
//        force += calc_force(pos, vel, mass,
//                            imageLoad(position_in, i).xyz,
//                            imageLoad(velocity_in, i).xyz,
//                            imageLoad(mass_in, i).x);
    }
    for(i = id + 1; i < elements_count; ++i) {
        vec3 pos_ = imageLoad(position_in, i).xyz;
        vec3 vel_ = imageLoad(velocity_in, i).xyz;
        float mass_ = imageLoad(mass_in, i).x;
        force += calc_force(pos, vel, mass, pos_, vel_, mass_);
//        force += calc_force(pos, vel, mass,
//                            imageLoad(position_in, i).xyz,
//                            imageLoad(velocity_in, i).xyz,
//                            imageLoad(mass_in, i).x);
    }

    vec3 vel_new = vel + force / mass;
    vec3 pos_new = pos + vel_new;

    imageStore(position_out, id, vec4(pos_new, 0.0));
//    imageStore(position_out, id, vec4(pos + vel_new, 0.0));
    imageStore(velocity_out, id, vec4(vel_new, 0.0));
}
)";

