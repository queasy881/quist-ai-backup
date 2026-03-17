#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct DroppedBlock {
    glm::vec3 pos;
    glm::vec3 vel;
    uint8_t   blockId;
    float     age       = 0.0f;   // seconds since spawn
    float     rotation  = 0.0f;   // Y-axis rotation in radians
    float     bobPhase  = 0.0f;   // bobbing phase offset
    float     floorY    = 0.0f;   // approximate ground level for this drop
    bool      collected = false;

    static constexpr float LIFETIME     = 60.0f;  // despawn after 60s
    static constexpr float PICKUP_DELAY = 0.5f;   // can't pick up immediately
    static constexpr float PICKUP_RANGE = 1.8f;   // auto-pickup radius
    static constexpr float MAGNET_RANGE = 3.0f;   // start flying toward player
    static constexpr float SCALE        = 0.25f;  // render size (quarter block)
    static constexpr float BOB_SPEED    = 2.5f;   // bobbing frequency
    static constexpr float BOB_HEIGHT   = 0.08f;  // bobbing amplitude
    static constexpr float SPIN_SPEED   = 1.5f;   // rotation speed rad/s
    static constexpr float GRAVITY      = -12.0f;
    static constexpr float FRICTION     = 0.92f;
};

class Inventory;

class DroppedBlockManager {
public:
    void spawn(const glm::ivec3& blockPos, uint8_t blockId);
    void update(float dt, const glm::vec3& playerPos, Inventory& inv);

    const std::vector<DroppedBlock>& drops() const { return m_drops; }

private:
    std::vector<DroppedBlock> m_drops;
};
