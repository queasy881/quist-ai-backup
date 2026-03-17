#include "rendering/TextureAtlas.h"
#include <stb_image.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

TextureAtlas::~TextureAtlas() { if (m_id) glDeleteTextures(1, &m_id); }

bool TextureAtlas::build(const std::vector<std::string>& paths, int tileSize) {
    int count = static_cast<int>(paths.size());

    // Auto-detect tile size from the largest texture if not specified
    if (tileSize <= 0) {
        tileSize = 16;
        for (int i = 0; i < count; ++i) {
            int w = 0, h = 0, ch = 0;
            if (stbi_info(paths[i].c_str(), &w, &h, &ch)) {
                int maxDim = w > h ? w : h;
                if (maxDim > tileSize) tileSize = maxDim;
            }
        }
        std::printf("TextureAtlas: auto tile size = %d\n", tileSize);
    }

    m_tpr     = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
    m_atlasPx = m_tpr * tileSize;

    std::vector<unsigned char> atlas(m_atlasPx * m_atlasPx * 4, 0);

    for (int i = 0; i < count; ++i) {
        int w = 0, h = 0, ch = 0;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* img = stbi_load(paths[i].c_str(), &w, &h, &ch, 4);

        int col = i % m_tpr;
        int row = i / m_tpr;
        int ox  = col * tileSize;
        int oy  = row * tileSize;

        if (img) {
            std::printf("  [%d] %s: %dx%d ch=%d\n", i, paths[i].c_str(), w, h, ch);
            if (w == tileSize && h == tileSize) {
                // Exact match — direct copy
                for (int y = 0; y < tileSize; ++y) {
                    int srcIdx = (y * w + 0) * 4;
                    int dstIdx = ((oy + y) * m_atlasPx + ox) * 4;
                    std::memcpy(&atlas[dstIdx], &img[srcIdx], tileSize * 4);
                }
            } else {
                // Nearest-neighbor downscale (or upscale) to tileSize
                for (int y = 0; y < tileSize; ++y) {
                    int srcY = y * h / tileSize;
                    for (int x = 0; x < tileSize; ++x) {
                        int srcX = x * w / tileSize;
                        int srcIdx = (srcY * w + srcX) * 4;
                        int dstIdx = ((oy + y) * m_atlasPx + (ox + x)) * 4;
                        std::memcpy(&atlas[dstIdx], &img[srcIdx], 4);
                    }
                }
            }
            stbi_image_free(img);
        } else {
            std::fprintf(stderr, "TextureAtlas: missing %s – using magenta\n", paths[i].c_str());
            for (int y = 0; y < tileSize; ++y) {
                for (int x = 0; x < tileSize; ++x) {
                    int idx = ((oy + y) * m_atlasPx + (ox + x)) * 4;
                    atlas[idx+0] = 255; atlas[idx+1] = 0;
                    atlas[idx+2] = 255; atlas[idx+3] = 255;
                }
            }
        }
    }

    if (m_id) glDeleteTextures(1, &m_id);
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 m_atlasPx, m_atlasPx, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Limit mipmap levels to avoid tiles bleeding
    int maxLevels = static_cast<int>(std::log2(tileSize));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevels);
    glGenerateMipmap(GL_TEXTURE_2D);

    std::printf("TextureAtlas: %dx%d (%d tiles, %d per row)\n",
                m_atlasPx, m_atlasPx, count, m_tpr);
    return true;
}

void TextureAtlas::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
