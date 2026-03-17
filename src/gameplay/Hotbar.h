#pragma once
#include "gameplay/Inventory.h"

class Hotbar {
public:
    static constexpr int SLOTS = 9;

    Hotbar(Inventory& inv);

    void scroll(float delta);
    void select(int slot);

    int  selected() const { return m_selected; }
    const ItemStack& selectedItem() const;

    Inventory& inventory() { return m_inv; }
    const Inventory& inventory() const { return m_inv; }

private:
    Inventory& m_inv;
    int m_selected = 0;
};
