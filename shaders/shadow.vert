#version 460 core

// Shadow depth-only vertex shader.
// Transforms geometry into light-clip space for CSM rendering.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;       // unused but must be declared for stride
layout(location = 2) in vec3 aNormal;   // unused
layout(location = 3) in float aAO;      // unused
layout(location = 4) in float aTexIndex;// unused

uniform mat4 uLightSpace;
uniform mat4 uModel;

void main() {
    gl_Position = uLightSpace * uModel * vec4(aPos, 1.0);
}
