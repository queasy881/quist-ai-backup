#pragma once
#include <cstdint>

enum class BlockID : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Water,
    Wood,
    Leaves,
    Snow,
    DeepStone,
    Gravel,
    COUNT
};

struct Block {
    uint8_t     id;
    const char* name;
    float       hardness;
    bool        solid;
    bool        transparent;
    bool        gravityAffected;
    int         texTop;
    int         texSide;
    int         texBottom;

    bool isAir()   const { return id == static_cast<uint8_t>(BlockID::Air); }
    bool isFluid() const { return id == static_cast<uint8_t>(BlockID::Water); }
};
