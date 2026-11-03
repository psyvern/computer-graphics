#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 MVP;

out vec2 texture_coord;

void main() {
    texture_coord = aTexCoord;

    gl_Position = MVP * vec4(aPos, 0.0, 1.0);
}

