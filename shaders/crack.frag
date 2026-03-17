#version 460 core

in vec2 vUV;
uniform int uStage; // 0-9 crack stage

out vec4 FragColor;

// --- Pixel-art style hash (snaps to 16x16 texel grid like Minecraft) ---
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453);
}

float hash1(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Voronoi distance for chunky fracture cells
float voronoi(vec2 uv, float jitter) {
    vec2 cell = floor(uv);
    vec2 frac_uv = fract(uv);
    float minDist = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(cell + neighbor) * jitter;
            float d = length(neighbor + point - frac_uv);
            minDist = min(minDist, d);
        }
    }
    return minDist;
}

// Voronoi edge detection (distance to nearest cell border)
float voronoiEdge(vec2 uv, float jitter) {
    vec2 cell = floor(uv);
    vec2 frac_uv = fract(uv);
    float d1 = 1.0, d2 = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(cell + neighbor) * jitter;
            float d = length(neighbor + point - frac_uv);
            if (d < d1) { d2 = d1; d1 = d; }
            else if (d < d2) { d2 = d; }
        }
    }
    return d2 - d1;
}

void main() {
    if (uStage < 0) discard;

    // Pixelate UVs to 16x16 grid for that blocky Minecraft look
    vec2 pixUV = floor(vUV * 16.0) / 16.0 + 0.5 / 16.0;

    float stage = float(uStage);
    float crack = 0.0;
    float highlight = 0.0;

    // Layer 1: Large fracture lines (Voronoi edges) — appear from stage 0
    {
        float scale = 3.0 + stage * 0.3;
        float edge = voronoiEdge(pixUV * scale, 0.8);
        // Crack line thickness increases with stage
        float thickness = 0.04 + stage * 0.025;
        float line = 1.0 - smoothstep(0.0, thickness, edge);
        crack = max(crack, line);
        // White highlight along crack edge
        float hlEdge = smoothstep(thickness, thickness + 0.03, edge)
                     * (1.0 - smoothstep(thickness + 0.03, thickness + 0.08, edge));
        highlight = max(highlight, hlEdge * 0.4);
    }

    // Layer 2: Medium crack network — appears from stage 2
    if (uStage >= 2) {
        float scale = 5.0 + stage * 0.5;
        float edge = voronoiEdge(pixUV * scale + 7.3, 0.9);
        float thickness = 0.03 + (stage - 2.0) * 0.02;
        float line = 1.0 - smoothstep(0.0, thickness, edge);
        crack = max(crack, line * 0.85);
        highlight = max(highlight, smoothstep(thickness, thickness + 0.04, edge)
                       * (1.0 - smoothstep(thickness + 0.04, thickness + 0.09, edge)) * 0.3);
    }

    // Layer 3: Fine fractures — appears from stage 5
    if (uStage >= 5) {
        float scale = 8.0 + stage * 0.8;
        float edge = voronoiEdge(pixUV * scale + 13.7, 1.0);
        float thickness = 0.02 + (stage - 5.0) * 0.015;
        float line = 1.0 - smoothstep(0.0, thickness, edge);
        crack = max(crack, line * 0.7);
    }

    // Layer 4: Chunky pixel noise at high stages (fragmentation)
    if (uStage >= 7) {
        float n = hash1(floor(vUV * 16.0));
        float threshold = 0.7 - (stage - 7.0) * 0.15;
        if (n > threshold) crack = max(crack, 0.6);
    }

    crack = clamp(crack, 0.0, 1.0);

    // Overall darkening that increases with stage
    float baseDark = stage / 9.0 * 0.35;

    // Final color: black cracks with slight white highlight edges
    float alpha = baseDark + crack * (0.55 + stage * 0.03);
    vec3 color = mix(vec3(0.0), vec3(0.6), highlight * (1.0 - crack));

    FragColor = vec4(color, clamp(alpha, 0.0, 0.95));
}
