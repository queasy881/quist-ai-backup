#pragma once
#include "rendering/ChunkRenderer.h"
#include "rendering/SkyRenderer.h"
#include "rendering/DeferredPipeline.h"
#include "rendering/Shader.h"
#include "rendering/TextureAtlas.h"
#include "rendering/HudRenderer.h"
#include "world/World.h"
#include "player/Camera.h"
#include "physics/Raycast.h"
#include "gameplay/DroppedBlock.h"

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init(int screenW, int screenH);
    void resize(int screenW, int screenH);

    void beginFrame(float totalTime);
    void renderWorld(World& world, const Camera& cam, float aspect, float totalTime);
    void renderBlockOutline(const Camera& cam, float aspect, const RaycastHit& hit);
    void renderMiningCrack(const Camera& cam, float aspect, const glm::ivec3& blockPos, float progress);
    void renderDroppedBlocks(const Camera& cam, float aspect, float totalTime,
                             const std::vector<DroppedBlock>& drops);
    void renderCrosshair(int winW, int winH);
    void renderChunkBorders(const Camera& cam, float aspect, const glm::vec3& playerPos);
    void renderHUD(int winW, int winH, const Hotbar& hotbar, const MiningSystem& mining);
    void endFrame();

    TextureAtlas&      atlas()          { return m_atlas; }
    ChunkRenderer&     chunkRenderer()  { return m_chunkRenderer; }
    SkyRenderer&       skyRenderer()    { return m_skyRenderer; }
    HudRenderer&       hudRenderer()    { return m_hudRenderer; }
    DeferredPipeline&  deferred()       { return m_deferred; }

private:
    TextureAtlas    m_atlas;
    ChunkRenderer   m_chunkRenderer;
    SkyRenderer     m_skyRenderer;
    HudRenderer     m_hudRenderer;
    DeferredPipeline m_deferred;

    Shader         m_outlineShader;
    Shader         m_crackShader;
    Shader         m_uiShader;
    GLuint         m_outlineVao = 0, m_outlineVbo = 0;
    GLuint         m_crossVao = 0, m_crossVbo = 0;
    GLuint         m_crackVao = 0, m_crackVbo = 0;
    GLuint         m_dropVao = 0, m_dropVbo = 0, m_dropEbo = 0;

    int m_screenW = 0, m_screenH = 0;

    void initOutline();
    void initCrosshair();
    void initCrackQuads();
    void initDropCube();

    // Deferred rendering sub-steps
    void renderShadowPass(World& world, const Camera& cam, float aspect, float totalTime);
    void renderGBufferPass(World& world, const Camera& cam, float aspect, float totalTime);
    void renderDeferredLighting(const Camera& cam, float aspect, float totalTime);
};
