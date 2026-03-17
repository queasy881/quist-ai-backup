#include "core/Game.h"
#include "world/BlockRegistry.h"
#include <GLFW/glfw3.h>
#include <cstdio>

Game::Game() : m_hotbar(m_inventory) {}
Game::~Game() = default;

void Game::run() {
    init();

    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        Time::update();
        processInput();
        update();
        render();
        m_window->swapBuffers();
        Input::update();  // reset deltas after frame is done
    }
}

void Game::init() {
    BlockRegistry::init();

    m_window   = std::make_unique<Window>(1280, 720, "VOXELREAMS");
    Input::init(m_window->getHandle());
    Input::setCursorLocked(true);
    Time::init();

    m_renderer = std::make_unique<Renderer>();
    m_renderer->init(m_window->getWidth(), m_window->getHeight());

    m_world  = std::make_unique<World>(42);
    m_player = std::make_unique<Player>();

    // Spawn player on top of terrain at (0, 0)
    int surfaceY = m_world->getSurfaceHeight(0, 0);
    m_player->setPosition(glm::vec3(0.0f, static_cast<float>(surfaceY + 2), 0.0f));
}

void Game::processInput() {
    // Escape → toggle cursor
    if (Input::isKeyPressed(GLFW_KEY_ESCAPE)) {
        if (m_inventoryOpen) {
            m_inventoryOpen = false;
            Input::setCursorLocked(true);
        } else {
            bool locked = Input::isCursorLocked();
            Input::setCursorLocked(!locked);
        }
    }

    // E → inventory
    if (Input::isKeyPressed(GLFW_KEY_E)) {
        m_inventoryOpen = !m_inventoryOpen;
        Input::setCursorLocked(!m_inventoryOpen);
    }

    // F4 → chunk borders
    if (Input::isKeyPressed(GLFW_KEY_F4))
        m_showChunkBorders = !m_showChunkBorders;

    // Hotbar scroll
    float scroll = Input::getScrollDelta();
    if (scroll != 0.0f) m_hotbar.scroll(scroll);

    // Number keys 1-9 → hotbar selection
    for (int k = GLFW_KEY_1; k <= GLFW_KEY_9; ++k)
        if (Input::isKeyPressed(k)) m_hotbar.select(k - GLFW_KEY_1);
}

void Game::update() {
    float dt = Time::deltaTime();
    if (dt > 0.1f) dt = 0.1f; // cap huge dt after stalls

    m_player->update(*m_world, dt);
    m_world->update(m_player->getPosition());

    // Raycast from camera
    const Camera& cam = m_player->getCamera();
    m_lookHit = Raycast::cast(*m_world, cam.getPosition(), cam.getFront(), 6.0f);

    // Mining
    bool mining = Input::isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && Input::isCursorLocked();
    m_mining.update(m_lookHit, mining, dt, *m_world, m_drops);

    // Dropped block items
    m_drops.update(dt, m_player->getPosition(), m_inventory);

    // Placement
    bool place = Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT) && Input::isCursorLocked();
    m_placement.update(m_lookHit, place, *m_world, *m_player, m_hotbar);
}

void Game::render() {
    float totalTime = Time::totalTime();
    int w = m_window->getWidth();
    int h = m_window->getHeight();
    // Update viewport size from GLFW (handles resize)
    glfwGetFramebufferSize(m_window->getHandle(), &w, &h);
    if (w <= 0 || h <= 0) return;
    glViewport(0, 0, w, h);

    // Resize deferred pipeline if needed
    m_renderer->resize(w, h);

    float aspect = static_cast<float>(w) / static_cast<float>(h);
    const Camera& cam = m_player->getCamera();

    m_renderer->beginFrame(totalTime);
    m_renderer->renderWorld(*m_world, cam, aspect, totalTime);
    m_renderer->renderBlockOutline(cam, aspect, m_lookHit);

    // Mining crack overlay
    if (m_mining.isActive()) {
        m_renderer->renderMiningCrack(cam, aspect, m_mining.targetBlock(), m_mining.progress());
    }

    if (m_showChunkBorders)
        m_renderer->renderChunkBorders(cam, aspect, m_player->getPosition());

    // Render dropped block items
    m_renderer->renderDroppedBlocks(cam, aspect, totalTime, m_drops.drops());

    m_renderer->renderHUD(w, h, m_hotbar, m_mining);
    m_renderer->renderCrosshair(w, h);
    m_renderer->endFrame();

    // FPS counter in title bar
    static int lastFps = 0;
    int fps = Time::fps();
    if (fps != lastFps) {
        char title[128];
        std::snprintf(title, sizeof(title),
            "VOXELREAMS | FPS: %d | Pos: %.1f %.1f %.1f | Hotbar: %d",
            fps, m_player->getPosition().x, m_player->getPosition().y,
            m_player->getPosition().z, m_hotbar.selected() + 1);
        glfwSetWindowTitle(m_window->getHandle(), title);
        lastFps = fps;
    }
}
