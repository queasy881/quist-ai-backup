#pragma once
#include "rendering/GBuffer.h"
#include "rendering/ShadowMap.h"
#include "rendering/PostProcess.h"
#include "rendering/FullscreenQuad.h"
#include "rendering/Shader.h"
#include "rendering/TextureAtlas.h"
#include "rendering/SkyRenderer.h"
#include "world/World.h"
#include "player/Camera.h"
#include <glm/glm.hpp>

// Owns all deferred-rendering resources and orchestrates the full pipeline:
//   Shadow → G-Buffer → SSAO → Deferred Lighting → SSR → Atmosphere → Water →
//   Volumetric → Bloom → Tone map
class DeferredPipeline {
public:
    DeferredPipeline() = default;
    ~DeferredPipeline();

    void init(int width, int height);
    void resize(int width, int height);
    void destroy();

    // ── Individual pass methods ──

    // 1. Shadow pass: renders scene from sun's perspective into CSM
    void beginShadowPass();
    void bindShadowCascade(int i);
    void endShadowPass();

    // 2. G-Buffer geometry pass
    void beginGBufferPass();
    void endGBufferPass();

    // 3. SSAO (reads from G-Buffer)
    void ssaoPass(const Camera& cam, float aspect);

    // 4. Deferred lighting (PBR + CSM shadows + SSAO → HDR)
    void lightingPass(const Camera& cam, float aspect, float daylight,
                      const glm::vec3& sunDir, const glm::vec3& fogColor,
                      float fogStart, float fogEnd);

    // 5. SSR
    void ssrPass(const Camera& cam, float aspect);

    // 6. Atmosphere (sky fill for unlit pixels, drawn into HDR)
    void atmospherePass(const Camera& cam, float aspect, float totalTime,
                        SkyRenderer& sky);

    // 7. Volumetric light (god rays)
    void volumetricPass(const Camera& cam, float aspect,
                        const glm::vec3& sunDir, float daylight);

    // 8-9. Bloom + Tone map → final output
    void bloomPass();
    void tonemapPass(float exposure, float totalTime);

    // Access to sub-objects for integration
    GBuffer&     gbuffer()     { return m_gbuffer; }
    ShadowMap&   shadowMap()   { return m_shadowMap; }
    PostProcess& postProcess() { return m_postProcess; }
    Shader&      gbufferShader()  { return m_gbufferShader; }
    Shader&      shadowShader()   { return m_shadowShader; }

    GLuint ssaoTex() const { return m_ssaoBlurTex; }

private:
    int m_width = 0, m_height = 0;

    GBuffer     m_gbuffer;
    ShadowMap   m_shadowMap;
    PostProcess m_postProcess;

    // Shaders
    Shader m_gbufferShader;
    Shader m_shadowShader;
    Shader m_lightingShader;
    Shader m_ssaoShader;
    Shader m_ssaoBlurShader;
    Shader m_ssrShader;
    Shader m_atmosphereShader;
    Shader m_volumetricShader;
    Shader m_bloomDownShader;
    Shader m_bloomUpShader;
    Shader m_tonemapShader;

    // SSAO resources
    GLuint m_ssaoFBO     = 0;
    GLuint m_ssaoTex     = 0;
    GLuint m_ssaoBlurFBO = 0;
    GLuint m_ssaoBlurTex = 0;
    GLuint m_noiseTex    = 0;

    void createSSAOResources();
    void destroySSAOResources();
};
