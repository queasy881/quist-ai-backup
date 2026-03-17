#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in float aAO;
layout(location = 4) in float aTexIndex;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec2  vUV;
out vec3  vNormal;
out float vAO;
out float vTexIndex;
out float vFogDist;
out vec3  vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos     = worldPos.xyz;
    gl_Position   = uProj * uView * worldPos;

    vUV       = aUV;
    vNormal   = aNormal;
    vAO       = aAO;
    vTexIndex = aTexIndex;
    vFogDist  = length((uView * worldPos).xyz);
}
