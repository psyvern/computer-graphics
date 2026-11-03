#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 MVP;

out vec3 color;
out vec2 texture_coord;

void main() {
    color = aColor;
    texture_coord = aTexCoord;

    gl_Position = MVP * vec4(aPos, 1.0);
}
