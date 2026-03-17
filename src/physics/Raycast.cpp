#include "physics/Raycast.h"
#include "world/World.h"
#include "world/BlockRegistry.h"
#include <cmath>

// DDA voxel traversal (Amanatides & Woo)
RaycastHit Raycast::cast(const World& world,
                          const glm::vec3& origin,
                          const glm::vec3& dir,
                          float maxDist)
{
    RaycastHit result;
    if (glm::length(dir) < 1e-9f) return result;

    glm::vec3 d = glm::normalize(dir);

    glm::ivec3 pos(
        static_cast<int>(std::floor(origin.x)),
        static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z)));

    glm::ivec3 step;
    glm::vec3  tMax, tDelta;

    for (int i = 0; i < 3; ++i) {
        if (d[i] > 0) {
            step[i]   = 1;
            tMax[i]   = (static_cast<float>(pos[i]) + 1.0f - origin[i]) / d[i];
            tDelta[i] = 1.0f / d[i];
        } else if (d[i] < 0) {
            step[i]   = -1;
            tMax[i]   = (static_cast<float>(pos[i]) - origin[i]) / d[i];
            tDelta[i] = 1.0f / (-d[i]);
        } else {
            step[i]   = 0;
            tMax[i]   = 1e30f;
            tDelta[i] = 1e30f;
        }
    }

    float t = 0.0f;
    glm::ivec3 prev = pos;

    while (t < maxDist) {
        uint8_t id = world.getBlock(pos.x, pos.y, pos.z);
        const Block& b = BlockRegistry::get(id);
        if (b.solid) {
            result.hit      = true;
            result.blockPos = pos;
            result.prevPos  = prev;
            result.normal   = prev - pos;
            result.distance = t;
            return result;
        }

        prev = pos;
        if (tMax.x < tMax.y && tMax.x < tMax.z) {
            t      = tMax.x;
            pos.x += step.x;
            tMax.x += tDelta.x;
        } else if (tMax.y < tMax.z) {
            t      = tMax.y;
            pos.y += step.y;
            tMax.y += tDelta.y;
        } else {
            t      = tMax.z;
            pos.z += step.z;
            tMax.z += tDelta.z;
        }
    }

    return result;
}
