#pragma once
#include <glm/glm.hpp>

class World;

struct RaycastHit {
    bool      hit       = false;
    glm::ivec3 blockPos = {0,0,0};
    glm::ivec3 prevPos  = {0,0,0}; // adjacent air block (for placement)
    glm::ivec3 normal   = {0,0,0};
    float      distance = 0.0f;
};

class Raycast {
public:
    static RaycastHit cast(const World& world,
                           const glm::vec3& origin,
                           const glm::vec3& direction,
                           float maxDist = 6.0f);
};
