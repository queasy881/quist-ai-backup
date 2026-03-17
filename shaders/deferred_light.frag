#version 460 core

// Deferred PBR lighting with Cascaded Shadow Maps, SSAO, and fog.

in vec2 vUV;

// G-Buffer textures
uniform sampler2D uAlbedoRoughness; // 0
uniform sampler2D uNormalMetallic;  // 1
uniform sampler2D uPositionAO;      // 2
uniform sampler2D uDepth;           // 3
uniform sampler2D uSSAO;            // 4
uniform sampler2DArrayShadow uShadowMap; // 5

// Camera
uniform mat4  uView;
uniform mat4  uProj;
uniform mat4  uInvView;
uniform mat4  uInvProj;
uniform vec3  uCamPos;

// Sun
uniform vec3  uSunDir;
uniform float uDaylight;

// Fog
uniform vec3  uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

// CSM
uniform mat4  uLightSpace[4];
uniform float uCascadeSplit[4];

out vec4 FragColor;

// ── Constants ────────────────────────────────────────────────
const float PI = 3.14159265359;

// ── PBR functions ────────────────────────────────────────────

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 0.0001);
}

// Geometry function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel (Schlick approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ── Shadow sampling ──────────────────────────────────────────

float SampleShadow(vec3 worldPos) {
    // Determine which cascade to use based on view-space depth
    vec4 viewPos = uView * vec4(worldPos, 1.0);
    float depth = -viewPos.z; // positive depth

    int cascade = 3;
    for (int i = 0; i < 4; ++i) {
        if (depth < uCascadeSplit[i]) {
            cascade = i;
            break;
        }
    }

    // Transform to light space
    vec4 lightSpacePos = uLightSpace[cascade] * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    // PCF 3x3 sampling
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / 2048.0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            // sampler2DArrayShadow: (u, v, layer, ref)
            shadow += texture(uShadowMap, vec4(projCoords.xy + offset, float(cascade), projCoords.z));
        }
    }
    return shadow / 9.0;
}

// ── Main ─────────────────────────────────────────────────────

void main() {
    // Read G-Buffer
    vec4 albedoRoughness = texture(uAlbedoRoughness, vUV);
    vec4 normalMetallic  = texture(uNormalMetallic,  vUV);
    vec4 positionAO      = texture(uPositionAO,      vUV);

    vec3  albedo    = albedoRoughness.rgb;
    float roughness = albedoRoughness.a;
    vec3  N         = normalize(normalMetallic.rgb * 2.0 - 1.0);
    float metallic  = normalMetallic.a;
    vec3  worldPos  = positionAO.rgb;
    float ao        = positionAO.a;
    float ssao      = texture(uSSAO, vUV).r;

    // Combined AO
    float combinedAO = ao * ssao;

    // If position is zero (sky), output fog color
    float depth = texture(uDepth, vUV).r;
    if (depth >= 0.9999) {
        FragColor = vec4(0.0);
        return;
    }

    // PBR lighting
    vec3 V = normalize(uCamPos - worldPos);
    vec3 L = normalize(uSunDir);
    vec3 H = normalize(V + L);

    // Dielectric F0 = 0.04, lerp to albedo for metals
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Sun color (warm tone) scaled by daylight
    vec3 sunColor = vec3(1.0, 0.95, 0.85) * uDaylight * 3.0;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator  = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular   = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    // Shadow
    float shadow = SampleShadow(worldPos);

    vec3 Lo = (kD * albedo / PI + specular) * sunColor * NdotL * shadow;

    // Ambient (hemisphere: upper = sky, lower = ground)
    vec3 skyAmbient = mix(vec3(0.03, 0.03, 0.06), vec3(0.15, 0.20, 0.35), uDaylight);
    vec3 groundAmbient = mix(vec3(0.02, 0.02, 0.03), vec3(0.08, 0.10, 0.06), uDaylight);
    vec3 ambient = mix(groundAmbient, skyAmbient, dot(N, vec3(0,1,0)) * 0.5 + 0.5);
    ambient *= albedo * combinedAO;

    // Underground darkness (same as original)
    float depthFade = clamp(worldPos.y / 64.0, 0.2, 1.0);
    vec3 color = (ambient + Lo) * depthFade;

    // Fog
    float dist = length(worldPos - uCamPos);
    float fogFactor = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    color = mix(color, uFogColor * uDaylight, fogFactor);

    FragColor = vec4(color, 1.0);
}
