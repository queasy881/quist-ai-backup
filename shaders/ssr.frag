#version 460 core

// Screen-Space Reflections via ray marching in screen space.

in vec2 vUV;

uniform sampler2D uColor;          // HDR color buffer
uniform sampler2D uNormalMetallic;  // G-Buffer normals + metallic
uniform sampler2D uPositionAO;      // G-Buffer world position
uniform sampler2D uDepth;           // Depth buffer

uniform mat4 uView;
uniform mat4 uProj;
uniform vec2 uResolution;

out vec4 FragColor;

const int   MAX_STEPS    = 64;
const float STEP_SIZE    = 0.3;
const float MAX_DISTANCE = 50.0;
const float THICKNESS    = 0.5;

void main() {
    vec4 nm = texture(uNormalMetallic, vUV);
    float metallic = nm.a;

    // Only reflect on somewhat metallic/shiny surfaces
    // For voxels, reflect on water-like surfaces (detected by low roughness later)
    // For now, use a low threshold
    vec3 color = texture(uColor, vUV).rgb;

    if (metallic < 0.01) {
        FragColor = vec4(color, 1.0);
        return;
    }

    vec3 worldPos = texture(uPositionAO, vUV).rgb;
    vec3 normal   = normalize(nm.rgb * 2.0 - 1.0);

    vec3 viewPos    = (uView * vec4(worldPos, 1.0)).xyz;
    vec3 viewNormal = normalize(mat3(uView) * normal);

    // Reflect in view space
    vec3 viewDir   = normalize(viewPos);
    vec3 reflectDir = reflect(viewDir, viewNormal);

    // Ray march in view space
    vec3 hitColor = vec3(0.0);
    float alpha   = 0.0;

    vec3 rayPos = viewPos;
    for (int i = 0; i < MAX_STEPS; ++i) {
        rayPos += reflectDir * STEP_SIZE;

        // Project to screen
        vec4 projected = uProj * vec4(rayPos, 1.0);
        projected.xyz /= projected.w;
        vec2 sampleUV = projected.xy * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            break;

        // Compare depths
        float sampleDepth = (uView * vec4(texture(uPositionAO, sampleUV).rgb, 1.0)).z;
        float diff = rayPos.z - sampleDepth;

        if (diff > 0.0 && diff < THICKNESS) {
            hitColor = texture(uColor, sampleUV).rgb;
            // Fade at edges of screen
            float edgeFade = 1.0 - pow(max(abs(sampleUV.x - 0.5), abs(sampleUV.y - 0.5)) * 2.0, 2.0);
            alpha = clamp(edgeFade, 0.0, 1.0) * metallic;
            break;
        }

        if (length(rayPos - viewPos) > MAX_DISTANCE) break;
    }

    FragColor = vec4(mix(color, hitColor, alpha), 1.0);
}
