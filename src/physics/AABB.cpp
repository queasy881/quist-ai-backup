#include "physics/AABB.h"

AABB AABB::fromCenterSize(const glm::vec3& center, const glm::vec3& size) {
    glm::vec3 half = size * 0.5f;
    return AABB(center - half, center + half);
}

bool AABB::intersects(const AABB& o) const {
    return (min.x <= o.max.x && max.x >= o.min.x) &&
           (min.y <= o.max.y && max.y >= o.min.y) &&
           (min.z <= o.max.z && max.z >= o.min.z);
}

bool AABB::contains(const glm::vec3& p) const {
    return p.x >= min.x && p.x <= max.x &&
           p.y >= min.y && p.y <= max.y &&
           p.z >= min.z && p.z <= max.z;
}

AABB AABB::expanded(const glm::vec3& a) const  { return {min - a, max + a}; }
AABB AABB::translated(const glm::vec3& o) const { return {min + o, max + o}; }
glm::vec3 AABB::getCenter() const { return (min + max) * 0.5f; }
glm::vec3 AABB::getSize()   const { return max - min; }
