#pragma once

#include <ShaderProgram.hpp>
#include <VertexArrayObject.hpp>
#include <Texture.hpp>
#include <glm/glm.hpp>
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

template<size_t VBO_COUNT>
struct ComputeShaderBase {
    struct BufferImage {
        GLuint id;
        Texture<GL_TEXTURE_BUFFER> tex;
        GLuint access;
        GLuint format;
        void init(VertexBufferObject& vbo, GLuint id, GLuint access, GLuint format) {
            this->id = id;
            this->access = access;
            this->format = format;
            tex.bind();
            tex.tex_buffer(vbo, format);
        }
        void bind_image() {
            tex.bind_image(id, access, format);
        }
    };

    std::array<BufferImage, VBO_COUNT> tex;
    ShaderProgram program;
    template<typename T>
    void set_buffer(VertexBufferObject& vbo, GLuint id, GLuint access) {
        tex.at(id).init(vbo, id, access, type_to_format_v<T>);
    }
    void bind_buffer_images() {
        for(auto& t : tex) {
            t.bind_image();
        }
    }
    ComputeShaderBase(const std::string& code)
        : program(Shader(code, GL_COMPUTE_SHADER)) {}

    void use_program() {
        program.use();
    }
    void dispatch(GLuint x, GLuint y, GLuint z) {
        bind_buffer_images();
        glDispatchCompute(x, y, z);
    }
    void barrier() {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
};
