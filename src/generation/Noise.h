#pragma once
#include <cstdint>

class Noise {
public:
    explicit Noise(uint32_t seed = 0);

    float perlin2D(float x, float y) const;
    float perlin3D(float x, float y, float z) const;

    float fbm2D(float x, float y, int octaves = 6,
                float lacunarity = 2.0f, float gain = 0.5f) const;
    float fbm3D(float x, float y, float z, int octaves = 6,
                float lacunarity = 2.0f, float gain = 0.5f) const;

    float ridgeNoise2D(float x, float y, int octaves = 6,
                       float lacunarity = 2.0f, float gain = 0.5f) const;

private:
    uint8_t perm[512];

    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad2(int hash, float x, float y);
    static float grad3(int hash, float x, float y, float z);
};
