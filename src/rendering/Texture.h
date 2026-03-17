#pragma once
#include <glad/gl.h>
#include <string>

class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(Texture&& o) noexcept;
    Texture& operator=(Texture&& o) noexcept;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool loadFromFile(const std::string& path);
    bool loadFromMemory(const unsigned char* data, int w, int h, int ch);
    void bind(int unit = 0) const;

    GLuint id()     const { return m_id; }
    int    width()  const { return m_w; }
    int    height() const { return m_h; }

private:
    GLuint m_id = 0;
    int m_w = 0, m_h = 0;
};
