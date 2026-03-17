#pragma once

#include "core/Window.h"
#include "core/Input.h"
#include "core/Time.h"

#include "rendering/Renderer.h"
#include "world/World.h"
#include "player/Player.h"
#include "physics/Raycast.h"

#include "gameplay/Inventory.h"
#include "gameplay/Hotbar.h"
#include "gameplay/MiningSystem.h"
#include "gameplay/PlacementSystem.h"
#include "gameplay/DroppedBlock.h"

#include <memory>

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void init();
    void processInput();
    void update();
    void render();

    std::unique_ptr<Window>   m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<World>    m_world;
    std::unique_ptr<Player>   m_player;

    Inventory           m_inventory;
    Hotbar              m_hotbar;
    MiningSystem        m_mining;
    PlacementSystem     m_placement;
    DroppedBlockManager m_drops;

    RaycastHit m_lookHit;
    bool       m_showChunkBorders = false;
    bool       m_inventoryOpen    = false;
};
