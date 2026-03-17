#include "player/Camera.h"
#include <cmath>
#include <algorithm>

Camera::Camera() { updateVectors(); }

void Camera::processMouseMovement(float dx, float dy, float sens) {
    m_yaw   += dx * sens;
    m_pitch -= dy * sens;                 // inverted Y
    m_pitch  = std::clamp(m_pitch, -89.0f, 89.0f);
    updateVectors();
}

void Camera::updateVectors() {
    float yr = glm::radians(m_yaw);
    float pr = glm::radians(m_pitch);
    m_front = glm::normalize(glm::vec3(
        std::cos(yr) * std::cos(pr),
        std::sin(pr),
        std::sin(yr) * std::cos(pr)));
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_pos, m_pos + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
}

Camera::Frustum Camera::getFrustum(float aspect) const {
    Frustum f{};
    glm::mat4 vp = getProjectionMatrix(aspect) * getViewMatrix();
    // Extract planes (Gribb-Hartmann method)
    for (int i = 0; i < 3; ++i) {
        f.planes[i*2+0] = glm::vec4(
            vp[0][3] + vp[0][i],
            vp[1][3] + vp[1][i],
            vp[2][3] + vp[2][i],
            vp[3][3] + vp[3][i]);
        f.planes[i*2+1] = glm::vec4(
            vp[0][3] - vp[0][i],
            vp[1][3] - vp[1][i],
            vp[2][3] - vp[2][i],
            vp[3][3] - vp[3][i]);
    }
    for (auto& p : f.planes) {
        float len = glm::length(glm::vec3(p));
        if (len > 0.0f) p /= len;
    }
    return f;
}
