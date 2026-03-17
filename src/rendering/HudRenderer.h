#pragma once
#include "rendering/TextRenderer.h"
#include "rendering/TextureAtlas.h"
#include "gameplay/Hotbar.h"
#include "gameplay/MiningSystem.h"

class HudRenderer {
public:
    void init(TextureAtlas& atlas);
    void render(int screenW, int screenH,
                const Hotbar& hotbar,
                const MiningSystem& mining);

    TextRenderer& textRenderer() { return m_text; }

private:
    TextRenderer m_text;
    GLuint       m_atlasTex = 0;
    int          m_atlasGridSize = 0;
    int          m_atlasCellSize = 0;

    void drawHotbar(int screenW, int screenH, const Hotbar& hotbar);
    void drawMiningBar(int screenW, int screenH, const MiningSystem& mining);
};
