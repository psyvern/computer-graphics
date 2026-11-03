#version 330 core

out vec4 FragColor;

uniform sampler2D ourTexture;

in vec3 color;
in vec2 texture_coord;
in vec3 normal;

void main() {
    FragColor = texture(ourTexture, texture_coord) * vec4(color, 1.0);
    FragColor.rgb = FragColor.rgb * clamp(dot(normal, vec3(0, 1, 0)) + 0.3, 0, 1);
}

