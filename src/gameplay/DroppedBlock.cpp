#include "gameplay/DroppedBlock.h"
#include "gameplay/Inventory.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

static float randFloat(float lo, float hi) {
    float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return lo + t * (hi - lo);
}

void DroppedBlockManager::spawn(const glm::ivec3& blockPos, uint8_t blockId) {
    DroppedBlock d;
    d.pos = glm::vec3(blockPos) + glm::vec3(0.5f, 0.3f, 0.5f); // center of block
    // Random pop-out velocity
    d.vel = glm::vec3(
        randFloat(-1.5f, 1.5f),
        randFloat(3.0f, 5.0f),   // pop upward
        randFloat(-1.5f, 1.5f)
    );
    d.blockId  = blockId;
    d.age      = 0.0f;
    d.rotation = randFloat(0.0f, 6.28f);
    d.bobPhase = randFloat(0.0f, 6.28f);
    d.floorY   = static_cast<float>(blockPos.y); // ground is the broken block's Y
    d.collected = false;

    m_drops.push_back(d);
}

void DroppedBlockManager::update(float dt, const glm::vec3& playerPos, Inventory& inv) {
    for (auto& d : m_drops) {
        if (d.collected) continue;

        d.age += dt;

        // Despawn after lifetime
        if (d.age > DroppedBlock::LIFETIME) {
            d.collected = true;
            continue;
        }

        // Animation
        d.rotation += DroppedBlock::SPIN_SPEED * dt;
        d.bobPhase += DroppedBlock::BOB_SPEED * dt;

        // Physics: gravity + friction
        d.vel.y += DroppedBlock::GRAVITY * dt;
        d.pos += d.vel * dt;

        // Simple ground collision
        float ground = d.floorY + 0.1f;
        if (d.pos.y < ground) {
            d.pos.y = ground;
            d.vel.y = 0.0f;
        }

        // Horizontal friction
        d.vel.x *= DroppedBlock::FRICTION;
        d.vel.z *= DroppedBlock::FRICTION;

        // Dampen vertical bounce
        if (d.pos.y <= ground + 0.05f && std::fabs(d.vel.y) < 0.5f) {
            d.vel.y = 0.0f;
        }

        // Pickup logic
        if (d.age < DroppedBlock::PICKUP_DELAY) continue;

        float dist = glm::length(playerPos - d.pos);

        // Magnet: fly toward player when close
        if (dist < DroppedBlock::MAGNET_RANGE && dist > 0.01f) {
            glm::vec3 dir = glm::normalize(playerPos - d.pos);
            float speed = 6.0f * (1.0f - dist / DroppedBlock::MAGNET_RANGE);
            d.vel = dir * speed + glm::vec3(0.0f, 1.0f, 0.0f); // slight lift
        }

        // Pickup
        if (dist < DroppedBlock::PICKUP_RANGE) {
            if (inv.addItem(d.blockId, 1)) {
                d.collected = true;
            }
        }
    }

    // Remove collected drops
    m_drops.erase(
        std::remove_if(m_drops.begin(), m_drops.end(),
            [](const DroppedBlock& d) { return d.collected; }),
        m_drops.end()
    );
}
