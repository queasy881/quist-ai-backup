#pragma once
#include "rendering/Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct HudVertex {
    glm::vec2 pos;   // NDC
    glm::vec2 uv;
    glm::vec4 color;
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    void init(const std::string& fontAtlasPath, const std::string& vertPath, const std::string& fragPath);

    // screenW/H = window pixel size; x,y in pixel coords from top-left; scale=pixel multiplier
    void drawText(const std::string& text, float x, float y, float scale,
                  const glm::vec4& color, int screenW, int screenH);

    // Draw a solid-color quad (no texture). Coords in pixels from top-left.
    void drawRect(float x, float y, float w, float h,
                  const glm::vec4& color, int screenW, int screenH);

    // Draw a textured quad from the block atlas. Coords in pixels.
    void drawTexturedRect(float x, float y, float w, float h,
                          float u0, float v0, float u1, float v1,
                          GLuint texId, int screenW, int screenH);

    // Call at start/end of HUD rendering
    void beginBatch();
    void flush();

    Shader& shader() { return m_shader; }

private:
    static constexpr int CELL_W = 16;  // font atlas cell width
    static constexpr int CELL_H = 16;  // font atlas cell height
    static constexpr int COLS   = 16;  // chars per row in atlas
    static constexpr int ROWS   = 6;   // rows in atlas
    static constexpr int START_CHAR = 32;

    Shader m_shader;
    GLuint m_fontTex = 0;
    GLuint m_vao = 0, m_vbo = 0;

    std::vector<HudVertex> m_batch;
    int  m_currentUseTexture = 0;
    GLuint m_currentTex = 0;

    glm::vec2 pixelToNDC(float px, float py, int sw, int sh) const;
    void flushBatch();
};
