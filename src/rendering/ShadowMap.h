#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <array>

// Cascaded Shadow Maps – 4 cascades for the directional sun light.
class ShadowMap {
public:
    static constexpr int NUM_CASCADES = 4;
    static constexpr int SHADOW_RES   = 2048;

    ShadowMap() = default;
    ~ShadowMap();

    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    void create();
    void destroy();

    // Compute cascade splits and light-space matrices
    void update(const glm::mat4& view, const glm::mat4& proj,
                const glm::vec3& sunDir, float nearPlane, float farPlane);

    // Bind cascade i for rendering
    void bindCascade(int i) const;
    void unbind() const;

    GLuint depthArrayTex() const { return m_depthArray; }

    const glm::mat4& lightSpaceMatrix(int i) const { return m_lightSpace[i]; }
    const std::array<glm::mat4, NUM_CASCADES>& allLightSpaceMatrices() const { return m_lightSpace; }
    const std::array<float, NUM_CASCADES>& cascadeSplits() const { return m_splits; }

    void bindTexture(int unit) const;

private:
    GLuint m_fbo       = 0;
    GLuint m_depthArray= 0;

    std::array<glm::mat4, NUM_CASCADES> m_lightSpace;
    std::array<float, NUM_CASCADES>     m_splits;
};
