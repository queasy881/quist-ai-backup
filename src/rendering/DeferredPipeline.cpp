#include "rendering/DeferredPipeline.h"
#include "player/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <random>
#include <vector>

DeferredPipeline::~DeferredPipeline() { destroy(); }

void DeferredPipeline::destroy() {
    destroySSAOResources();
    m_gbuffer.destroy();
    m_shadowMap.destroy();
    m_postProcess.destroy();
}

// ── Init ─────────────────────────────────────────────────────

void DeferredPipeline::init(int w, int h) {
    m_width = w; m_height = h;

    FullscreenQuad::init();

    m_gbuffer.create(w, h);
    m_shadowMap.create();
    m_postProcess.create(w, h);

    // Load all shaders
    m_gbufferShader    = Shader("shaders/gbuffer.vert",       "shaders/gbuffer.frag");
    m_shadowShader     = Shader("shaders/shadow.vert",        "shaders/shadow.frag");
    m_lightingShader   = Shader("shaders/deferred_light.vert","shaders/deferred_light.frag");
    m_ssaoShader       = Shader("shaders/fullscreen.vert",    "shaders/ssao.frag");
    m_ssaoBlurShader   = Shader("shaders/fullscreen.vert",    "shaders/ssao_blur.frag");
    m_ssrShader        = Shader("shaders/fullscreen.vert",    "shaders/ssr.frag");
    m_atmosphereShader = Shader("shaders/fullscreen.vert",    "shaders/atmosphere.frag");
    m_volumetricShader = Shader("shaders/fullscreen.vert",    "shaders/volumetric.frag");
    m_bloomDownShader  = Shader("shaders/fullscreen.vert",    "shaders/bloom_down.frag");
    m_bloomUpShader    = Shader("shaders/fullscreen.vert",    "shaders/bloom_up.frag");
    m_tonemapShader    = Shader("shaders/fullscreen.vert",    "shaders/tonemap.frag");

    createSSAOResources();
    std::printf("DeferredPipeline: initialized %dx%d\n", w, h);
}

void DeferredPipeline::resize(int w, int h) {
    if (w == m_width && h == m_height) return;
    m_width = w; m_height = h;
    m_gbuffer.resize(w, h);
    m_postProcess.resize(w, h);
    destroySSAOResources();
    createSSAOResources();
}

// ── SSAO kernel + noise texture ──────────────────────────────

void DeferredPipeline::createSSAOResources() {
    // SSAO FBO (half-res for performance)
    int sw = m_width / 2, sh = m_height / 2;
    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;

    glCreateFramebuffers(1, &m_ssaoFBO);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_ssaoTex);
    glTextureStorage2D(m_ssaoTex, 1, GL_R8, sw, sh);
    glTextureParameteri(m_ssaoTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_ssaoTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_ssaoTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_ssaoTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_ssaoFBO, GL_COLOR_ATTACHMENT0, m_ssaoTex, 0);
    GLenum db = GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(m_ssaoFBO, 1, &db);

    // Blur FBO (same resolution)
    glCreateFramebuffers(1, &m_ssaoBlurFBO);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_ssaoBlurTex);
    glTextureStorage2D(m_ssaoBlurTex, 1, GL_R8, sw, sh);
    glTextureParameteri(m_ssaoBlurTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_ssaoBlurTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_ssaoBlurTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_ssaoBlurTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_ssaoBlurFBO, GL_COLOR_ATTACHMENT0, m_ssaoBlurTex, 0);
    glNamedFramebufferDrawBuffers(m_ssaoBlurFBO, 1, &db);

    // 4x4 noise texture (random tangent-space rotations)
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::vector<glm::vec3> noise(16);
    for (auto& n : noise)
        n = glm::normalize(glm::vec3(dist(rng)*2.f-1.f, dist(rng)*2.f-1.f, 0.0f));

    glCreateTextures(GL_TEXTURE_2D, 1, &m_noiseTex);
    glTextureStorage2D(m_noiseTex, 1, GL_RGB16F, 4, 4);
    glTextureSubImage2D(m_noiseTex, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, noise.data());
    glTextureParameteri(m_noiseTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_noiseTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_noiseTex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_noiseTex, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void DeferredPipeline::destroySSAOResources() {
    auto del = [](GLuint& id, void(*fn)(GLsizei,const GLuint*)) {
        if (id) { fn(1, &id); id = 0; }
    };
    del(m_ssaoFBO, glDeleteFramebuffers);
    del(m_ssaoTex, glDeleteTextures);
    del(m_ssaoBlurFBO, glDeleteFramebuffers);
    del(m_ssaoBlurTex, glDeleteTextures);
    del(m_noiseTex, glDeleteTextures);
}

// ── 1. Shadow pass ───────────────────────────────────────────

void DeferredPipeline::beginShadowPass() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // Reduce peter-panning
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
}

void DeferredPipeline::bindShadowCascade(int i) {
    m_shadowMap.bindCascade(i);
    m_shadowShader.use();
    m_shadowShader.setMat4("uLightSpace", m_shadowMap.lightSpaceMatrix(i));
}

void DeferredPipeline::endShadowPass() {
    m_shadowMap.unbind();
    glDisable(GL_POLYGON_OFFSET_FILL);
}

// ── 2. G-Buffer pass ─────────────────────────────────────────

void DeferredPipeline::beginGBufferPass() {
    m_gbuffer.bind();
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    m_gbufferShader.use();
}

void DeferredPipeline::endGBufferPass() {
    m_gbuffer.unbind();
}

// ── 3. SSAO ──────────────────────────────────────────────────

void DeferredPipeline::ssaoPass(const Camera& cam, float aspect) {
    int sw = m_width / 2, sh = m_height / 2;
    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;

    // Generate SSAO hemisphere kernel (64 samples)
    // Embedded in the shader as uniform array
    static bool kernelGenerated = false;
    static glm::vec3 kernel[64];
    if (!kernelGenerated) {
        std::mt19937 rng(0);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        for (int i = 0; i < 64; ++i) {
            glm::vec3 s(dist(rng)*2.f-1.f, dist(rng)*2.f-1.f, dist(rng));
            s = glm::normalize(s) * dist(rng);
            float scale = float(i) / 64.0f;
            scale = 0.1f + scale * scale * 0.9f; // lerp(0.1, 1.0, scale^2)
            kernel[i] = s * scale;
        }
        kernelGenerated = true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
    glViewport(0, 0, sw, sh);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_ssaoShader.use();
    // Bind G-Buffer textures
    glBindTextureUnit(0, m_gbuffer.positionAOTex());
    glBindTextureUnit(1, m_gbuffer.normalMetallicTex());
    glBindTextureUnit(2, m_noiseTex);

    m_ssaoShader.setInt("uPosition", 0);
    m_ssaoShader.setInt("uNormal", 1);
    m_ssaoShader.setInt("uNoise", 2);

    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    glm::mat4 view = cam.getViewMatrix();
    m_ssaoShader.setMat4("uProj", proj);
    m_ssaoShader.setMat4("uView", view);
    m_ssaoShader.setVec2("uNoiseScale", glm::vec2(float(sw)/4.f, float(sh)/4.f));

    for (int i = 0; i < 64; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "uSamples[%d]", i);
        m_ssaoShader.setVec3(name, kernel[i]);
    }

    FullscreenQuad::draw();

    // Blur pass
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
    glViewport(0, 0, sw, sh);
    glClear(GL_COLOR_BUFFER_BIT);

    m_ssaoBlurShader.use();
    glBindTextureUnit(0, m_ssaoTex);
    m_ssaoBlurShader.setInt("uInput", 0);

    FullscreenQuad::draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ── 4. Deferred lighting ─────────────────────────────────────

void DeferredPipeline::lightingPass(const Camera& cam, float aspect,
                                     float daylight, const glm::vec3& sunDir,
                                     const glm::vec3& fogColor,
                                     float fogStart, float fogEnd) {
    m_postProcess.bindHDR();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_lightingShader.use();

    // G-Buffer textures
    m_gbuffer.bindTextures(0); // units 0,1,2
    glBindTextureUnit(3, m_gbuffer.depthTex());

    // SSAO
    glBindTextureUnit(4, m_ssaoBlurTex);

    // Shadow cascades
    m_shadowMap.bindTexture(5);

    m_lightingShader.setInt("uAlbedoRoughness", 0);
    m_lightingShader.setInt("uNormalMetallic",  1);
    m_lightingShader.setInt("uPositionAO",      2);
    m_lightingShader.setInt("uDepth",           3);
    m_lightingShader.setInt("uSSAO",            4);
    m_lightingShader.setInt("uShadowMap",       5);

    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    m_lightingShader.setMat4("uView", view);
    m_lightingShader.setMat4("uProj", proj);
    m_lightingShader.setMat4("uInvView", glm::inverse(view));
    m_lightingShader.setMat4("uInvProj", glm::inverse(proj));
    m_lightingShader.setVec3("uCamPos", cam.getPosition());
    m_lightingShader.setVec3("uSunDir", glm::normalize(sunDir));
    m_lightingShader.setFloat("uDaylight", daylight);
    m_lightingShader.setVec3("uFogColor", fogColor);
    m_lightingShader.setFloat("uFogStart", fogStart);
    m_lightingShader.setFloat("uFogEnd", fogEnd);

    // Pass cascade data
    for (int i = 0; i < ShadowMap::NUM_CASCADES; ++i) {
        char name[64];
        std::snprintf(name, sizeof(name), "uLightSpace[%d]", i);
        m_lightingShader.setMat4(name, m_shadowMap.lightSpaceMatrix(i));
        std::snprintf(name, sizeof(name), "uCascadeSplit[%d]", i);
        m_lightingShader.setFloat(name, m_shadowMap.cascadeSplits()[i]);
    }

    FullscreenQuad::draw();

    // Copy G-Buffer depth to HDR FBO for subsequent passes that need depth
    glBlitNamedFramebuffer(m_gbuffer.fbo(), m_postProcess.hdrFBO(),
                           0, 0, m_width, m_height,
                           0, 0, m_width, m_height,
                           GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    m_postProcess.unbind();
}

// ── 5. SSR ───────────────────────────────────────────────────

void DeferredPipeline::ssrPass(const Camera& cam, float aspect) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_postProcess.pingFBO());
    glViewport(0, 0, m_width, m_height);
    glDisable(GL_DEPTH_TEST);

    m_ssrShader.use();
    glBindTextureUnit(0, m_postProcess.hdrTex());
    glBindTextureUnit(1, m_gbuffer.normalMetallicTex());
    glBindTextureUnit(2, m_gbuffer.positionAOTex());
    glBindTextureUnit(3, m_gbuffer.depthTex());

    m_ssrShader.setInt("uColor", 0);
    m_ssrShader.setInt("uNormalMetallic", 1);
    m_ssrShader.setInt("uPositionAO", 2);
    m_ssrShader.setInt("uDepth", 3);

    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    m_ssrShader.setMat4("uView", view);
    m_ssrShader.setMat4("uProj", proj);
    m_ssrShader.setVec2("uResolution", glm::vec2(m_width, m_height));

    FullscreenQuad::draw();

    // Copy result back to HDR
    glBlitNamedFramebuffer(m_postProcess.pingFBO(), m_postProcess.hdrFBO(),
                           0, 0, m_width, m_height,
                           0, 0, m_width, m_height,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ── 6. Atmosphere ────────────────────────────────────────────

void DeferredPipeline::atmospherePass(const Camera& cam, float aspect,
                                       float totalTime, SkyRenderer& sky) {
    m_postProcess.bindHDR();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    m_atmosphereShader.use();
    glBindTextureUnit(0, m_gbuffer.depthTex());
    m_atmosphereShader.setInt("uDepth", 0);

    glm::mat4 view = glm::mat4(glm::mat3(cam.getViewMatrix()));
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    m_atmosphereShader.setMat4("uInvVP", glm::inverse(proj * view));
    m_atmosphereShader.setFloat("uTime", totalTime);
    m_atmosphereShader.setVec3("uSunDir", sky.getSunDirection(totalTime));
    m_atmosphereShader.setFloat("uDaylight", sky.getDaylight(totalTime));

    FullscreenQuad::draw();

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    m_postProcess.unbind();
}

// ── 7. Volumetric ────────────────────────────────────────────

void DeferredPipeline::volumetricPass(const Camera& cam, float aspect,
                                       const glm::vec3& sunDir, float daylight) {
    // Render volumetric at full resolution into ping buffer (cleared first)
    glBindFramebuffer(GL_FRAMEBUFFER, m_postProcess.pingFBO());
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_volumetricShader.use();
    glBindTextureUnit(0, m_gbuffer.depthTex());
    m_shadowMap.bindTexture(1);

    m_volumetricShader.setInt("uDepth", 0);
    m_volumetricShader.setInt("uShadowMap", 1);

    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    m_volumetricShader.setMat4("uInvVP", glm::inverse(proj * view));
    m_volumetricShader.setVec3("uCamPos", cam.getPosition());
    m_volumetricShader.setVec3("uSunDir", glm::normalize(sunDir));
    m_volumetricShader.setFloat("uDaylight", daylight);

    for (int i = 0; i < ShadowMap::NUM_CASCADES; ++i) {
        char name[64];
        std::snprintf(name, sizeof(name), "uLightSpace[%d]", i);
        m_volumetricShader.setMat4(name, m_shadowMap.lightSpaceMatrix(i));
    }

    FullscreenQuad::draw();

    // Additively blend volumetric result into HDR buffer
    m_postProcess.bindHDR();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glViewport(0, 0, m_width, m_height);

    // Simple blit using tonemap shader in passthrough would be complex,
    // so we use a direct blit + additive blend
    m_tonemapShader.use();
    glBindTextureUnit(0, m_postProcess.pingTex());
    m_tonemapShader.setInt("uHDR", 0);
    m_tonemapShader.setFloat("uExposure", 1.0f);
    m_tonemapShader.setInt("uPassthrough", 1); // passthrough mode
    FullscreenQuad::draw();

    glDisable(GL_BLEND);
    m_postProcess.unbind();
}

// ── 8. Bloom ─────────────────────────────────────────────────

void DeferredPipeline::bloomPass() {
    glDisable(GL_DEPTH_TEST);

    // Downsample from HDR → bloom chain
    GLuint srcTex = m_postProcess.hdrTex();
    for (int i = 0; i < PostProcess::BLOOM_MIPS; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_postProcess.bloomFBO(i));
        glViewport(0, 0, m_postProcess.bloomWidth(i), m_postProcess.bloomHeight(i));

        m_bloomDownShader.use();
        glBindTextureUnit(0, srcTex);
        m_bloomDownShader.setInt("uInput", 0);
        m_bloomDownShader.setVec2("uSrcResolution",
            i == 0 ? glm::vec2(m_width, m_height)
                   : glm::vec2(m_postProcess.bloomWidth(i-1), m_postProcess.bloomHeight(i-1)));
        m_bloomDownShader.setInt("uFirstPass", i == 0 ? 1 : 0);

        FullscreenQuad::draw();
        srcTex = m_postProcess.bloomTex(i);
    }

    // Upsample from bottom of mip chain back up
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    for (int i = PostProcess::BLOOM_MIPS - 2; i >= 0; --i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_postProcess.bloomFBO(i));
        glViewport(0, 0, m_postProcess.bloomWidth(i), m_postProcess.bloomHeight(i));

        m_bloomUpShader.use();
        glBindTextureUnit(0, m_postProcess.bloomTex(i + 1));
        m_bloomUpShader.setInt("uInput", 0);
        m_bloomUpShader.setFloat("uFilterRadius", 0.005f);

        FullscreenQuad::draw();
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ── 9. Tone mapping ──────────────────────────────────────────

void DeferredPipeline::tonemapPass(float exposure, float totalTime) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    m_tonemapShader.use();
    glBindTextureUnit(0, m_postProcess.hdrTex());
    glBindTextureUnit(1, m_postProcess.bloomTex(0));

    m_tonemapShader.setInt("uHDR", 0);
    m_tonemapShader.setInt("uBloom", 1);
    m_tonemapShader.setFloat("uExposure", exposure);
    m_tonemapShader.setFloat("uBloomStrength", 0.04f);
    m_tonemapShader.setFloat("uTime", totalTime);
    m_tonemapShader.setInt("uPassthrough", 0);

    FullscreenQuad::draw();
}
