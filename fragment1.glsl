#version 330 core

out vec4 FragColor;

uniform sampler2D ourTexture;

in vec3 color;
in vec2 texture_coord;

void main() {
    FragColor = texture(ourTexture, texture_coord) * vec4(color, 1.0);
}
