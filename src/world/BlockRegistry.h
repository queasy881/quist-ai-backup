#pragma once
#include "world/Block.h"

/*  Texture-atlas slot assignments:
 *   0  grass_top        5  water
 *   1  grass_side       6  wood
 *   2  dirt             7  leaves
 *   3  stone            8  snow
 *   4  sand             9  deepstone
 *                      10  gravel
 */
class BlockRegistry {
public:
    static void init();
    static const Block& get(uint8_t id);
    static const Block& get(BlockID id);

private:
    static Block s_blocks[static_cast<int>(BlockID::COUNT)];
};
