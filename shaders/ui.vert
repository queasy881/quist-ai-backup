#version 460 core

layout(location = 0) in vec2 aPos;

uniform float uAspect;

void main() {
    vec2 p = aPos;
    p.x /= uAspect;   // correct for aspect ratio
    gl_Position = vec4(p, 0.0, 1.0);
}
