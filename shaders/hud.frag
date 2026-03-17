#version 460 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;
uniform int uUseTexture;  // 0 = solid color, 1 = font (alpha only), 2 = full texture

out vec4 FragColor;

void main() {
    if (uUseTexture == 1) {
        // Font atlas: white glyphs on transparent — use alpha * vertex color
        float a = texture(uTexture, vUV).a;
        FragColor = vec4(vColor.rgb, vColor.a * a);
    } else if (uUseTexture == 2) {
        // Block atlas: full RGBA texture color
        vec4 tex = texture(uTexture, vUV);
        FragColor = tex * vColor;
    } else {
        FragColor = vColor;
    }
}
