#version 460 core

// G-Buffer geometry pass vertex shader.
// Same vertex layout as the original chunk.vert.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in float aAO;
layout(location = 4) in float aTexIndex;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3  vWorldPos;
out vec2  vUV;
out vec3  vNormal;
out float vAO;
out float vTexIndex;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos     = worldPos.xyz;
    gl_Position   = uProj * uView * worldPos;

    vUV       = aUV;
    vNormal   = aNormal;  // world-space (block normals are axis-aligned)
    vAO       = aAO;
    vTexIndex = aTexIndex;
}
