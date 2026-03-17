#version 460 core

// Bilateral blur for SSAO (4x4 kernel, preserves edges).

in vec2 vUV;

uniform sampler2D uInput;

out float FragColor;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(uInput, 0));
    float result = 0.0;

    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(uInput, vUV + offset).r;
        }
    }

    FragColor = result / 16.0;
}
