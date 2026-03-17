#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setInt  (const char* n, int v) const;
    void setFloat(const char* n, float v) const;
    void setVec2 (const char* n, const glm::vec2& v) const;
    void setVec3 (const char* n, const glm::vec3& v) const;
    void setVec4 (const char* n, const glm::vec4& v) const;
    void setMat4 (const char* n, const glm::mat4& v) const;

    GLuint id() const { return m_prog; }

private:
    GLuint m_prog = 0;
    GLuint compile(GLenum type, const std::string& src);
    static std::string readFile(const std::string& path);
};
