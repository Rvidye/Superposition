#version 460 core

layout(location = 2) uniform vec4 color;

layout(location = 0) out vec4 fragColor;

void main(void) {
    fragColor = color;
}
