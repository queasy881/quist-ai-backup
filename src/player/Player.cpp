#include "player/Player.h"
#include "core/Input.h"
#include "physics/Physics.h"
#include "world/World.h"
#include <GLFW/glfw3.h>
#include <cmath>

Player::Player() {}

void Player::update(World& world, float dt) {
    // ── Mouse look ──
    if (Input::isCursorLocked()) {
        glm::vec2 md = Input::getMouseDelta();
        m_camera.processMouseMovement(md.x, md.y);
    }

    // ── Movement input ──
    glm::vec3 front  = m_camera.getFront();
    glm::vec3 right  = m_camera.getRight();
    front.y = 0; front = glm::normalize(front);
    right.y = 0; right = glm::normalize(right);

    bool sprinting = Input::isKeyDown(GLFW_KEY_LEFT_SHIFT);
    float speed = sprinting ? SPRINT_SPEED : WALK_SPEED;

    glm::vec3 wish(0);
    if (Input::isKeyDown(GLFW_KEY_W)) wish += front;
    if (Input::isKeyDown(GLFW_KEY_S)) wish -= front;
    if (Input::isKeyDown(GLFW_KEY_D)) wish += right;
    if (Input::isKeyDown(GLFW_KEY_A)) wish -= right;

    if (glm::length(wish) > 0.001f)
        wish = glm::normalize(wish) * speed;

    m_vel.x = wish.x;
    m_vel.z = wish.z;

    // ── Jump ──
    if (m_onGround && Input::isKeyDown(GLFW_KEY_SPACE))
        m_vel.y = Physics::JUMP_VEL;

    // ── Physics ──
    AABB box = getAABB();
    m_pos = Physics::moveAndSlide(world, box, m_vel, dt, m_onGround);

    // ── Update camera ──
    m_camera.setPosition(m_pos + glm::vec3(0, EYE_HEIGHT - HEIGHT * 0.5f, 0));
}

AABB Player::getAABB() const {
    return AABB::fromCenterSize(m_pos, glm::vec3(WIDTH, HEIGHT, WIDTH));
}
