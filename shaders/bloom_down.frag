#version 460 core

// Bloom downsample with 13-tap filter (dual kawase style).
// First pass also applies a brightness threshold.

in vec2 vUV;

uniform sampler2D uInput;
uniform vec2  uSrcResolution;
uniform int   uFirstPass;  // 1 = apply brightness threshold

out vec3 FragColor;

const float BLOOM_THRESHOLD = 1.0;
const float BLOOM_KNEE      = 0.5;

vec3 prefilter(vec3 c) {
    float brightness = max(c.r, max(c.g, c.b));
    float knee = BLOOM_THRESHOLD * BLOOM_KNEE;
    float soft = brightness - BLOOM_THRESHOLD + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - BLOOM_THRESHOLD) / max(brightness, 0.00001);
    return c * contribution;
}

void main() {
    vec2 texelSize = 1.0 / uSrcResolution;

    // 13-tap downsample (from Call of Duty: Advanced Warfare presentation)
    vec3 a = texture(uInput, vUV + texelSize * vec2(-1, -1)).rgb;
    vec3 b = texture(uInput, vUV + texelSize * vec2( 0, -1)).rgb;
    vec3 c = texture(uInput, vUV + texelSize * vec2( 1, -1)).rgb;
    vec3 d = texture(uInput, vUV + texelSize * vec2(-1,  0)).rgb;
    vec3 e = texture(uInput, vUV).rgb;
    vec3 f = texture(uInput, vUV + texelSize * vec2( 1,  0)).rgb;
    vec3 g = texture(uInput, vUV + texelSize * vec2(-1,  1)).rgb;
    vec3 h = texture(uInput, vUV + texelSize * vec2( 0,  1)).rgb;
    vec3 i = texture(uInput, vUV + texelSize * vec2( 1,  1)).rgb;

    vec3 j = texture(uInput, vUV + texelSize * vec2(-0.5, -0.5)).rgb;
    vec3 k = texture(uInput, vUV + texelSize * vec2( 0.5, -0.5)).rgb;
    vec3 l = texture(uInput, vUV + texelSize * vec2(-0.5,  0.5)).rgb;
    vec3 m = texture(uInput, vUV + texelSize * vec2( 0.5,  0.5)).rgb;

    vec3 result = e * 0.125;
    result += (a + c + g + i) * 0.03125;
    result += (b + d + f + h) * 0.0625;
    result += (j + k + l + m) * 0.125;

    if (uFirstPass == 1) {
        result = prefilter(result);
    }

    FragColor = max(result, 0.0);
}
