#pragma once
#include "player/Camera.h"
#include "physics/AABB.h"
#include <glm/glm.hpp>

class World;

class Player {
public:
    Player();

    void update(World& world, float dt);

    Camera&       getCamera()       { return m_camera; }
    const Camera& getCamera() const { return m_camera; }

    glm::vec3 getPosition() const { return m_pos; }
    void setPosition(const glm::vec3& pos) { m_pos = pos; }
    glm::vec3 getVelocity() const { return m_vel; }
    bool      isOnGround()  const { return m_onGround; }

    AABB getAABB() const;

    static constexpr float WIDTH  = 0.6f;
    static constexpr float HEIGHT = 1.8f;
    static constexpr float EYE_HEIGHT = 1.62f;

    static constexpr float WALK_SPEED   = 4.3f;
    static constexpr float SPRINT_SPEED = 5.6f;

private:
    Camera    m_camera;
    glm::vec3 m_pos{0, 200, 0};
    glm::vec3 m_vel{0, 0, 0};
    bool      m_onGround = false;
};
