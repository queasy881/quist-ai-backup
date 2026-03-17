#pragma once
#include <glad/gl.h>

// Post-processing pipeline: HDR buffer → bloom → tonemap.
// Uses ping-pong framebuffers and a mip-chain for bloom.
class PostProcess {
public:
    PostProcess() = default;
    ~PostProcess();

    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    void create(int width, int height);
    void resize(int width, int height);
    void destroy();

    // Primary HDR buffer (lighting writes here)
    GLuint hdrFBO()  const { return m_hdrFBO; }
    GLuint hdrTex()  const { return m_hdrTex; }
    GLuint hdrDepth()const { return m_hdrDepth; }

    void bindHDR() const;
    void unbind()  const;

    // Ping-pong helper for multi-pass effects
    GLuint pingFBO()  const { return m_pingFBO; }
    GLuint pongFBO()  const { return m_pongFBO; }
    GLuint pingTex()  const { return m_pingTex; }
    GLuint pongTex()  const { return m_pongTex; }

    // Bloom mip chain (6 levels)
    static constexpr int BLOOM_MIPS = 6;
    GLuint bloomFBO(int level) const { return m_bloomFBO[level]; }
    GLuint bloomTex(int level) const { return m_bloomTex[level]; }
    int bloomWidth(int level)  const { return m_bloomW[level]; }
    int bloomHeight(int level) const { return m_bloomH[level]; }

    int width()  const { return m_width; }
    int height() const { return m_height; }

private:
    int m_width = 0, m_height = 0;

    // HDR framebuffer (RGBA16F + depth)
    GLuint m_hdrFBO   = 0;
    GLuint m_hdrTex   = 0;
    GLuint m_hdrDepth = 0;

    // Ping-pong (RGBA16F, full-res)
    GLuint m_pingFBO = 0, m_pongFBO = 0;
    GLuint m_pingTex = 0, m_pongTex = 0;

    // Bloom mip chain (each successively half-res)
    GLuint m_bloomFBO[BLOOM_MIPS] = {};
    GLuint m_bloomTex[BLOOM_MIPS] = {};
    int    m_bloomW[BLOOM_MIPS]   = {};
    int    m_bloomH[BLOOM_MIPS]   = {};

    void createFBOs();
};
