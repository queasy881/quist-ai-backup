#pragma once
#include "generation/Noise.h"

struct BiomeParams {
    float heightMultiplier;
    float roughness;
};

class Heightmap {
public:
    explicit Heightmap(uint32_t seed);

    int getHeight(float worldX, float worldZ,
                  float heightMul, float roughness) const;

private:
    Noise m_base;
    Noise m_mountain;
    Noise m_erosion;
    Noise m_detail;
};
