#include "generation/CaveGen.h"
#include <cmath>

CaveGen::CaveGen(uint32_t seed)
    : m_cheese(seed + 100)
    , m_spag1(seed + 200)
    , m_spag2(seed + 300)
    , m_cavern(seed + 400)
    , m_shaft(seed + 500)
{}

bool CaveGen::isCave(int wx, int wy, int wz) const {
    if (wy <= 1 || wy >= 255) return false;

    float x = static_cast<float>(wx);
    float y = static_cast<float>(wy);
    float z = static_cast<float>(wz);

    return isCheeseCave(x, y, z)
        || isSpaghettiCave(x, y, z)
        || isCavern(x, y, z)
        || isVerticalShaft(x, y, z);
}

// ── Cheese caves: 3-D noise pockets that widen with depth ──
bool CaveGen::isCheeseCave(float x, float y, float z) const {
    float n = m_cheese.perlin3D(x * 0.015f, y * 0.02f, z * 0.015f);
    // Threshold decreases with depth → bigger caves lower down
    float threshold = 0.58f - (1.0f - y / 128.0f) * 0.15f;
    return n > threshold;
}

// ── Spaghetti caves: two 3-D noise fields intersecting near zero ──
bool CaveGen::isSpaghettiCave(float x, float y, float z) const {
    float n1 = m_spag1.perlin3D(x * 0.03f, y * 0.03f, z * 0.03f);
    float n2 = m_spag2.perlin3D(x * 0.03f, y * 0.03f, z * 0.03f);
    // Tunnel width: minimum 2-3 blocks, widens with depth
    float r = 0.04f + (1.0f - y / 128.0f) * 0.02f;
    return (n1 * n1 + n2 * n2) < r * r;
}

// ── Large caverns deep underground ──
bool CaveGen::isCavern(float x, float y, float z) const {
    if (y > 40.0f) return false;
    float n = m_cavern.perlin3D(x * 0.01f, y * 0.015f, z * 0.01f);
    return n > 0.42f;
}

// ── Vertical shafts (at least 2-3 blocks wide) ──
bool CaveGen::isVerticalShaft(float x, float y, float z) const {
    if (y > 80.0f) return false;
    float n = m_shaft.perlin2D(x * 0.035f, z * 0.035f);
    return std::fabs(n) < 0.018f;
}
