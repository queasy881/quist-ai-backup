#include "rendering/Texture.h"
#include <stb_image.h>
#include <cstdio>

Texture::~Texture() { if (m_id) glDeleteTextures(1, &m_id); }

Texture::Texture(Texture&& o) noexcept : m_id(o.m_id), m_w(o.m_w), m_h(o.m_h) { o.m_id = 0; }
Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) { if (m_id) glDeleteTextures(1, &m_id); m_id = o.m_id; m_w = o.m_w; m_h = o.m_h; o.m_id = 0; }
    return *this;
}

bool Texture::loadFromFile(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);
    int ch = 0;
    unsigned char* data = stbi_load(path.c_str(), &m_w, &m_h, &ch, 4);
    if (!data) {
        std::fprintf(stderr, "Texture load failed: %s\n", path.c_str());
        return false;
    }
    bool ok = loadFromMemory(data, m_w, m_h, 4);
    stbi_image_free(data);
    return ok;
}

bool Texture::loadFromMemory(const unsigned char* data, int w, int h, int ch) {
    m_w = w; m_h = h;
    if (m_id) glDeleteTextures(1, &m_id);
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    return true;
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
