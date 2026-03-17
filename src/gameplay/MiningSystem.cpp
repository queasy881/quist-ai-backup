#include "gameplay/MiningSystem.h"
#include "world/World.h"
#include "world/BlockRegistry.h"
#include "gameplay/DroppedBlock.h"

void MiningSystem::update(const RaycastHit& hit, bool mining, float dt,
                           World& world, DroppedBlockManager& drops)
{
    if (!mining || !hit.hit) {
        m_active   = false;
        m_progress = 0.0f;
        return;
    }

    if (!m_active || m_target != hit.blockPos) {
        m_active   = true;
        m_target   = hit.blockPos;
        m_progress = 0.0f;
    }

    uint8_t id = world.getBlock(m_target.x, m_target.y, m_target.z);
    const Block& block = BlockRegistry::get(id);
    if (!block.solid) { m_active = false; return; }

    float hardness = block.hardness;
    if (hardness <= 0.0f) hardness = 0.1f;

    m_progress += (MINE_SPEED / hardness) * dt;

    if (m_progress >= 1.0f) {
        // Break block
        world.setBlock(m_target.x, m_target.y, m_target.z, 0);
        drops.spawn(m_target, id);
        m_progress = 0.0f;
        m_active   = false;

        // Gravity: check blocks above for gravity-affected types
        for (int y = m_target.y + 1; y < Chunk::HEIGHT; ++y) {
            uint8_t above = world.getBlock(m_target.x, y, m_target.z);
            if (above == 0) break;
            const Block& ab = BlockRegistry::get(above);
            if (!ab.gravityAffected) break;
            // Simple: move block down instantly
            world.setBlock(m_target.x, y, m_target.z, 0);
            world.setBlock(m_target.x, y - 1, m_target.z, above);
        }
    }
}

int MiningSystem::crackStage() const {
    if (!m_active) return -1;
    return static_cast<int>(m_progress * (CRACK_STAGES - 1));
}
