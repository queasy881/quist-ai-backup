#version 460 core

// Screen-Space Ambient Occlusion (SSAO) using hemisphere sampling.

in vec2 vUV;

uniform sampler2D uPosition;  // G-Buffer world position + AO
uniform sampler2D uNormal;    // G-Buffer normal + metallic
uniform sampler2D uNoise;     // 4x4 noise texture

uniform mat4 uProj;
uniform mat4 uView;
uniform vec2 uNoiseScale;

uniform vec3 uSamples[64];   // tangent-space hemisphere samples

out float FragColor;

const int KERNEL_SIZE = 64;
const float RADIUS    = 1.5;
const float BIAS      = 0.025;

void main() {
    vec3 worldPos = texture(uPosition, vUV).rgb;
    vec3 normal   = normalize(texture(uNormal, vUV).rgb * 2.0 - 1.0);

    // View-space position and normal
    vec3 viewPos    = (uView * vec4(worldPos, 1.0)).xyz;
    vec3 viewNormal = normalize(mat3(uView) * normal);

    // Random rotation from noise texture
    vec3 randomVec = texture(uNoise, vUV * uNoiseScale).xyz;

    // Gram-Schmidt to construct TBN
    vec3 tangent   = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, viewNormal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        // Tangent → view space
        vec3 samplePos = TBN * uSamples[i];
        samplePos = viewPos + samplePos * RADIUS;

        // Project to screen
        vec4 offset = uProj * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz  = offset.xyz * 0.5 + 0.5;

        // Sample depth at this position
        vec3 sampleWorldPos = texture(uPosition, offset.xy).rgb;
        float sampleDepth   = (uView * vec4(sampleWorldPos, 1.0)).z;

        // Range check and accumulate
        float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(viewPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + BIAS ? 1.0 : 0.0) * rangeCheck;
    }

    FragColor = 1.0 - (occlusion / float(KERNEL_SIZE));
}
