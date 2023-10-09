#include "Renderer.hpp"

#include <glm/gtx/transform.hpp>

std::string vertex_code = R"(
#version 430 core

layout (location = 0) in vec4 position;
layout (location = 1) in float size;
layout (location = 2) in vec3 vert;

layout (location = 0) uniform mat4 mvp;
layout (location = 1) uniform vec3 cam_pos;

mat4 rotation3d(vec3 a, float c, float s) {
  a = normalize(a);
  float oc = 1.0 - c;

  return mat4(
    oc*a.x*a.x + c,     oc*a.x*a.y - a.z*s, oc*a.z*a.x + a.y*s, 0.0,
    oc*a.x*a.y + a.z*s, oc*a.y*a.y + c,     oc*a.y*a.z - a.x*s, 0.0,
    oc*a.z*a.x - a.y*s, oc*a.y*a.z + a.x*s, oc*a.z*a.z + c,     0.0,
    0.0,                0.0,                0.0,                1.0
  );
}

float cross_sin(vec3 cross_vec, vec3 norm_cross_vec) {
    if(norm_cross_vec.x != 0)
        return cross_vec.x / norm_cross_vec.x;
    if(norm_cross_vec.y != 0)
        return cross_vec.y / norm_cross_vec.y;
    if(norm_cross_vec.z != 0)
        return cross_vec.z / norm_cross_vec.z;
    return 0.0f;
}

vec3 up = vec3(0.0, 0.0, 1.0);

void main() {
    vec3 cam_vec = normalize(cam_pos - position.xyz);
    float cos = dot(cam_vec, up);
    vec3 cross_vec = cross(cam_vec, up);
    vec3 norm_cross_vec = normalize(cross_vec);
    float sin = cross_sin(cross_vec, norm_cross_vec);

    vec3 vert_cam = (rotation3d(norm_cross_vec, cos, sin) * vec4(vert, 1.0)).xyz;

    gl_Position = mvp * vec4(position.xyz + vert_cam*size, 1.0);
}
)";

std::string vertex_point_code = R"(
#version 430 core

layout (location = 0) in vec4 position;

layout (location = 0) uniform mat4 mvp;
layout (location = 1) uniform vec3 cam_pos;

void main() {
    gl_Position = mvp * vec4(position.xyz, 1.0);
}
)";


std::string fragment_code = R"(
#version 430 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(vec3(0.0f), 1.0f);
}
)";

void Renderer::prepare_vertices() {
    glm::vec4 left{1.0, 0.0, 0.0, 1.0f};
    std::vector<glm::vec3> vertices;
    for(auto i : std::views::iota(0, 12)) {
        auto mat = glm::rotate(M_PIf*i/6,glm::vec3{ 0.0f, 0.0f, 1.0f});
        auto vec = mat*left;
        vertices.push_back(glm::vec3(vec));
    }

    vbo_vertices.bind().upload(vertices);
}

Renderer::Renderer(ArrayBufferObject &vbo_positions, ArrayBufferObject &vbo_radii)
    : program_circles(Shader::make_vertex(vertex_code),
                      Shader::make_fragment(fragment_code))
    , program_points(Shader::make_vertex(vertex_point_code),
                      Shader::make_fragment(fragment_code)){
    prepare_vertices();
    vao_circles.bind().add_array_buffer(vbo_positions, 0, 4, 0, 0, 1);
    vao_circles.bind().add_array_buffer(vbo_radii, 1, 1, 0, 0, 1);
    vao_circles.bind().add_array_buffer(vbo_vertices, 2, 3);

    vao_points.bind().add_array_buffer(vbo_positions, 0, 4);
}

void Renderer::render(size_t count, const glm::mat4& VP, const glm::vec3& cam_pos) {
    if(auto program = program_circles.use(); true) {
        program.set_uniformv(0, &VP).set_uniform(1, cam_pos);

        if(auto b = vao_circles.bind(); true)
            glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 12, count);
        }

    if(auto program = program_points.use(); true) {
        program.set_uniformv(0, &VP);

        if(auto b = vao_points.bind(); true)
            glDrawArrays(GL_POINTS, 0, count);
    }
}
