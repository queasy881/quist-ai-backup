#version 460 core

in vec2  vUV;
in vec3  vNormal;
in float vAO;
in float vTexIndex;
in float vFogDist;
in vec3  vWorldPos;

uniform sampler2D uTexAtlas;
uniform float uAtlasSize;   // tiles per row
uniform float uDaylight;
uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform vec3  uCamPos;
uniform float uIsWater;

out vec4 FragColor;

void main() {
    // Compute atlas UV from tile index + face-local UV
    float tileSize = 1.0 / uAtlasSize;
    float idx      = floor(vTexIndex + 0.5);
    float col      = mod(idx, uAtlasSize);
    float row      = floor(idx / uAtlasSize);

    vec2 tileUV = fract(vUV);                          // wraps for greedy-merged quads
    vec2 atlasUV = (vec2(col, row) + tileUV) * tileSize;

    vec4 texColor = texture(uTexAtlas, atlasUV);

    // Simple directional lighting from top
    float sunDot = max(dot(vNormal, vec3(0.0, 1.0, 0.0)), 0.0);
    float light  = 0.55 + 0.45 * sunDot;
    light *= uDaylight;

    // Underground darkness
    float depthFade = clamp(vWorldPos.y / 64.0, 0.2, 1.0);
    light *= depthFade;

    // Ambient occlusion
    light *= mix(0.45, 1.0, vAO);

    vec3 litColor = texColor.rgb * light;

    // Fog
    float fogFactor = clamp((vFogDist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    litColor = mix(litColor, uFogColor, fogFactor);

    float alpha = texColor.a;

    // Water tint + transparency
    if (uIsWater > 0.5) {
        litColor *= vec3(0.3, 0.5, 0.95);
        alpha = 0.6;
    }

    FragColor = vec4(litColor, alpha);
}
