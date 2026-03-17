#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    struct Frustum { glm::vec4 planes[6]; };

    Camera();

    void processMouseMovement(float dx, float dy, float sensitivity = 0.1f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;
    Frustum   getFrustum(float aspect) const;

    glm::vec3 getPosition() const { return m_pos; }
    void setPosition(const glm::vec3& p) { m_pos = p; }

    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getRight() const { return m_right; }
    glm::vec3 getUp()    const { return m_up;    }

    float getYaw()   const { return m_yaw;   }
    float getPitch() const { return m_pitch; }
    float getFov()   const { return m_fov;   }
    void  setFov(float f)  { m_fov = f; }
    float getNear()  const { return m_near; }
    float getFar()   const { return m_far;  }

private:
    void updateVectors();

    glm::vec3 m_pos   {0, 80, 0};
    glm::vec3 m_front {0, 0, -1};
    glm::vec3 m_up    {0, 1, 0};
    glm::vec3 m_right {1, 0, 0};
    glm::vec3 m_worldUp{0, 1, 0};

    float m_yaw   = -90.0f;
    float m_pitch = 0.0f;
    float m_fov   = 70.0f;
    float m_near  = 0.05f;
    float m_far   = 500.0f;
};
