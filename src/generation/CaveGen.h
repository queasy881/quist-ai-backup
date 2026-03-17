#pragma once
#include "generation/Noise.h"

class CaveGen {
public:
    explicit CaveGen(uint32_t seed);

    bool isCave(int worldX, int worldY, int worldZ) const;

private:
    bool isCheeseCave(float x, float y, float z) const;
    bool isSpaghettiCave(float x, float y, float z) const;
    bool isCavern(float x, float y, float z) const;
    bool isVerticalShaft(float x, float y, float z) const;

    Noise m_cheese;
    Noise m_spag1;
    Noise m_spag2;
    Noise m_cavern;
    Noise m_shaft;
};
