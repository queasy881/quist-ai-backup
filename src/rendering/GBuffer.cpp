#include "rendering/GBuffer.h"
#include <cstdio>

GBuffer::~GBuffer() { destroy(); }

void GBuffer::destroy() {
    if (m_fbo)   { glDeleteFramebuffers(1, &m_fbo);  m_fbo = 0;   }
    if (m_depth) { glDeleteTextures(1, &m_depth);     m_depth = 0; }
    for (auto& t : m_tex) { if (t) { glDeleteTextures(1, &t); t = 0; } }
}

void GBuffer::create(int width, int height) {
    m_width = width; m_height = height;
    glCreateFramebuffers(1, &m_fbo);
    createAttachments();
}

void GBuffer::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    m_width = width; m_height = height;
    // Delete old textures (keep FBO)
    if (m_depth) glDeleteTextures(1, &m_depth);
    for (auto& t : m_tex) if (t) glDeleteTextures(1, &t);
    createAttachments();
}

void GBuffer::createAttachments() {
    // Albedo(RGB) + Roughness(A)
    glCreateTextures(GL_TEXTURE_2D, 1, &m_tex[0]);
    glTextureStorage2D(m_tex[0], 1, GL_RGBA8, m_width, m_height);
    glTextureParameteri(m_tex[0], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_tex[0], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_tex[0], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_tex[0], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT0, m_tex[0], 0);

    // Normal(RGB, view-space octahedron or full) + Metallic(A)
    glCreateTextures(GL_TEXTURE_2D, 1, &m_tex[1]);
    glTextureStorage2D(m_tex[1], 1, GL_RGBA16F, m_width, m_height);
    glTextureParameteri(m_tex[1], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_tex[1], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_tex[1], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_tex[1], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT1, m_tex[1], 0);

    // World Position(RGB) + AO(A)
    glCreateTextures(GL_TEXTURE_2D, 1, &m_tex[2]);
    glTextureStorage2D(m_tex[2], 1, GL_RGBA16F, m_width, m_height);
    glTextureParameteri(m_tex[2], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_tex[2], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_tex[2], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_tex[2], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT2, m_tex[2], 0);

    // Depth
    glCreateTextures(GL_TEXTURE_2D, 1, &m_depth);
    glTextureStorage2D(m_depth, 1, GL_DEPTH_COMPONENT32F, m_width, m_height);
    glTextureParameteri(m_depth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_depth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_depth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_depth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // For shadow comparison
    glTextureParameteri(m_depth, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glNamedFramebufferTexture(m_fbo, GL_DEPTH_ATTACHMENT, m_depth, 0);

    GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glNamedFramebufferDrawBuffers(m_fbo, 3, drawBuffers);

    GLenum status = glCheckNamedFramebufferStatus(m_fbo, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "GBuffer framebuffer incomplete: 0x%X\n", status);
    }
}

void GBuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void GBuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::bindTextures(int firstUnit) const {
    glBindTextureUnit(firstUnit + 0, m_tex[0]);
    glBindTextureUnit(firstUnit + 1, m_tex[1]);
    glBindTextureUnit(firstUnit + 2, m_tex[2]);
    glBindTextureUnit(firstUnit + 3, m_depth);
}
