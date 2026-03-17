#pragma once
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min{0};
    glm::vec3 max{0};

    AABB() = default;
    AABB(const glm::vec3& mn, const glm::vec3& mx) : min(mn), max(mx) {}

    static AABB fromCenterSize(const glm::vec3& center, const glm::vec3& size);
    bool intersects(const AABB& other) const;
    bool contains(const glm::vec3& point) const;
    AABB expanded(const glm::vec3& amount) const;
    AABB translated(const glm::vec3& offset) const;
    glm::vec3 getCenter() const;
    glm::vec3 getSize() const;
};
