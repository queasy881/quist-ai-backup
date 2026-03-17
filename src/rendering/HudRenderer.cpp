#include "rendering/HudRenderer.h"
#include "world/BlockRegistry.h"
#include <cstring>
#include <string>
#include <cstdio>

void HudRenderer::init(TextureAtlas& atlas) {
    m_text.init("assets/fonts/font_atlas.png", "shaders/hud.vert", "shaders/hud.frag");
    m_atlasTex     = atlas.id();
    m_atlasGridSize = atlas.tilesPerRow();
    m_atlasCellSize = 16; // tile pixel size
    std::fprintf(stderr, "[HUD] init: atlasTex=%u, grid=%d, shaderProg=%u, fontTex=%u\n",
                 m_atlasTex, m_atlasGridSize,
                 m_text.shader().id(), 0u /* font tex is private */);
}

void HudRenderer::render(int screenW, int screenH,
                          const Hotbar& hotbar,
                          const MiningSystem& mining) {
    static bool once = false;
    if (!once) {
        std::fprintf(stderr, "[HUD] render called: %dx%d, shaderProg=%u\n",
                     screenW, screenH, m_text.shader().id());
        once = true;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_text.beginBatch();
    drawHotbar(screenW, screenH, hotbar);
    drawMiningBar(screenW, screenH, mining);
    m_text.flush();

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void HudRenderer::drawHotbar(int screenW, int screenH, const Hotbar& hotbar) {
    // Hotbar dimensions
    constexpr float SLOT_SIZE  = 64.0f;
    constexpr float SLOT_PAD   = 6.0f;
    constexpr float BORDER     = 4.0f;
    constexpr int   SLOTS      = 9;

    float totalW = SLOTS * SLOT_SIZE + (SLOTS - 1) * SLOT_PAD;
    float startX = (screenW - totalW) * 0.5f;
    float startY = screenH - SLOT_SIZE - 16.0f;

    // Background bar
    m_text.drawRect(startX - BORDER - 2, startY - BORDER - 2,
                    totalW + (BORDER + 2) * 2, SLOT_SIZE + (BORDER + 2) * 2,
                    glm::vec4(0.0f, 0.0f, 0.0f, 0.55f), screenW, screenH);

    for (int i = 0; i < SLOTS; ++i) {
        float x = startX + i * (SLOT_SIZE + SLOT_PAD);
        float y = startY;

        // Slot background
        bool selected = (i == hotbar.selected());
        glm::vec4 slotColor = selected
            ? glm::vec4(0.8f, 0.8f, 0.8f, 0.7f)
            : glm::vec4(0.15f, 0.15f, 0.15f, 0.6f);
        m_text.drawRect(x, y, SLOT_SIZE, SLOT_SIZE, slotColor, screenW, screenH);

        // Selection border
        if (selected) {
            m_text.drawRect(x - BORDER, y - BORDER,
                            SLOT_SIZE + BORDER * 2, BORDER,
                            glm::vec4(1.0f), screenW, screenH);
            m_text.drawRect(x - BORDER, y + SLOT_SIZE,
                            SLOT_SIZE + BORDER * 2, BORDER,
                            glm::vec4(1.0f), screenW, screenH);
            m_text.drawRect(x - BORDER, y,
                            BORDER, SLOT_SIZE,
                            glm::vec4(1.0f), screenW, screenH);
            m_text.drawRect(x + SLOT_SIZE, y,
                            BORDER, SLOT_SIZE,
                            glm::vec4(1.0f), screenW, screenH);
        }

        // Item icon (block face from atlas)
        const ItemStack& item = hotbar.inventory().slot(i);
        if (!item.empty()) {
            const Block& block = BlockRegistry::get(item.blockId);
            int texIdx = block.texTop; // show top face as icon

            // Compute UV in atlas
            int col = texIdx % m_atlasGridSize;
            int row = texIdx / m_atlasGridSize;
            float uvStep = 1.0f / static_cast<float>(m_atlasGridSize);
            float u0 = col * uvStep;
            float v0 = row * uvStep;
            float u1 = u0 + uvStep;
            float v1 = v0 + uvStep;

            float iconPad = 8.0f;
            m_text.flush(); // flush solid rects before switching to textured
            m_text.beginBatch();
            m_text.drawTexturedRect(x + iconPad, y + iconPad,
                                    SLOT_SIZE - iconPad * 2, SLOT_SIZE - iconPad * 2,
                                    u0, v0, u1, v1,
                                    m_atlasTex, screenW, screenH);
            m_text.flush();
            m_text.beginBatch();

            // Item count
            if (item.count > 1) {
                char buf[8];
                std::snprintf(buf, sizeof(buf), "%d", item.count);
                float textX = x + SLOT_SIZE - 8.0f * static_cast<float>(strlen(buf)) * 2.0f - 3.0f;
                float textY = y + SLOT_SIZE - 16.0f * 2.0f - 2.0f;
                // Shadow
                m_text.drawText(buf, textX + 1, textY + 1, 2.0f,
                                glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), screenW, screenH);
                // Text
                m_text.drawText(buf, textX, textY, 2.0f,
                                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), screenW, screenH);
                m_text.flush();
                m_text.beginBatch();
            }
        }

        // Slot number (small, top-left)
        char numBuf[4];
        std::snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
        m_text.drawText(numBuf, x + 3, y + 2, 1.2f,
                        glm::vec4(0.7f, 0.7f, 0.7f, 0.6f), screenW, screenH);
        m_text.flush();
        m_text.beginBatch();
    }
}

void HudRenderer::drawMiningBar(int screenW, int screenH, const MiningSystem& mining) {
    if (!mining.isActive()) return;

    float progress = mining.progress();
    float barW = 160.0f;
    float barH = 8.0f;
    float x = (screenW - barW) * 0.5f;
    float y = screenH * 0.5f + 20.0f;

    // Background
    m_text.drawRect(x - 1, y - 1, barW + 2, barH + 2,
                    glm::vec4(0.0f, 0.0f, 0.0f, 0.7f), screenW, screenH);

    // Filled portion
    m_text.drawRect(x, y, barW * progress, barH,
                    glm::vec4(0.3f, 0.9f, 0.3f, 0.9f), screenW, screenH);
}
