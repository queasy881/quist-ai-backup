#version 460 core

// Bloom upsample with 9-tap tent filter.

in vec2 vUV;

uniform sampler2D uInput;
uniform float uFilterRadius;

out vec3 FragColor;

void main() {
    // 9-tap tent filter (3x3 grid of bilinear samples)
    float x = uFilterRadius;
    float y = uFilterRadius;

    vec3 a = texture(uInput, vUV + vec2(-x, -y)).rgb;
    vec3 b = texture(uInput, vUV + vec2( 0, -y)).rgb;
    vec3 c = texture(uInput, vUV + vec2( x, -y)).rgb;

    vec3 d = texture(uInput, vUV + vec2(-x, 0)).rgb;
    vec3 e = texture(uInput, vUV).rgb;
    vec3 f = texture(uInput, vUV + vec2( x, 0)).rgb;

    vec3 g = texture(uInput, vUV + vec2(-x,  y)).rgb;
    vec3 h = texture(uInput, vUV + vec2( 0,  y)).rgb;
    vec3 i = texture(uInput, vUV + vec2( x,  y)).rgb;

    // Weights: center=4, edges=2, corners=1 → total=16
    FragColor = (e * 4.0 + (b + d + f + h) * 2.0 + (a + c + g + i)) / 16.0;
}
