#pragma once
#include "physics/Raycast.h"
#include "physics/AABB.h"

class World;
class Player;
class Hotbar;

class PlacementSystem {
public:
    void update(const RaycastHit& hit, bool placePressed,
                World& world, Player& player, Hotbar& hotbar);
};
