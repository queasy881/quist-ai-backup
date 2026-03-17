#include "rendering/PostProcess.h"
#include <cstdio>
#include <algorithm>

PostProcess::~PostProcess() { destroy(); }

void PostProcess::destroy() {
    auto del = [](GLuint& fbo, GLuint& tex) {
        if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
        if (tex) { glDeleteTextures(1, &tex);      tex = 0; }
    };

    if (m_hdrDepth) { glDeleteRenderbuffers(1, &m_hdrDepth); m_hdrDepth = 0; }
    del(m_hdrFBO, m_hdrTex);
    del(m_pingFBO, m_pingTex);
    del(m_pongFBO, m_pongTex);

    for (int i = 0; i < BLOOM_MIPS; ++i)
        del(m_bloomFBO[i], m_bloomTex[i]);
}

void PostProcess::create(int w, int h) {
    m_width = w; m_height = h;
    createFBOs();
    std::printf("PostProcess: created %dx%d (%d bloom mips)\n", w, h, BLOOM_MIPS);
}

void PostProcess::resize(int w, int h) {
    if (w == m_width && h == m_height) return;
    destroy();
    m_width = w; m_height = h;
    createFBOs();
}

void PostProcess::createFBOs() {
    // ── HDR framebuffer ──
    glCreateFramebuffers(1, &m_hdrFBO);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_hdrTex);
    glTextureStorage2D(m_hdrTex, 1, GL_RGBA16F, m_width, m_height);
    glTextureParameteri(m_hdrTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_hdrTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_hdrTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_hdrTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_hdrFBO, GL_COLOR_ATTACHMENT0, m_hdrTex, 0);

    // Depth as renderbuffer (shared with G-Buffer depth if needed)
    glCreateRenderbuffers(1, &m_hdrDepth);
    glNamedRenderbufferStorage(m_hdrDepth, GL_DEPTH_COMPONENT32F, m_width, m_height);
    glNamedFramebufferRenderbuffer(m_hdrFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_hdrDepth);

    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glNamedFramebufferDrawBuffers(m_hdrFBO, 1, &drawBuf);

    if (glCheckNamedFramebufferStatus(m_hdrFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::printf("ERROR: HDR FBO incomplete!\n");

    // ── Ping-pong buffers ──
    auto createFullRes = [&](GLuint& fbo, GLuint& tex) {
        glCreateFramebuffers(1, &fbo);
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        glTextureStorage2D(tex, 1, GL_RGBA16F, m_width, m_height);
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, tex, 0);
        GLenum db = GL_COLOR_ATTACHMENT0;
        glNamedFramebufferDrawBuffers(fbo, 1, &db);
    };
    createFullRes(m_pingFBO, m_pingTex);
    createFullRes(m_pongFBO, m_pongTex);

    // ── Bloom mip chain ──
    int bw = std::max(1, m_width / 2);
    int bh = std::max(1, m_height / 2);
    for (int i = 0; i < BLOOM_MIPS; ++i) {
        m_bloomW[i] = bw;
        m_bloomH[i] = bh;
        glCreateFramebuffers(1, &m_bloomFBO[i]);
        glCreateTextures(GL_TEXTURE_2D, 1, &m_bloomTex[i]);
        glTextureStorage2D(m_bloomTex[i], 1, GL_R11F_G11F_B10F, bw, bh);
        glTextureParameteri(m_bloomTex[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_bloomTex[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_bloomTex[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_bloomTex[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(m_bloomFBO[i], GL_COLOR_ATTACHMENT0, m_bloomTex[i], 0);
        GLenum db = GL_COLOR_ATTACHMENT0;
        glNamedFramebufferDrawBuffers(m_bloomFBO[i], 1, &db);
        bw = std::max(1, bw / 2);
        bh = std::max(1, bh / 2);
    }
}

void PostProcess::bindHDR() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
    glViewport(0, 0, m_width, m_height);
}

void PostProcess::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
