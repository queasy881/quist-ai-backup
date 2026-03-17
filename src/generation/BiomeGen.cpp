#include "generation/BiomeGen.h"
#include <cmath>

BiomeGen::BiomeGen(uint32_t seed)
    : m_tempNoise(seed + 10)
    , m_humidNoise(seed + 20)
{}

float BiomeGen::getTemperature(float wx, float wz) const {
    return m_tempNoise.fbm2D(wx * 0.0008f, wz * 0.0008f, 4) * 0.5f + 0.5f;
}

float BiomeGen::getHumidity(float wx, float wz) const {
    return m_humidNoise.fbm2D(wx * 0.0008f + 500.0f,
                              wz * 0.0008f + 500.0f, 4) * 0.5f + 0.5f;
}

BiomeData BiomeGen::getBiome(float worldX, float worldZ) const {
    float temp  = getTemperature(worldX, worldZ);
    float humid = getHumidity(worldX, worldZ);
    return makeBiome(temp, humid, 0.0f);
}

BiomeData BiomeGen::makeBiome(float temp, float humid, float /*elev*/) {
    BiomeData b;

    if (temp < 0.25f) {
        // ── Snow / Ice ──
        b.type             = BiomeType::Snow;
        b.surfaceBlock     = static_cast<uint8_t>(BlockID::Snow);
        b.subSurfaceBlock  = static_cast<uint8_t>(BlockID::Dirt);
        b.heightMultiplier = 0.35f;
        b.roughness        = 0.3f;
        b.surfaceDepth     = 3;
        b.fogColor         = glm::vec3(0.8f, 0.85f, 0.95f);
    }
    else if (temp > 0.75f && humid < 0.35f) {
        // ── Desert ──
        b.type             = BiomeType::Desert;
        b.surfaceBlock     = static_cast<uint8_t>(BlockID::Sand);
        b.subSurfaceBlock  = static_cast<uint8_t>(BlockID::Sand);
        b.heightMultiplier = 0.15f;
        b.roughness        = 0.2f;
        b.surfaceDepth     = 6;
        b.fogColor         = glm::vec3(0.93f, 0.86f, 0.60f);
    }
    else if (temp > 0.45f && humid > 0.55f) {
        // ── Forest ──
        b.type             = BiomeType::Forest;
        b.surfaceBlock     = static_cast<uint8_t>(BlockID::Grass);
        b.subSurfaceBlock  = static_cast<uint8_t>(BlockID::Dirt);
        b.heightMultiplier = 0.45f;
        b.roughness        = 0.55f;
        b.surfaceDepth     = 4;
        b.fogColor         = glm::vec3(0.55f, 0.72f, 0.55f);
    }
    else if (temp > 0.35f && temp < 0.55f && humid > 0.25f && humid < 0.45f) {
        // ── Mountains (narrow band) ──
        b.type             = BiomeType::Mountains;
        b.surfaceBlock     = static_cast<uint8_t>(BlockID::Stone);
        b.subSurfaceBlock  = static_cast<uint8_t>(BlockID::Stone);
        b.heightMultiplier = 1.0f;
        b.roughness        = 0.9f;
        b.surfaceDepth     = 1;
        b.fogColor         = glm::vec3(0.7f, 0.75f, 0.85f);
    }
    else {
        // ── Plains (default) ──
        b.type             = BiomeType::Plains;
        b.surfaceBlock     = static_cast<uint8_t>(BlockID::Grass);
        b.subSurfaceBlock  = static_cast<uint8_t>(BlockID::Dirt);
        b.heightMultiplier = 0.3f;
        b.roughness        = 0.4f;
        b.surfaceDepth     = 4;
        b.fogColor         = glm::vec3(0.6f, 0.78f, 1.0f);
    }

    return b;
}
