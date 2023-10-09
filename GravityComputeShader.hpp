#pragma once

#include "ComputeShaderBase.hpp"

//

struct GravityComputeProgramConfig {
    ShaderProgram::InUse program;

    void set_elements_count(GLint val);
    void set_G(GLfloat val);
};

struct GravityComputeShader : ComputeShaderBase<5, GravityComputeProgramConfig>{
    using base_t = ComputeShaderBase<5, GravityComputeProgramConfig>;
    static const std::string code;


    GravityComputeShader();

    void set_position_in(ArrayBufferObject& vbo);
    void set_velocity_in(ArrayBufferObject& vbo);
    void set_mass_in(ArrayBufferObject& vbo);
    void set_position_out(ArrayBufferObject& vbo);
    void set_velocity_out(ArrayBufferObject& vbo);

};
