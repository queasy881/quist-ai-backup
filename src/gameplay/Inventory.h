#pragma once
#include <cstdint>
#include <algorithm>

struct ItemStack {
    uint8_t blockId = 0;
    int     count   = 0;

    bool empty() const { return blockId == 0 || count <= 0; }
};

class Inventory {
public:
    static constexpr int COLS = 9;
    static constexpr int ROWS = 4;
    static constexpr int SIZE = COLS * ROWS;
    static constexpr int MAX_STACK = 64;

    Inventory();

    ItemStack& slot(int index);
    const ItemStack& slot(int index) const;
    ItemStack& slot(int col, int row);

    bool addItem(uint8_t blockId, int count = 1);
    bool removeItem(uint8_t blockId, int count = 1);
    void swapSlots(int a, int b);
    void splitStack(int src, int dst);

private:
    ItemStack m_slots[SIZE];
};
