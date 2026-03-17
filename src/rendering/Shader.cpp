#include "rendering/Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <cstdio>

std::string Shader::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::fprintf(stderr, "Shader file not found: %s\n", path.c_str());
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader compile error:\n%s\n", log);
    }
    return s;
}

Shader::Shader(const std::string& vp, const std::string& fp) {
    std::string vs = readFile(vp);
    std::string fs = readFile(fp);
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);

    m_prog = glCreateProgram();
    glAttachShader(m_prog, v);
    glAttachShader(m_prog, f);
    glLinkProgram(m_prog);

    GLint ok = 0;
    glGetProgramiv(m_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(m_prog, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader link error:\n%s\n", log);
    }
    glDeleteShader(v);
    glDeleteShader(f);
}

Shader::~Shader() { if (m_prog) glDeleteProgram(m_prog); }

Shader::Shader(Shader&& o) noexcept : m_prog(o.m_prog) { o.m_prog = 0; }
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) { if (m_prog) glDeleteProgram(m_prog); m_prog = o.m_prog; o.m_prog = 0; }
    return *this;
}

void Shader::use() const { glUseProgram(m_prog); }

void Shader::setInt  (const char* n, int v)            const { glUniform1i(glGetUniformLocation(m_prog, n), v); }
void Shader::setFloat(const char* n, float v)          const { glUniform1f(glGetUniformLocation(m_prog, n), v); }
void Shader::setVec2 (const char* n, const glm::vec2& v) const { glUniform2fv(glGetUniformLocation(m_prog, n), 1, glm::value_ptr(v)); }
void Shader::setVec3 (const char* n, const glm::vec3& v) const { glUniform3fv(glGetUniformLocation(m_prog, n), 1, glm::value_ptr(v)); }
void Shader::setVec4 (const char* n, const glm::vec4& v) const { glUniform4fv(glGetUniformLocation(m_prog, n), 1, glm::value_ptr(v)); }
void Shader::setMat4 (const char* n, const glm::mat4& v) const { glUniformMatrix4fv(glGetUniformLocation(m_prog, n), 1, GL_FALSE, glm::value_ptr(v)); }
