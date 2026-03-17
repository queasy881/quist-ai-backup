#include "gameplay/Hotbar.h"

Hotbar::Hotbar(Inventory& inv) : m_inv(inv) {}

void Hotbar::scroll(float delta) {
    if (delta > 0)
        m_selected = (m_selected - 1 + SLOTS) % SLOTS;
    else if (delta < 0)
        m_selected = (m_selected + 1) % SLOTS;
}

void Hotbar::select(int s) {
    if (s >= 0 && s < SLOTS) m_selected = s;
}

const ItemStack& Hotbar::selectedItem() const {
    return m_inv.slot(m_selected);
}
