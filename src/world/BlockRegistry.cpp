#include "world/BlockRegistry.h"

Block BlockRegistry::s_blocks[static_cast<int>(BlockID::COUNT)];

void BlockRegistry::init() {
    //                 id   name         hard  solid transp grav  tTop tSide tBot
    s_blocks[0]  = {  0, "Air",         0.0f, false, true,  false, 0,  0,  0  };
    s_blocks[1]  = {  1, "Grass",       1.0f, true,  false, false, 0,  1,  2  };
    s_blocks[2]  = {  2, "Dirt",        0.8f, true,  false, false, 2,  2,  2  };
    s_blocks[3]  = {  3, "Stone",       3.0f, true,  false, false, 3,  3,  3  };
    s_blocks[4]  = {  4, "Sand",        0.5f, true,  false, true,  4,  4,  4  };
    s_blocks[5]  = {  5, "Water",       0.0f, false, true,  false, 5,  5,  5  };
    s_blocks[6]  = {  6, "Wood",        2.0f, true,  false, false, 11, 6,  11 };
    s_blocks[7]  = {  7, "Leaves",      0.3f, true,  true,  false, 7,  7,  7  };
    s_blocks[8]  = {  8, "Snow",        0.5f, true,  false, false, 8,  8,  8  };
    s_blocks[9]  = {  9, "DeepStone",   4.0f, true,  false, false, 9,  9,  9  };
    s_blocks[10] = { 10, "Gravel",      1.0f, true,  false, true, 10, 10, 10  };
}

const Block& BlockRegistry::get(uint8_t id) {
    if (id >= static_cast<uint8_t>(BlockID::COUNT)) return s_blocks[0];
    return s_blocks[id];
}

const Block& BlockRegistry::get(BlockID id) {
    return get(static_cast<uint8_t>(id));
}
