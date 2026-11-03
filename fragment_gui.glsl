#version 330 core

out vec4 FragColor;

uniform sampler2D font_texture;
uniform vec4 color;

in vec2 texture_coord;

void main() {
    float alpha = texture(font_texture, texture_coord).x;
    FragColor = color * alpha;
}
