#version 460 core

// Volumetric light scattering (god rays) via ray marching through shadow maps.

in vec2 vUV;

uniform sampler2D uDepth;
uniform sampler2DArrayShadow uShadowMap;

uniform mat4  uInvVP;
uniform vec3  uCamPos;
uniform vec3  uSunDir;
uniform float uDaylight;

uniform mat4  uLightSpace[4];

out vec4 FragColor;

const int   NUM_STEPS  = 32;
const float SCATTERING = 0.15;
const float MIE_G      = 0.75;
const float PI         = 3.14159265359;

// Mie phase function (Henyey-Greenstein)
float miePhase(float cosTheta) {
    float g = MIE_G;
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

float sampleShadowCascade0(vec3 worldPos) {
    // Use cascade 0 for volumetric (closest to camera, best quality)
    vec4 ls = uLightSpace[0] * vec4(worldPos, 1.0);
    vec3 proj = ls.xyz / ls.w * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0)
        return 1.0;
    return texture(uShadowMap, vec4(proj.xy, 0.0, proj.z));
}

void main() {
    // Reconstruct world position from depth
    float depth = texture(uDepth, vUV).r;
    vec2 ndc = vUV * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos4 = uInvVP * clipPos;
    vec3 worldPos = worldPos4.xyz / worldPos4.w;

    // Ray from camera to fragment
    vec3 rayDir = normalize(worldPos - uCamPos);
    float maxDist = min(length(worldPos - uCamPos), 100.0);
    float stepSize = maxDist / float(NUM_STEPS);

    // Phase function
    float cosTheta = dot(rayDir, normalize(uSunDir));
    float phase = miePhase(cosTheta);

    float accumScattering = 0.0;
    vec3 rayPos = uCamPos;

    for (int i = 0; i < NUM_STEPS; ++i) {
        rayPos += rayDir * stepSize;

        float inLight = sampleShadowCascade0(rayPos);
        accumScattering += inLight * stepSize * SCATTERING;
    }

    accumScattering *= phase;

    // Sun color contribution
    vec3 sunColor = vec3(1.0, 0.9, 0.7) * uDaylight;
    vec3 volumetric = sunColor * accumScattering;

    FragColor = vec4(volumetric, 1.0);
}
