#pragma once
#include "rendering/Shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>

class Camera;

class SkyRenderer {
public:
    SkyRenderer();
    ~SkyRenderer();

    void init(const std::string& vertPath, const std::string& fragPath);
    void render(const Camera& cam, float aspect, float totalTime);

    float getDaylight(float totalTime) const;
    glm::vec3 getSunDirection(float totalTime) const;
    glm::vec3 getSkyColor(float totalTime) const;
    glm::vec3 getFogColor(float totalTime) const;

private:
    Shader m_shader;
    GLuint m_vao = 0, m_vbo = 0;
};
