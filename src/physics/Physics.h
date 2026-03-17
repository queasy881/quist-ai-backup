#pragma once
#include "physics/AABB.h"
#include <glm/glm.hpp>

class World;

class Physics {
public:
    static constexpr float GRAVITY      = -28.0f;
    static constexpr float TERMINAL_VEL = -50.0f;
    static constexpr float JUMP_VEL     = 9.0f;

    static glm::vec3 moveAndSlide(const World& world,
                                  const AABB&  box,
                                  glm::vec3&   velocity,
                                  float        dt,
                                  bool&        onGround);

private:
    static bool collidesWithWorld(const World& world, const AABB& box);
};
