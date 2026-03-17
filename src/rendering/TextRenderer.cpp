#include "rendering/TextRenderer.h"
#include <stb_image.h>
#include <cstdio>

TextRenderer::TextRenderer() {}

TextRenderer::~TextRenderer() {
    if (m_fontTex) glDeleteTextures(1, &m_fontTex);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void TextRenderer::init(const std::string& fontAtlasPath,
                         const std::string& vertPath,
                         const std::string& fragPath) {
    m_shader = Shader(vertPath, fragPath);

    // Load font atlas
    int w, h, ch;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(fontAtlasPath.c_str(), &w, &h, &ch, 4);
    if (data) {
        glGenTextures(1, &m_fontTex);
        glBindTexture(GL_TEXTURE_2D, m_fontTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        stbi_image_free(data);
    }

    std::fprintf(stderr, "[TextRenderer] init: fontTex=%u, shaderProg=%u\n",
                 m_fontTex, m_shader.id());

    // VAO/VBO for dynamic batching
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex),
                          (void*)offsetof(HudVertex, pos));
    // uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex),
                          (void*)offsetof(HudVertex, uv));
    // color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(HudVertex),
                          (void*)offsetof(HudVertex, color));
    glBindVertexArray(0);
}

glm::vec2 TextRenderer::pixelToNDC(float px, float py, int sw, int sh) const {
    float nx = (px / static_cast<float>(sw)) * 2.0f - 1.0f;
    float ny = 1.0f - (py / static_cast<float>(sh)) * 2.0f;  // flip Y: top=+1
    return {nx, ny};
}

void TextRenderer::beginBatch() {
    m_batch.clear();
    m_currentUseTexture = 0;
    m_currentTex = 0;
}

void TextRenderer::flushBatch() {
    if (m_batch.empty()) return;

    m_shader.use();
    m_shader.setInt("uUseTexture", m_currentUseTexture);

    if (m_currentUseTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_currentTex);
        m_shader.setInt("uTexture", 0);
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_batch.size() * sizeof(HudVertex)),
                 m_batch.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_batch.size()));
    glBindVertexArray(0);

    m_batch.clear();
}

void TextRenderer::flush() {
    flushBatch();
}

void TextRenderer::drawText(const std::string& text, float x, float y, float scale,
                             const glm::vec4& color, int screenW, int screenH) {
    // Switch to font texture mode
    if (m_currentUseTexture != 1 || m_currentTex != m_fontTex) {
        flushBatch();
        m_currentUseTexture = 1;
        m_currentTex = m_fontTex;
    }

    float charW = 8.0f * scale;  // Monocraft is ~8px wide per char at size 14
    float charH = 16.0f * scale;

    float atlasW = static_cast<float>(COLS * CELL_W);  // 256
    float atlasH = static_cast<float>(ROWS * CELL_H);  // 96

    float cx = x;
    for (char c : text) {
        int idx = static_cast<int>(c) - START_CHAR;
        if (idx < 0 || idx >= COLS * ROWS) { cx += charW; continue; }

        int col = idx % COLS;
        int row = idx / COLS;

        float u0 = (col * CELL_W) / atlasW;
        float v0 = (row * CELL_H) / atlasH;
        float u1 = ((col + 1) * CELL_W) / atlasW;
        float v1 = ((row + 1) * CELL_H) / atlasH;

        glm::vec2 tl = pixelToNDC(cx, y, screenW, screenH);
        glm::vec2 br = pixelToNDC(cx + charW, y + charH, screenW, screenH);

        // Two triangles
        m_batch.push_back({tl,                        {u0, v0}, color});
        m_batch.push_back({{br.x, tl.y},              {u1, v0}, color});
        m_batch.push_back({{tl.x, br.y},              {u0, v1}, color});
        m_batch.push_back({{br.x, tl.y},              {u1, v0}, color});
        m_batch.push_back({br,                        {u1, v1}, color});
        m_batch.push_back({{tl.x, br.y},              {u0, v1}, color});

        cx += charW;
    }
}

void TextRenderer::drawRect(float x, float y, float w, float h,
                             const glm::vec4& color, int screenW, int screenH) {
    // Switch to solid color mode
    if (m_currentUseTexture != 0) {
        flushBatch();
        m_currentUseTexture = 0;
        m_currentTex = 0;
    }

    glm::vec2 tl = pixelToNDC(x, y, screenW, screenH);
    glm::vec2 br = pixelToNDC(x + w, y + h, screenW, screenH);

    m_batch.push_back({tl,            {0,0}, color});
    m_batch.push_back({{br.x, tl.y},  {0,0}, color});
    m_batch.push_back({{tl.x, br.y},  {0,0}, color});
    m_batch.push_back({{br.x, tl.y},  {0,0}, color});
    m_batch.push_back({br,            {0,0}, color});
    m_batch.push_back({{tl.x, br.y},  {0,0}, color});
}

void TextRenderer::drawTexturedRect(float x, float y, float w, float h,
                                     float u0, float v0, float u1, float v1,
                                     GLuint texId, int screenW, int screenH) {
    // Switch to full-color texture mode (mode 2) with given texture
    if (m_currentUseTexture != 2 || m_currentTex != texId) {
        flushBatch();
        m_currentUseTexture = 2;
        m_currentTex = texId;
    }

    glm::vec2 tl = pixelToNDC(x, y, screenW, screenH);
    glm::vec2 br = pixelToNDC(x + w, y + h, screenW, screenH);
    glm::vec4 col(1.0f);

    m_batch.push_back({tl,            {u0, v0}, col});
    m_batch.push_back({{br.x, tl.y},  {u1, v0}, col});
    m_batch.push_back({{tl.x, br.y},  {u0, v1}, col});
    m_batch.push_back({{br.x, tl.y},  {u1, v0}, col});
    m_batch.push_back({br,            {u1, v1}, col});
    m_batch.push_back({{tl.x, br.y},  {u0, v1}, col});
}
