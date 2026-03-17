#version 460 core

// Final tone mapping: ACES filmic + bloom composite + vignette + chromatic aberration.

in vec2 vUV;

uniform sampler2D uHDR;       // 0: HDR color buffer
uniform sampler2D uBloom;     // 1: Bloom mip 0
uniform float uExposure;
uniform float uBloomStrength;
uniform float uTime;
uniform int   uPassthrough;   // 1 = just output uHDR without effects

out vec4 FragColor;

// ACES filmic tone mapping (Stephen Hill's fit)
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(uHDR, vUV).rgb;

    if (uPassthrough == 1) {
        FragColor = vec4(hdr, 1.0);
        return;
    }

    // Add bloom
    vec3 bloom = texture(uBloom, vUV).rgb;
    hdr += bloom * uBloomStrength;

    // Exposure
    hdr *= uExposure;

    // ACES filmic tone mapping
    vec3 ldr = ACESFilm(hdr);

    // Gamma correction
    ldr = pow(ldr, vec3(1.0 / 2.2));

    // Vignette
    vec2 uv = vUV - 0.5;
    float vignette = 1.0 - dot(uv, uv) * 0.5;
    vignette = clamp(vignette, 0.0, 1.0);
    vignette = pow(vignette, 0.8);
    ldr *= vignette;

    // Subtle chromatic aberration at edges
    float ca_strength = 0.0008;
    float dist2 = dot(uv, uv);
    vec2 ca_offset = uv * dist2 * ca_strength;

    // Sample HDR at offset positions and apply same processing
    vec3 hdr_r = texture(uHDR, vUV + ca_offset).rgb;
    vec3 hdr_b = texture(uHDR, vUV - ca_offset).rgb;
    hdr_r = (hdr_r + bloom * uBloomStrength) * uExposure;
    hdr_b = (hdr_b + bloom * uBloomStrength) * uExposure;
    float r = pow(ACESFilm(hdr_r).r, 1.0/2.2) * vignette;
    float b = pow(ACESFilm(hdr_b).b, 1.0/2.2) * vignette;
    ldr = vec3(r, ldr.g, b);

    // Color grading: slight warm tint
    ldr *= vec3(1.02, 1.0, 0.98);

    FragColor = vec4(ldr, 1.0);
}
