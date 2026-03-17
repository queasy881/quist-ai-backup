#pragma once
#include "generation/Noise.h"
#include "world/Block.h"
#include <glm/glm.hpp>

enum class BiomeType { Plains, Forest, Mountains, Desert, Snow };

struct BiomeData {
    BiomeType type        = BiomeType::Plains;
    uint8_t   surfaceBlock    = static_cast<uint8_t>(BlockID::Grass);
    uint8_t   subSurfaceBlock = static_cast<uint8_t>(BlockID::Dirt);
    float     heightMultiplier = 0.4f;
    float     roughness       = 0.5f;
    int       surfaceDepth    = 4;
    glm::vec3 fogColor        = glm::vec3(0.6f, 0.75f, 1.0f);
};

class BiomeGen {
public:
    explicit BiomeGen(uint32_t seed);

    BiomeData getBiome(float worldX, float worldZ) const;

    float getTemperature(float wx, float wz) const;
    float getHumidity(float wx, float wz) const;

private:
    Noise m_tempNoise;
    Noise m_humidNoise;

    static BiomeData makeBiome(float temp, float humid, float elev);
};
