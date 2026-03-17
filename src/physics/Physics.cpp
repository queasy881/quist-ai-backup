#include "physics/Physics.h"
#include "world/World.h"
#include "world/BlockRegistry.h"
#include <cmath>
#include <algorithm>

bool Physics::collidesWithWorld(const World& world, const AABB& box) {
    int minX = static_cast<int>(std::floor(box.min.x));
    int minY = static_cast<int>(std::floor(box.min.y));
    int minZ = static_cast<int>(std::floor(box.min.z));
    int maxX = static_cast<int>(std::floor(box.max.x));
    int maxY = static_cast<int>(std::floor(box.max.y));
    int maxZ = static_cast<int>(std::floor(box.max.z));

    for (int y = minY; y <= maxY; ++y)
        for (int z = minZ; z <= maxZ; ++z)
            for (int x = minX; x <= maxX; ++x) {
                uint8_t id = world.getBlock(x, y, z);
                if (BlockRegistry::get(id).solid) {
                    AABB blockBox(glm::vec3(x, y, z), glm::vec3(x+1, y+1, z+1));
                    if (box.intersects(blockBox)) return true;
                }
            }
    return false;
}

glm::vec3 Physics::moveAndSlide(const World& world,
                                 const AABB& box,
                                 glm::vec3& vel,
                                 float dt,
                                 bool& onGround)
{
    glm::vec3 pos = box.getCenter();
    glm::vec3 half = box.getSize() * 0.5f;

    // Apply gravity
    vel.y += GRAVITY * dt;
    vel.y  = std::max(vel.y, TERMINAL_VEL);

    // Resolve each axis independently
    auto tryMove = [&](int axis) {
        glm::vec3 newPos = pos;
        newPos[axis] += vel[axis] * dt;
        AABB test = AABB::fromCenterSize(newPos, half * 2.0f);
        if (collidesWithWorld(world, test)) {
            // Binary-search narrowing (6 iterations is plenty)
            float lo = 0.0f, hi = vel[axis] * dt;
            for (int i = 0; i < 6; ++i) {
                float mid = (lo + hi) * 0.5f;
                glm::vec3 tp = pos; tp[axis] += mid;
                if (collidesWithWorld(world, AABB::fromCenterSize(tp, half * 2.0f)))
                    hi = mid;
                else
                    lo = mid;
            }
            pos[axis] += lo;
            if (axis == 1 && vel.y < 0.0f) onGround = true;
            vel[axis] = 0.0f;
        } else {
            pos = newPos;
        }
    };

    onGround = false;
    tryMove(1); // Y first (gravity)
    tryMove(0); // X
    tryMove(2); // Z

    return pos;
}
