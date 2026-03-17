#include "gameplay/PlacementSystem.h"
#include "world/World.h"
#include "world/BlockRegistry.h"
#include "player/Player.h"
#include "gameplay/Hotbar.h"

void PlacementSystem::update(const RaycastHit& hit, bool placePressed,
                              World& world, Player& player, Hotbar& hotbar)
{
    if (!placePressed || !hit.hit) return;

    const ItemStack& item = hotbar.selectedItem();
    if (item.empty()) return;

    glm::ivec3 pos = hit.prevPos;

    // Prevent placement inside player
    AABB blockBox(glm::vec3(pos), glm::vec3(pos) + glm::vec3(1));
    if (player.getAABB().intersects(blockBox)) return;

    // Prevent placing in solid
    uint8_t existing = world.getBlock(pos.x, pos.y, pos.z);
    if (BlockRegistry::get(existing).solid) return;

    world.setBlock(pos.x, pos.y, pos.z, item.blockId);
    hotbar.inventory().removeItem(item.blockId, 1);
}
