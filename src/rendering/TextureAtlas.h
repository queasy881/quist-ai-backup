#pragma once
#include <glad/gl.h>
#include <string>
#include <vector>

class TextureAtlas {
public:
    TextureAtlas() = default;
    ~TextureAtlas();

    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    bool build(const std::vector<std::string>& paths, int tileSize = 0);
    void bind(int unit = 0) const;

    int   tilesPerRow() const { return m_tpr; }
    float tileUVSize()  const { return 1.0f / static_cast<float>(m_tpr); }
    GLuint id()         const { return m_id;  }

private:
    GLuint m_id  = 0;
    int    m_tpr = 0;   // tiles per row
    int    m_atlasPx = 0;
};
