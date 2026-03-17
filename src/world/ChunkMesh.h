#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

class Chunk;

struct ChunkVertex {
    glm::vec3 position;
    glm::vec2 uv;       // face-local UV (>1 for greedy-merged quads)
    glm::vec3 normal;
    float     ao;
    float     texIndex;  // atlas tile index
};

class ChunkMesh {
public:
    void build(const Chunk& chunk,
               const Chunk* nx_neg, const Chunk* nx_pos,
               const Chunk* nz_neg, const Chunk* nz_pos);

    const std::vector<ChunkVertex>& solidVerts()   const { return m_verts; }
    const std::vector<uint32_t>&    solidIndices()  const { return m_idx;   }

    const std::vector<ChunkVertex>& waterVerts()    const { return m_wVerts; }
    const std::vector<uint32_t>&    waterIndices()  const { return m_wIdx;   }

    bool empty() const { return m_verts.empty() && m_wVerts.empty(); }

private:
    std::vector<ChunkVertex> m_verts;
    std::vector<uint32_t>    m_idx;
    std::vector<ChunkVertex> m_wVerts;
    std::vector<uint32_t>    m_wIdx;

    // Helper: get block id, accounting for neighbour chunks at edges
    static uint8_t sampleBlock(int x, int y, int z,
                               const Chunk& c,
                               const Chunk* xn, const Chunk* xp,
                               const Chunk* zn, const Chunk* zp);

    // Ambient-occlusion value for one vertex corner
    static float vertexAO(bool side1, bool side2, bool corner);

    // Greedy-mesh one face direction
    void greedyFace(int axis, int dir,
                    const Chunk& c,
                    const Chunk* xn, const Chunk* xp,
                    const Chunk* zn, const Chunk* zp);
};
