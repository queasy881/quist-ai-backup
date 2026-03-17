#include "gameplay/Inventory.h"

Inventory::Inventory() {
    // Start with empty inventory — mine blocks to collect them
}

ItemStack& Inventory::slot(int i) {
    static ItemStack dummy;
    if (i < 0 || i >= SIZE) return dummy;
    return m_slots[i];
}

const ItemStack& Inventory::slot(int i) const {
    static const ItemStack dummy;
    if (i < 0 || i >= SIZE) return dummy;
    return m_slots[i];
}

ItemStack& Inventory::slot(int col, int row) {
    return slot(row * COLS + col);
}

bool Inventory::addItem(uint8_t blockId, int count) {
    // First pass: merge into existing stacks
    for (int i = 0; i < SIZE && count > 0; ++i) {
        if (m_slots[i].blockId == blockId && m_slots[i].count < MAX_STACK) {
            int add = std::min(count, MAX_STACK - m_slots[i].count);
            m_slots[i].count += add;
            count -= add;
        }
    }
    // Second pass: fill empty slots
    for (int i = 0; i < SIZE && count > 0; ++i) {
        if (m_slots[i].empty()) {
            int add = std::min(count, MAX_STACK);
            m_slots[i] = { blockId, add };
            count -= add;
        }
    }
    return count == 0;
}

bool Inventory::removeItem(uint8_t blockId, int count) {
    for (int i = SIZE - 1; i >= 0 && count > 0; --i) {
        if (m_slots[i].blockId == blockId) {
            int rem = std::min(count, m_slots[i].count);
            m_slots[i].count -= rem;
            count -= rem;
            if (m_slots[i].count <= 0) m_slots[i] = {};
        }
    }
    return count == 0;
}

void Inventory::swapSlots(int a, int b) {
    if (a < 0 || a >= SIZE || b < 0 || b >= SIZE) return;
    std::swap(m_slots[a], m_slots[b]);
}

void Inventory::splitStack(int src, int dst) {
    if (src < 0 || src >= SIZE || dst < 0 || dst >= SIZE) return;
    if (m_slots[src].empty() || !m_slots[dst].empty()) return;
    int half = m_slots[src].count / 2;
    if (half <= 0) return;
    m_slots[dst] = { m_slots[src].blockId, half };
    m_slots[src].count -= half;
}
