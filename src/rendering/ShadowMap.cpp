#include "rendering/ShadowMap.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <algorithm>
#include <cmath>

ShadowMap::~ShadowMap() { destroy(); }

void ShadowMap::destroy() {
    if (m_fbo)       { glDeleteFramebuffers(1, &m_fbo);  m_fbo = 0; }
    if (m_depthArray) { glDeleteTextures(1, &m_depthArray); m_depthArray = 0; }
}

void ShadowMap::create() {
    // 2D texture array: one layer per cascade
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_depthArray);
    glTextureStorage3D(m_depthArray, 1, GL_DEPTH_COMPONENT32F,
                       SHADOW_RES, SHADOW_RES, NUM_CASCADES);
    glTextureParameteri(m_depthArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_depthArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_depthArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_depthArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.f, 1.f, 1.f, 1.f};
    glTextureParameterfv(m_depthArray, GL_TEXTURE_BORDER_COLOR, borderColor);
    // PCF comparison
    glTextureParameteri(m_depthArray, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_depthArray, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glCreateFramebuffers(1, &m_fbo);
    // We'll attach individual layers before each cascade render
    glNamedFramebufferDrawBuffer(m_fbo, GL_NONE);
    glNamedFramebufferReadBuffer(m_fbo, GL_NONE);

    std::printf("ShadowMap: created %d cascades @ %dx%d\n", NUM_CASCADES, SHADOW_RES, SHADOW_RES);
}

void ShadowMap::update(const glm::mat4& view, const glm::mat4& proj,
                       const glm::vec3& sunDir, float nearPlane, float farPlane) {
    // Lambda for practical split scheme (Nvidia's cascade split)
    float lambda = 0.75f;
    float ratio = farPlane / nearPlane;

    for (int i = 0; i < NUM_CASCADES; ++i) {
        float p = static_cast<float>(i + 1) / static_cast<float>(NUM_CASCADES);
        float log_ = nearPlane * std::pow(ratio, p);
        float uni  = nearPlane + (farPlane - nearPlane) * p;
        m_splits[i] = lambda * log_ + (1.0f - lambda) * uni;
    }

    for (int i = 0; i < NUM_CASCADES; ++i) {
        float nearSplit = (i == 0) ? nearPlane : m_splits[i - 1];
        float farSplit  = m_splits[i];

        // Frustum corners in NDC
        glm::vec3 frustumCorners[8] = {
            {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
            {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
        };

        // Build projection for this sub-frustum
        // Extract FOV/aspect from the projection matrix
        float tanHalfFov = 1.0f / proj[1][1];
        float aspect     = proj[1][1] / proj[0][0];

        // Transform NDC frustum corners to world space for sub-frustum
        glm::mat4 subProj = glm::perspective(2.0f * std::atan(tanHalfFov), aspect, nearSplit, farSplit);
        glm::mat4 invVP = glm::inverse(subProj * view);

        glm::vec3 center(0.0f);
        for (int j = 0; j < 8; ++j) {
            glm::vec4 wc = invVP * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = glm::vec3(wc) / wc.w;
            center += frustumCorners[j];
        }
        center /= 8.0f;

        // Build light view from center
        glm::vec3 lightDir = glm::normalize(sunDir);
        glm::mat4 lightView = glm::lookAt(center - lightDir * 100.0f, center, glm::vec3(0, 1, 0));

        // Find bounding box in light space
        float minX =  1e30f, maxX = -1e30f;
        float minY =  1e30f, maxY = -1e30f;
        float minZ =  1e30f, maxZ = -1e30f;
        for (int j = 0; j < 8; ++j) {
            glm::vec4 ls = lightView * glm::vec4(frustumCorners[j], 1.0f);
            minX = std::min(minX, ls.x); maxX = std::max(maxX, ls.x);
            minY = std::min(minY, ls.y); maxY = std::max(maxY, ls.y);
            minZ = std::min(minZ, ls.z); maxZ = std::max(maxZ, ls.z);
        }

        // Extend Z range to capture shadow casters behind the camera
        float zMult = 4.0f;
        if (minZ < 0) minZ *= zMult; else minZ /= zMult;
        if (maxZ < 0) maxZ /= zMult; else maxZ *= zMult;

        // Snap to texel grid to reduce shimmer
        float worldUnitsPerTexel = (maxX - minX) / static_cast<float>(SHADOW_RES);
        minX = std::floor(minX / worldUnitsPerTexel) * worldUnitsPerTexel;
        maxX = std::floor(maxX / worldUnitsPerTexel) * worldUnitsPerTexel;
        minY = std::floor(minY / worldUnitsPerTexel) * worldUnitsPerTexel;
        maxY = std::floor(maxY / worldUnitsPerTexel) * worldUnitsPerTexel;

        glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        m_lightSpace[i] = lightProj * lightView;
    }
}

void ShadowMap::bindCascade(int i) const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glNamedFramebufferTextureLayer(m_fbo, GL_DEPTH_ATTACHMENT, m_depthArray, 0, i);
    glViewport(0, 0, SHADOW_RES, SHADOW_RES);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bindTexture(int unit) const {
    glBindTextureUnit(unit, m_depthArray);
}
