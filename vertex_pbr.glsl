#version 330 core

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 2) in vec2 vertex_texture;
layout(location = 3) in vec3 vertex_normal;

uniform mat4 MVP;

out vec3 color;
out vec2 texture_coord;
out vec3 normal;

void main() {
    color = vertex_color;
    texture_coord = vertex_texture;
    normal = vertex_normal;

    gl_Position = MVP * vec4(vertex_position, 1.0);
}

