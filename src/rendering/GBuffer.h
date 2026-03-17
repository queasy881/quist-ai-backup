#pragma once
#include <glad/gl.h>

// G-Buffer for deferred rendering.
// Attachments:
//   0 – Albedo (RGB) + Roughness (A)       RGBA8
//   1 – Normal (RGB, view-space) + Metallic (A)  RGBA16F
//   2 – World Position (RGB) + AO (A)      RGBA16F
//   Depth – 32-bit float
class GBuffer {
public:
    GBuffer() = default;
    ~GBuffer();

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

    void create(int width, int height);
    void resize(int width, int height);
    void destroy();

    void bind() const;    // bind for geometry writing
    void unbind() const;

    // Individual texture access
    GLuint albedoRoughnessTex() const { return m_tex[0]; }
    GLuint normalMetallicTex()  const { return m_tex[1]; }
    GLuint positionAOTex()      const { return m_tex[2]; }
    GLuint depthTex()           const { return m_depth;  }

    int width()  const { return m_width;  }
    int height() const { return m_height; }

    // Bind all G-Buffer textures to sequential units starting at firstUnit
    void bindTextures(int firstUnit) const;

    GLuint fbo() const { return m_fbo; }

private:
    GLuint m_fbo   = 0;
    GLuint m_tex[3]= {0, 0, 0};
    GLuint m_depth = 0;
    int m_width  = 0;
    int m_height = 0;

    void createAttachments();
};
