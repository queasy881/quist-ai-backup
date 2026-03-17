#include "rendering/SkyRenderer.h"
#include "player/Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>

static constexpr float DAY_LENGTH = 600.0f; // seconds per full cycle

// Full-screen triangle trick (covers entire screen with one triangle)
static float kSkyVerts[] = {
    -1.0f, -1.0f,
     3.0f, -1.0f,
    -1.0f,  3.0f,
};

SkyRenderer::SkyRenderer() {}

SkyRenderer::~SkyRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

void SkyRenderer::init(const std::string& vp, const std::string& fp) {
    m_shader = Shader(vp, fp);
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kSkyVerts), kSkyVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindVertexArray(0);
}

float SkyRenderer::getDaylight(float t) const {
    float angle = std::fmod(t, DAY_LENGTH) / DAY_LENGTH;   // 0..1
    float sun   = std::sin(angle * 3.14159265f * 2.0f);     // -1..1
    return std::clamp(sun * 0.5f + 0.5f, 0.15f, 1.0f);
}

glm::vec3 SkyRenderer::getSunDirection(float t) const {
    float angle = std::fmod(t, DAY_LENGTH) / DAY_LENGTH * 3.14159265f * 2.0f;
    return glm::normalize(glm::vec3(std::cos(angle), std::sin(angle), 0.3f));
}

glm::vec3 SkyRenderer::getSkyColor(float t) const {
    float d = getDaylight(t);
    glm::vec3 day(0.45f, 0.65f, 1.0f);
    glm::vec3 night(0.01f, 0.01f, 0.04f);
    return glm::mix(night, day, d);
}

glm::vec3 SkyRenderer::getFogColor(float t) const {
    float d = getDaylight(t);
    glm::vec3 dayFog(0.6f, 0.75f, 1.0f);
    glm::vec3 nightFog(0.02f, 0.02f, 0.05f);
    return glm::mix(nightFog, dayFog, d);
}

void SkyRenderer::render(const Camera& cam, float aspect, float totalTime) {
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    m_shader.use();
    glm::mat4 view = glm::mat4(glm::mat3(cam.getViewMatrix())); // strip translation
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    m_shader.setMat4("uInvVP", glm::inverse(proj * view));
    m_shader.setFloat("uTime", totalTime);
    m_shader.setFloat("uDayLength", DAY_LENGTH);

    glm::vec3 sunDir = getSunDirection(totalTime);
    m_shader.setVec3("uSunDir", sunDir);

    float daylight = getDaylight(totalTime);
    m_shader.setFloat("uDaylight", daylight);

    glm::vec3 skyCol = getSkyColor(totalTime);
    m_shader.setVec3("uSkyColor", skyCol);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}
