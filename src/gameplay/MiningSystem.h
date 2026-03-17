#pragma once
#include "physics/Raycast.h"

class World;
class Inventory;
class DroppedBlockManager;

class MiningSystem {
public:
    static constexpr int   CRACK_STAGES = 8;
    static constexpr float MINE_SPEED   = 1.0f; // multiplier

    void update(const RaycastHit& hit, bool mining, float dt,
                World& world, DroppedBlockManager& drops);

    float progress()  const { return m_progress; }
    int   crackStage() const;
    bool  isActive()   const { return m_active; }
    glm::ivec3 targetBlock() const { return m_target; }

private:
    bool       m_active   = false;
    float      m_progress = 0.0f;
    glm::ivec3 m_target   = {0,0,0};
};
