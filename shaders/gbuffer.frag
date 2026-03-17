#version 460 core

// G-Buffer geometry pass fragment shader.
// Outputs PBR material properties for deferred lighting.

in vec3  vWorldPos;
in vec2  vUV;
in vec3  vNormal;
in float vAO;
in float vTexIndex;

uniform sampler2D uTexAtlas;
uniform float uAtlasSize;

// MRT outputs matching GBuffer layout:
//   0: Albedo.rgb + Roughness.a        (RGBA8)
//   1: Normal.rgb + Metallic.a          (RGBA16F)
//   2: WorldPosition.rgb + AO.a         (RGBA16F)
layout(location = 0) out vec4 gAlbedoRoughness;
layout(location = 1) out vec4 gNormalMetallic;
layout(location = 2) out vec4 gPositionAO;

void main() {
    // Atlas UV computation (same as original chunk.frag)
    float tileSize = 1.0 / uAtlasSize;
    float idx      = floor(vTexIndex + 0.5);
    float col      = mod(idx, uAtlasSize);
    float row      = floor(idx / uAtlasSize);
    vec2 tileUV    = fract(vUV);
    vec2 atlasUV   = (vec2(col, row) + tileUV) * tileSize;

    vec4 texColor = texture(uTexAtlas, atlasUV);

    // Discard fully transparent fragments (leaves, etc.)
    if (texColor.a < 0.1) discard;

    // PBR material estimation from texture:
    // Voxel blocks are dielectric (non-metal) with varying roughness.
    // Roughness: stone/gravel → 0.95, dirt → 0.9, grass → 0.85, sand → 0.8
    // We encode a reasonable default; the lighting shader handles the rest.
    float roughness = 0.85;

    // Metallic: 0 for all natural voxel blocks
    float metallic = 0.0;

    gAlbedoRoughness = vec4(texColor.rgb, roughness);
    gNormalMetallic  = vec4(normalize(vNormal) * 0.5 + 0.5, metallic);
    gPositionAO      = vec4(vWorldPos, vAO);
}
