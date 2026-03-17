#include "generation/Heightmap.h"
#include <algorithm>
#include <cmath>

static constexpr int SEA_LEVEL = 62;

Heightmap::Heightmap(uint32_t seed)
    : m_base(seed)
    , m_mountain(seed + 1)
    , m_erosion(seed + 2)
    , m_detail(seed + 3)
{}

int Heightmap::getHeight(float wx, float wz,
                         float heightMul, float roughness) const
{
    // Continent-scale base terrain   [0..1]
    float base = m_base.fbm2D(wx * 0.002f, wz * 0.002f, 6) * 0.5f + 0.5f;

    // Ridge noise for sharp mountain peaks   [0..1]
    float ridge = m_mountain.ridgeNoise2D(wx * 0.003f, wz * 0.003f, 5);

    // Erosion noise carves valleys   [0..1]
    float erosion = m_erosion.fbm2D(wx * 0.005f + 100.0f,
                                    wz * 0.005f + 100.0f, 4) * 0.5f + 0.5f;

    // Fine detail   [0..1]
    float detail = m_detail.fbm2D(wx * 0.02f, wz * 0.02f, 3) * 0.5f + 0.5f;

    float h = base * 40.0f
            + ridge * 60.0f * heightMul
            + detail * 10.0f * roughness
            - erosion * 20.0f;

    h += static_cast<float>(SEA_LEVEL);

    return std::clamp(static_cast<int>(h), 1, 254);
}
