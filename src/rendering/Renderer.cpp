#include "rendering/Renderer.h"
#include "rendering/FullscreenQuad.h"
#include "world/BlockRegistry.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <cstring>

Renderer::Renderer() {}
Renderer::~Renderer() {
    if (m_outlineVao) glDeleteVertexArrays(1, &m_outlineVao);
    if (m_outlineVbo) glDeleteBuffers(1, &m_outlineVbo);
    if (m_crossVao)   glDeleteVertexArrays(1, &m_crossVao);
    if (m_crossVbo)   glDeleteBuffers(1, &m_crossVbo);
    if (m_crackVao)   glDeleteVertexArrays(1, &m_crackVao);
    if (m_crackVbo)   glDeleteBuffers(1, &m_crackVbo);
    if (m_dropVao)    glDeleteVertexArrays(1, &m_dropVao);
    if (m_dropVbo)    glDeleteBuffers(1, &m_dropVbo);
    if (m_dropEbo)    glDeleteBuffers(1, &m_dropEbo);
    FullscreenQuad::shutdown();
}

void Renderer::init(int screenW, int screenH) {
    m_screenW = screenW;
    m_screenH = screenH;

    // Texture atlas
    std::vector<std::string> texFiles = {
        "assets/textures/grass_top.png",      // 0
        "assets/textures/grass_side.png",     // 1
        "assets/textures/dirt.png",           // 2
        "assets/textures/stone.png",          // 3
        "assets/textures/sand.png",           // 4
        "assets/textures/water.png",          // 5
        "assets/textures/wood.png",           // 6
        "assets/textures/leaves.png",         // 7
        "assets/textures/snow.png",           // 8
        "assets/textures/deepstone.png",      // 9
        "assets/textures/gravel.png",         // 10
        "assets/textures/wood_top.png",        // 11
    };
    m_atlas.build(texFiles);

    // Shaders
    m_chunkRenderer.init("shaders/chunk.vert", "shaders/chunk.frag");
    m_skyRenderer.init("shaders/sky.vert", "shaders/sky.frag");
    m_outlineShader = Shader("shaders/outline.vert", "shaders/outline.frag");
    m_crackShader   = Shader("shaders/crack.vert",   "shaders/crack.frag");
    m_uiShader      = Shader("shaders/ui.vert", "shaders/ui.frag");

    initOutline();
    initCrosshair();
    initCrackQuads();
    initDropCube();

    m_hudRenderer.init(m_atlas);

    // Initialize the deferred rendering pipeline
    m_deferred.init(m_screenW, m_screenH);
}

void Renderer::resize(int w, int h) {
    if (w == m_screenW && h == m_screenH) return;
    m_screenW = w;
    m_screenH = h;
    m_deferred.resize(w, h);
}

// ── Block outline (wireframe cube) ───────────────────────────

void Renderer::initOutline() {
    // 12 edges of a unit cube
    float v[] = {
        0,0,0, 1,0,0,  1,0,0, 1,0,1,  1,0,1, 0,0,1,  0,0,1, 0,0,0,
        0,1,0, 1,1,0,  1,1,0, 1,1,1,  1,1,1, 0,1,1,  0,1,1, 0,1,0,
        0,0,0, 0,1,0,  1,0,0, 1,1,0,  1,0,1, 1,1,1,  0,0,1, 0,1,1,
    };
    glGenVertexArrays(1, &m_outlineVao);
    glGenBuffers(1, &m_outlineVbo);
    glBindVertexArray(m_outlineVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_outlineVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), nullptr);
    glBindVertexArray(0);
}

void Renderer::renderBlockOutline(const Camera& cam, float aspect, const RaycastHit& hit) {
    if (!hit.hit) return;
    m_outlineShader.use();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(hit.blockPos));
    // Slightly expand to avoid z-fighting
    model = glm::translate(model, glm::vec3(-0.002f));
    model = glm::scale(model, glm::vec3(1.004f));

    m_outlineShader.setMat4("uMVP",
        cam.getProjectionMatrix(aspect) * cam.getViewMatrix() * model);
    m_outlineShader.setVec3("uColor", glm::vec3(0.0f, 0.0f, 0.0f));

    glLineWidth(2.0f);
    glBindVertexArray(m_outlineVao);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

// ── Mining crack overlay (6 face quads darkened) ─────────────

void Renderer::initCrackQuads() {
    // 6 faces of a unit cube, each as 2 triangles (36 verts)
    // Each vertex: pos(3) + uv(2) = 5 floats
    float v[] = {
        // -X face
        0,0,0, 0,0,  0,1,0, 0,1,  0,1,1, 1,1,  0,0,0, 0,0,  0,1,1, 1,1,  0,0,1, 1,0,
        // +X face
        1,0,1, 0,0,  1,1,1, 0,1,  1,1,0, 1,1,  1,0,1, 0,0,  1,1,0, 1,1,  1,0,0, 1,0,
        // -Y face
        0,0,0, 0,0,  0,0,1, 0,1,  1,0,1, 1,1,  0,0,0, 0,0,  1,0,1, 1,1,  1,0,0, 1,0,
        // +Y face
        0,1,0, 0,0,  1,1,0, 1,0,  1,1,1, 1,1,  0,1,0, 0,0,  1,1,1, 1,1,  0,1,1, 0,1,
        // -Z face
        1,0,0, 0,0,  1,1,0, 0,1,  0,1,0, 1,1,  1,0,0, 0,0,  0,1,0, 1,1,  0,0,0, 1,0,
        // +Z face
        0,0,1, 0,0,  0,1,1, 0,1,  1,1,1, 1,1,  0,0,1, 0,0,  1,1,1, 1,1,  1,0,1, 1,0,
    };
    glGenVertexArrays(1, &m_crackVao);
    glGenBuffers(1, &m_crackVbo);
    glBindVertexArray(m_crackVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_crackVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
    // uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
                          (void*)(3*sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::renderMiningCrack(const Camera& cam, float aspect,
                                  const glm::ivec3& blockPos, float progress) {
    if (progress <= 0.0f) return;

    // Compute crack stage (0-9)
    int stage = static_cast<int>(progress * 10.0f);
    if (stage > 9) stage = 9;

    m_crackShader.use();

    // Place the crack overlay exactly on the block, slightly expanded to avoid z-fighting
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(blockPos));
    model = glm::translate(model, glm::vec3(-0.002f));
    model = glm::scale(model, glm::vec3(1.004f));

    glm::mat4 mvp = cam.getProjectionMatrix(aspect) * cam.getViewMatrix() * model;
    m_crackShader.setMat4("uMVP", mvp);
    m_crackShader.setInt("uStage", stage);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -2.0f);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(m_crackVao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ── Crosshair ────────────────────────────────────────────────

void Renderer::initCrosshair() {
    float s = 0.015f;
    float v[] = {
        -s,  0.0f,
         s,  0.0f,
         0.0f, -s,
         0.0f,  s,
    };
    glGenVertexArrays(1, &m_crossVao);
    glGenBuffers(1, &m_crossVbo);
    glBindVertexArray(m_crossVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_crossVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), nullptr);
    glBindVertexArray(0);
}

void Renderer::renderCrosshair(int winW, int winH) {
    m_uiShader.use();
    float aspect = static_cast<float>(winW) / static_cast<float>(winH);
    m_uiShader.setFloat("uAspect", aspect);
    m_uiShader.setVec3("uColor", glm::vec3(1.0f));

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); // invert color
    glLineWidth(2.0f);
    glBindVertexArray(m_crossVao);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
}

// ── Chunk border debug ───────────────────────────────────────

void Renderer::renderChunkBorders(const Camera& cam, float aspect, const glm::vec3& ppos) {
    int cx = static_cast<int>(std::floor(ppos.x)) & ~15;
    int cz = static_cast<int>(std::floor(ppos.z)) & ~15;

    m_outlineShader.use();

    float lines[24 * 3];
    int idx = 0;
    auto addLine = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
        lines[idx++] = x0; lines[idx++] = y0; lines[idx++] = z0;
        lines[idx++] = x1; lines[idx++] = y1; lines[idx++] = z1;
    };

    float y0 = 0, y1 = 256;
    // 4 vertical edges of current chunk column
    addLine((float)cx,    y0, (float)cz,    (float)cx,    y1, (float)cz);
    addLine((float)cx+16, y0, (float)cz,    (float)cx+16, y1, (float)cz);
    addLine((float)cx,    y0, (float)cz+16, (float)cx,    y1, (float)cz+16);
    addLine((float)cx+16, y0, (float)cz+16, (float)cx+16, y1, (float)cz+16);

    GLuint tmpVao, tmpVbo;
    glGenVertexArrays(1, &tmpVao);
    glGenBuffers(1, &tmpVbo);
    glBindVertexArray(tmpVao);
    glBindBuffer(GL_ARRAY_BUFFER, tmpVbo);
    glBufferData(GL_ARRAY_BUFFER, idx * sizeof(float), lines, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), nullptr);

    glm::mat4 mvp = cam.getProjectionMatrix(aspect) * cam.getViewMatrix();
    m_outlineShader.setMat4("uMVP", mvp);
    m_outlineShader.setVec3("uColor", glm::vec3(1.0f, 1.0f, 0.0f));

    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, idx / 3);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &tmpVao);
    glDeleteBuffers(1, &tmpVbo);
}

// ── Dropped block items (small spinning 3D cubes) ────────────

// Template unit cube: 24 verts (4 per face), layout matches ChunkVertex
// pos(3) uv(2) normal(3) ao(1) texIndex(1) = 10 floats per vertex
// All faces use CCW winding for correct front-face culling
static const float s_dropCubeTemplate[24 * 10] = {
    // +Y (top) — verts 0-3, CCW when viewed from above
    0,1,0, 0,0,  0,1,0, 1.0f, 0,
    0,1,1, 0,1,  0,1,0, 1.0f, 0,
    1,1,1, 1,1,  0,1,0, 1.0f, 0,
    1,1,0, 1,0,  0,1,0, 1.0f, 0,
    // -Y (bottom) — verts 4-7, CCW when viewed from below
    0,0,1, 0,0,  0,-1,0, 1.0f, 0,
    0,0,0, 0,1,  0,-1,0, 1.0f, 0,
    1,0,0, 1,1,  0,-1,0, 1.0f, 0,
    1,0,1, 1,0,  0,-1,0, 1.0f, 0,
    // +X — verts 8-11
    1,0,1, 0,0,  1,0,0, 1.0f, 0,
    1,0,0, 1,0,  1,0,0, 1.0f, 0,
    1,1,0, 1,1,  1,0,0, 1.0f, 0,
    1,1,1, 0,1,  1,0,0, 1.0f, 0,
    // -X — verts 12-15
    0,0,0, 0,0, -1,0,0, 1.0f, 0,
    0,0,1, 1,0, -1,0,0, 1.0f, 0,
    0,1,1, 1,1, -1,0,0, 1.0f, 0,
    0,1,0, 0,1, -1,0,0, 1.0f, 0,
    // +Z — verts 16-19
    0,0,1, 0,0,  0,0,1, 1.0f, 0,
    1,0,1, 1,0,  0,0,1, 1.0f, 0,
    1,1,1, 1,1,  0,0,1, 1.0f, 0,
    0,1,1, 0,1,  0,0,1, 1.0f, 0,
    // -Z — verts 20-23
    1,0,0, 0,0,  0,0,-1, 1.0f, 0,
    0,0,0, 1,0,  0,0,-1, 1.0f, 0,
    0,1,0, 1,1,  0,0,-1, 1.0f, 0,
    1,1,0, 0,1,  0,0,-1, 1.0f, 0,
};

static const uint32_t s_dropCubeIndices[36] = {
    0,1,2,  0,2,3,     // +Y
    4,5,6,  4,6,7,     // -Y
    8,9,10, 8,10,11,   // +X
    12,13,14, 12,14,15, // -X
    16,17,18, 16,18,19, // +Z
    20,21,22, 20,22,23, // -Z
};

void Renderer::initDropCube() {
    glGenVertexArrays(1, &m_dropVao);
    glGenBuffers(1, &m_dropVbo);
    glGenBuffers(1, &m_dropEbo);
    glBindVertexArray(m_dropVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_dropVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_dropCubeTemplate), s_dropCubeTemplate, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_dropEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(s_dropCubeIndices), s_dropCubeIndices, GL_STATIC_DRAW);

    size_t stride = 10 * sizeof(float);
    size_t off = 0;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)off); off += 3*sizeof(float);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)off); off += 2*sizeof(float);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)off); off += 3*sizeof(float);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)off); off += sizeof(float);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)off);

    glBindVertexArray(0);
}

void Renderer::renderDroppedBlocks(const Camera& cam, float aspect, float totalTime,
                                    const std::vector<DroppedBlock>& drops) {
    if (drops.empty()) return;

    auto& shader = m_chunkRenderer.shader();
    shader.use();

    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    shader.setMat4("uView", view);
    shader.setMat4("uProj", proj);
    shader.setFloat("uAtlasSize", static_cast<float>(m_atlas.tilesPerRow()));
    shader.setFloat("uDaylight", m_skyRenderer.getDaylight(totalTime));
    shader.setVec3("uFogColor", m_skyRenderer.getFogColor(totalTime));
    shader.setFloat("uFogStart", static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE - 32));
    shader.setFloat("uFogEnd",   static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE));
    shader.setVec3("uCamPos", cam.getPosition());
    shader.setInt("uTexAtlas", 0);
    shader.setFloat("uIsWater", 0.0f);

    m_atlas.bind(0);
    glBindVertexArray(m_dropVao);

    // Temp buffer for patching texIndex per drop
    float verts[24 * 10];

    for (const auto& drop : drops) {
        if (drop.collected) continue;

        const Block& block = BlockRegistry::get(drop.blockId);

        // Build model matrix: translate → rotate → scale
        float bobOffset = std::sin(drop.bobPhase) * DroppedBlock::BOB_HEIGHT;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), drop.pos + glm::vec3(0, bobOffset, 0));
        model = glm::rotate(model, drop.rotation, glm::vec3(0, 1, 0));
        float s = DroppedBlock::SCALE;
        model = glm::translate(model, glm::vec3(-s * 0.5f, 0, -s * 0.5f));
        model = glm::scale(model, glm::vec3(s));

        shader.setMat4("uModel", model);

        // Copy template and patch texIndex for each face
        std::memcpy(verts, s_dropCubeTemplate, sizeof(verts));
        float texTop    = static_cast<float>(block.texTop);
        float texBot    = static_cast<float>(block.texBottom);
        float texSide   = static_cast<float>(block.texSide);
        // Face order: top(0-3), bottom(4-7), +X(8-11), -X(12-15), +Z(16-19), -Z(20-23)
        float texPerFace[6] = { texTop, texBot, texSide, texSide, texSide, texSide };
        for (int face = 0; face < 6; ++face) {
            for (int v = 0; v < 4; ++v) {
                verts[(face * 4 + v) * 10 + 9] = texPerFace[face];
            }
        }

        // Upload full buffer and draw
        glBindBuffer(GL_ARRAY_BUFFER, m_dropVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}

// ── Frame helpers ────────────────────────────────────────────

void Renderer::beginFrame(float totalTime) {
    glm::vec3 sky = m_skyRenderer.getSkyColor(totalTime);
    glClearColor(sky.r, sky.g, sky.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderWorld(World& world, const Camera& cam, float aspect, float totalTime) {
    // Upload new meshes
    {
        auto& cols = world.getColumns();
        for (auto& [cc, col] : cols) {
            if (col.mesh && !col.meshUploaded) {
                m_chunkRenderer.uploadMesh(cc, *col.mesh);
                const_cast<ChunkColumn&>(col).meshUploaded = true;
            }
        }
    }

    // ── 1. Shadow pass ───────────────────────────────────
    renderShadowPass(world, cam, aspect, totalTime);

    // ── 2. G-Buffer pass ─────────────────────────────────
    renderGBufferPass(world, cam, aspect, totalTime);

    // ── 3-9. Screen-space passes + compositing ──────────
    renderDeferredLighting(cam, aspect, totalTime);

    // ── Water pass (forward, rendered into HDR before tonemap) ──
    // Water was already tone-mapped at this point, so we render into default FBO
    // with the original forward shader. This is acceptable for alpha-blended water.
    // Restore depth from G-Buffer for depth testing
    glViewport(0, 0, m_screenW, m_screenH);
    glEnable(GL_DEPTH_TEST);
    {
        float daylight = m_skyRenderer.getDaylight(totalTime);
        glm::vec3 fogColor = m_skyRenderer.getFogColor(totalTime);

        m_chunkRenderer.shader().use();
        glm::mat4 view = cam.getViewMatrix();
        glm::mat4 proj = cam.getProjectionMatrix(aspect);
        m_chunkRenderer.shader().setMat4("uView", view);
        m_chunkRenderer.shader().setMat4("uProj", proj);
        m_chunkRenderer.shader().setFloat("uAtlasSize", static_cast<float>(m_atlas.tilesPerRow()));
        m_chunkRenderer.shader().setFloat("uDaylight", daylight);
        m_chunkRenderer.shader().setVec3("uFogColor", fogColor);
        m_chunkRenderer.shader().setFloat("uFogStart", static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE - 32));
        m_chunkRenderer.shader().setFloat("uFogEnd",   static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE));
        m_chunkRenderer.shader().setVec3("uCamPos", cam.getPosition());
        m_chunkRenderer.shader().setInt("uTexAtlas", 0);
        m_atlas.bind(0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (auto& [cc, g] : m_chunkRenderer.gpuMeshes()) {
            if (g.wIndexCount == 0) continue;
            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                              glm::vec3(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE));
            m_chunkRenderer.shader().setMat4("uModel", model);
            m_chunkRenderer.shader().setFloat("uIsWater", 1.0f);
            glBindVertexArray(g.wVao);
            glDrawElements(GL_TRIANGLES, g.wIndexCount, GL_UNSIGNED_INT, nullptr);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }
}

// ── Shadow pass: render scene from sun into CSM ──────────────

void Renderer::renderShadowPass(World& /*world*/, const Camera& cam, float aspect, float totalTime) {
    glm::vec3 sunDir = m_skyRenderer.getSunDirection(totalTime);
    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);

    m_deferred.shadowMap().update(view, proj, sunDir, cam.getNear(), cam.getFar());
    m_deferred.beginShadowPass();

    auto& shadowShader = m_deferred.shadowShader();

    for (int c = 0; c < ShadowMap::NUM_CASCADES; ++c) {
        m_deferred.bindShadowCascade(c);

        // Render all opaque chunks into this cascade
        for (auto& [cc, g] : m_chunkRenderer.gpuMeshes()) {
            if (g.indexCount == 0) continue;

            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                              glm::vec3(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE));
            shadowShader.setMat4("uModel", model);

            glBindVertexArray(g.vao);
            glDrawElements(GL_TRIANGLES, g.indexCount, GL_UNSIGNED_INT, nullptr);
        }
    }

    m_deferred.endShadowPass();
}

// ── G-Buffer pass: render geometry with PBR materials ────────

void Renderer::renderGBufferPass(World& /*world*/, const Camera& cam, float aspect, float /*totalTime*/) {
    m_deferred.beginGBufferPass();

    auto& gbufShader = m_deferred.gbufferShader();
    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    gbufShader.setMat4("uView", view);
    gbufShader.setMat4("uProj", proj);
    gbufShader.setFloat("uAtlasSize", static_cast<float>(m_atlas.tilesPerRow()));
    gbufShader.setInt("uTexAtlas", 0);
    m_atlas.bind(0);

    Camera::Frustum frust = cam.getFrustum(aspect);

    // Render all opaque chunks into G-Buffer
    for (auto& [cc, g] : m_chunkRenderer.gpuMeshes()) {
        if (g.indexCount == 0) continue;

        // Frustum cull
        glm::vec3 cmin(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE);
        glm::vec3 cmax = cmin + glm::vec3(Chunk::SIZE, Chunk::HEIGHT, Chunk::SIZE);
        glm::vec3 center = (cmin + cmax) * 0.5f;
        float radius = glm::length(cmax - center);
        bool visible = true;
        for (int i = 0; i < 6; ++i) {
            float d = glm::dot(glm::vec3(frust.planes[i]), center) + frust.planes[i].w;
            if (d < -radius) { visible = false; break; }
        }
        if (!visible) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                          glm::vec3(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE));
        gbufShader.setMat4("uModel", model);

        glBindVertexArray(g.vao);
        glDrawElements(GL_TRIANGLES, g.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    m_deferred.endGBufferPass();
}

// ── Screen-space passes ──────────────────────────────────────

void Renderer::renderDeferredLighting(const Camera& cam, float aspect, float totalTime) {
    float daylight = m_skyRenderer.getDaylight(totalTime);
    glm::vec3 sunDir  = m_skyRenderer.getSunDirection(totalTime);
    glm::vec3 fogColor = m_skyRenderer.getFogColor(totalTime);
    float fogStart = static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE - 32);
    float fogEnd   = static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE);

    // 3. SSAO
    m_deferred.ssaoPass(cam, aspect);

    // 4. Deferred lighting (PBR + CSM + SSAO → HDR)
    m_deferred.lightingPass(cam, aspect, daylight, sunDir, fogColor, fogStart, fogEnd);

    // 5. SSR
    m_deferred.ssrPass(cam, aspect);

    // 6. Atmosphere (sky fill)
    m_deferred.atmospherePass(cam, aspect, totalTime, m_skyRenderer);

    // 7. Volumetric lighting
    m_deferred.volumetricPass(cam, aspect, sunDir, daylight);

    // 8. Bloom
    m_deferred.bloomPass();

    // 9. Tone mapping → default framebuffer
    m_deferred.tonemapPass(1.8f, totalTime);

    // Restore viewport and depth test for overlays
    glViewport(0, 0, m_screenW, m_screenH);
    glEnable(GL_DEPTH_TEST);

    // Copy G-Buffer depth to default FBO for overlays (outline, crack, etc.)
    glBlitNamedFramebuffer(
        m_deferred.gbuffer().fbo(),
        0, // default framebuffer
        0, 0, m_screenW, m_screenH,
        0, 0, m_screenW, m_screenH,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void Renderer::endFrame() {}

void Renderer::renderHUD(int winW, int winH, const Hotbar& hotbar, const MiningSystem& mining) {
    m_hudRenderer.render(winW, winH, hotbar, mining);
}
