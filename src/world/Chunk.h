#pragma once
#include <cstdint>
#include <atomic>
#include <cstring>

class Chunk {
public:
    static constexpr int SIZE   = 16;
    static constexpr int HEIGHT = 256;
    static constexpr int VOLUME = SIZE * HEIGHT * SIZE;

    enum class State { Empty, Generating, Generated, Meshing, Meshed, Ready };

    Chunk(int cx, int cz);

    uint8_t getBlock(int x, int y, int z) const;
    void    setBlock(int x, int y, int z, uint8_t id);

    int getX() const { return m_cx; }
    int getZ() const { return m_cz; }

    State getState() const { return m_state.load(std::memory_order_acquire); }
    void  setState(State s){ m_state.store(s, std::memory_order_release); }

    const uint8_t* rawBlocks() const { return m_blocks; }

    bool isDirty()  const { return m_dirty; }
    void setDirty(bool d) { m_dirty = d; }

private:
    int m_cx, m_cz;
    uint8_t m_blocks[VOLUME];
    std::atomic<State> m_state{State::Empty};
    bool m_dirty = false;

    static int index(int x, int y, int z) {
        return (y * SIZE * SIZE) + (z * SIZE) + x;
    }
};
