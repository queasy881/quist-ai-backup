#include "world/Chunk.h"

Chunk::Chunk(int cx, int cz) : m_cx(cx), m_cz(cz) {
    std::memset(m_blocks, 0, sizeof(m_blocks));
}

uint8_t Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= HEIGHT || z < 0 || z >= SIZE)
        return 0; // Air
    return m_blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, uint8_t id) {
    if (x < 0 || x >= SIZE || y < 0 || y >= HEIGHT || z < 0 || z >= SIZE)
        return;
    m_blocks[index(x, y, z)] = id;
    m_dirty = true;
}
