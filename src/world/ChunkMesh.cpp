#include "world/ChunkMesh.h"
#include "world/Chunk.h"
#include "world/BlockRegistry.h"
#include <cstring>
#include <algorithm>

// ── helpers ──────────────────────────────────────────────────

uint8_t ChunkMesh::sampleBlock(int x, int y, int z,
                                const Chunk& c,
                                const Chunk* xn, const Chunk* xp,
                                const Chunk* zn, const Chunk* zp)
{
    if (y < 0 || y >= Chunk::HEIGHT) return 0;
    if (x >= 0 && x < Chunk::SIZE && z >= 0 && z < Chunk::SIZE)
        return c.getBlock(x, y, z);
    if (x < 0  && xn) return xn->getBlock(x + Chunk::SIZE, y, z);
    if (x >= Chunk::SIZE && xp) return xp->getBlock(x - Chunk::SIZE, y, z);
    if (z < 0  && zn) return zn->getBlock(x, y, z + Chunk::SIZE);
    if (z >= Chunk::SIZE && zp) return zp->getBlock(x, y, z - Chunk::SIZE);
    return 0;
}

float ChunkMesh::vertexAO(bool s1, bool s2, bool corner) {
    if (s1 && s2) return 0.0f;
    return (3.0f - (s1 ? 1.0f : 0.0f) - (s2 ? 1.0f : 0.0f) - (corner ? 1.0f : 0.0f)) / 3.0f;
}

// ── build ────────────────────────────────────────────────────

void ChunkMesh::build(const Chunk& chunk,
                      const Chunk* xn, const Chunk* xp,
                      const Chunk* zn, const Chunk* zp)
{
    m_verts.clear(); m_idx.clear();
    m_wVerts.clear(); m_wIdx.clear();

    // 6 faces: axis 0=X, 1=Y, 2=Z  ×  dir -1 / +1
    greedyFace(0, -1, chunk, xn, xp, zn, zp); // -X
    greedyFace(0, +1, chunk, xn, xp, zn, zp); // +X
    greedyFace(1, -1, chunk, xn, xp, zn, zp); // -Y
    greedyFace(1, +1, chunk, xn, xp, zn, zp); // +Y
    greedyFace(2, -1, chunk, xn, xp, zn, zp); // -Z
    greedyFace(2, +1, chunk, xn, xp, zn, zp); // +Z
}

// ── greedy meshing for one face direction ───────────────────

void ChunkMesh::greedyFace(int axis, int dir,
                            const Chunk& c,
                            const Chunk* xn, const Chunk* xp,
                            const Chunk* zn, const Chunk* zp)
{
    // axis: 0=X, 1=Y, 2=Z
    // u, v are the two axes perpendicular to 'axis'
    int dims[3] = { Chunk::SIZE, Chunk::HEIGHT, Chunk::SIZE };
    int u = (axis + 1) % 3;
    int v = (axis + 2) % 3;

    int du = dims[u];
    int dv = dims[v];

    // Mask: one slice along the axis direction
    struct MaskEntry { int16_t blockId; float ao[4]; };
    std::vector<MaskEntry> mask(du * dv);

    glm::vec3 normal(0);
    normal[axis] = static_cast<float>(dir);

    int sliceCount = dims[axis];
    for (int slice = 0; slice < sliceCount; ++slice) {
        // Fill mask
        std::memset(mask.data(), 0, mask.size() * sizeof(MaskEntry));

        for (int iv = 0; iv < dv; ++iv) {
            for (int iu = 0; iu < du; ++iu) {
                int pos[3]; pos[axis] = slice; pos[u] = iu; pos[v] = iv;
                int nx_[3]; nx_[axis] = slice + dir; nx_[u] = iu; nx_[v] = iv;

                uint8_t cur  = sampleBlock(pos[0], pos[1], pos[2], c, xn, xp, zn, zp);
                uint8_t neig = sampleBlock(nx_[0], nx_[1], nx_[2], c, xn, xp, zn, zp);

                const Block& bCur  = BlockRegistry::get(cur);
                const Block& bNeig = BlockRegistry::get(neig);

                // Emit face only between solid/non-solid boundary
                bool curSolid  = bCur.solid || bCur.isFluid();
                bool neigSolid = bNeig.solid || bNeig.isFluid();

                if (!curSolid)  continue;
                if (neigSolid && !bNeig.transparent) continue;
                if (bCur.isFluid() && bNeig.isFluid()) continue; // skip internal water faces

                // Determine texture index for this face
                int texId;
                if (axis == 1 && dir == 1)      texId = bCur.texTop;
                else if (axis == 1 && dir == -1) texId = bCur.texBottom;
                else                              texId = bCur.texSide;

                // Compute AO at the 4 corners of this single face
                float ao[4];
                {
                    // Corner offsets in (u, v) order: (0,0) (1,0) (1,1) (0,1)
                    int du0[4] = {0, 1, 1, 0};
                    int dv0[4] = {0, 0, 1, 1};
                    for (int c_ = 0; c_ < 4; ++c_) {
                        // Side/corner neighbours for AO
                        int step = (dir > 0) ? 1 : 0; // offset along axis to the face plane
                        int base_[3]; base_[axis] = slice + step; base_[u] = iu + du0[c_]; base_[v] = iv + dv0[c_];

                        int su[3], sv[3], sc[3];
                        for (int k = 0; k < 3; ++k) { su[k] = base_[k]; sv[k] = base_[k]; sc[k] = base_[k]; }
                        su[u] += (du0[c_] == 0 ? -1 : 1);
                        sv[v] += (dv0[c_] == 0 ? -1 : 1);
                        sc[u] += (du0[c_] == 0 ? -1 : 1);
                        sc[v] += (dv0[c_] == 0 ? -1 : 1);

                        bool s1 = BlockRegistry::get(sampleBlock(su[0], su[1], su[2], c, xn, xp, zn, zp)).solid;
                        bool s2 = BlockRegistry::get(sampleBlock(sv[0], sv[1], sv[2], c, xn, xp, zn, zp)).solid;
                        bool cn = BlockRegistry::get(sampleBlock(sc[0], sc[1], sc[2], c, xn, xp, zn, zp)).solid;
                        ao[c_] = vertexAO(s1, s2, cn);
                    }
                }

                int mi = iv * du + iu;
                mask[mi].blockId = static_cast<int16_t>(texId + 1); // +1 so 0 = empty
                for (int k = 0; k < 4; ++k) mask[mi].ao[k] = ao[k];
            }
        }

        // Greedy merge
        std::vector<bool> visited(du * dv, false);

        for (int iv = 0; iv < dv; ++iv) {
            for (int iu = 0; iu < du; ++iu) {
                int mi = iv * du + iu;
                if (visited[mi] || mask[mi].blockId == 0) continue;

                auto& ref = mask[mi];

                // Extend along u
                int width = 1;
                while (iu + width < du) {
                    int ni = iv * du + (iu + width);
                    if (visited[ni] || mask[ni].blockId != ref.blockId) break;
                    // Check AO matches for mergeable face
                    bool aoMatch = true;
                    for (int k = 0; k < 4; ++k)
                        if (mask[ni].ao[k] != ref.ao[k]) { aoMatch = false; break; }
                    if (!aoMatch) break;
                    ++width;
                }

                // Extend along v
                int height = 1;
                bool done = false;
                while (iv + height < dv && !done) {
                    for (int k = 0; k < width; ++k) {
                        int ni = (iv + height) * du + (iu + k);
                        if (visited[ni] || mask[ni].blockId != ref.blockId) { done = true; break; }
                        bool aoMatch = true;
                        for (int a = 0; a < 4; ++a)
                            if (mask[ni].ao[a] != ref.ao[a]) { aoMatch = false; break; }
                        if (!aoMatch) { done = true; break; }
                    }
                    if (!done) ++height;
                }

                // Mark visited
                for (int jv = 0; jv < height; ++jv)
                    for (int ju = 0; ju < width; ++ju)
                        visited[(iv + jv) * du + (iu + ju)] = true;

                // Emit quad
                float faceOffset = (dir > 0) ? 1.0f : 0.0f;

                glm::vec3 origin(0);
                origin[axis] = static_cast<float>(slice) + faceOffset;
                origin[u]    = static_cast<float>(iu);
                origin[v]    = static_cast<float>(iv);

                glm::vec3 deltaU(0), deltaV(0);
                deltaU[u] = static_cast<float>(width);
                deltaV[v] = static_cast<float>(height);

                float texIdx = static_cast<float>(ref.blockId - 1);
                float uSize = static_cast<float>(width);
                float vSize = static_cast<float>(height);

                // Determine whether this is a water face
                // texIdx == 5 is water
                bool isWater = (ref.blockId - 1 == 5);

                auto& verts  = isWater ? m_wVerts : m_verts;
                auto& indices = isWater ? m_wIdx  : m_idx;

                uint32_t base = static_cast<uint32_t>(verts.size());

                // 4 vertices: 0=origin, 1=+u, 2=+u+v, 3=+v
                ChunkVertex v0, v1, v2, v3;
                v0.position = origin;
                v1.position = origin + deltaU;
                v2.position = origin + deltaU + deltaV;
                v3.position = origin + deltaV;

                v0.uv = {0,     0};
                v1.uv = {uSize, 0};
                v2.uv = {uSize, vSize};
                v3.uv = {0,     vSize};

                if (axis == 0) {
                    // X faces: u=Y(vertical), v=Z(horizontal)
                    // Remap so texU→Z, texV→Y inverted (green at top)
                    v0.uv = {0,     uSize};
                    v1.uv = {0,     0};
                    v2.uv = {vSize, 0};
                    v3.uv = {vSize, uSize};
                } else if (axis == 2) {
                    // Z faces: u=X(horizontal), v=Y(vertical)
                    // Invert V so green (texV=0) maps to high Y (top)
                    v0.uv = {0,     vSize};
                    v1.uv = {uSize, vSize};
                    v2.uv = {uSize, 0};
                    v3.uv = {0,     0};
                }

                v0.normal = v1.normal = v2.normal = v3.normal = normal;
                v0.texIndex = v1.texIndex = v2.texIndex = v3.texIndex = texIdx;

                v0.ao = ref.ao[0];
                v1.ao = ref.ao[1];
                v2.ao = ref.ao[2];
                v3.ao = ref.ao[3];

                verts.push_back(v0);
                verts.push_back(v1);
                verts.push_back(v2);
                verts.push_back(v3);

                // Flip triangle winding for AO to avoid anisotropy artifacts
                if (v0.ao + v2.ao > v1.ao + v3.ao) {
                    if (dir > 0) {
                        indices.push_back(base + 0);
                        indices.push_back(base + 1);
                        indices.push_back(base + 2);
                        indices.push_back(base + 0);
                        indices.push_back(base + 2);
                        indices.push_back(base + 3);
                    } else {
                        indices.push_back(base + 0);
                        indices.push_back(base + 2);
                        indices.push_back(base + 1);
                        indices.push_back(base + 0);
                        indices.push_back(base + 3);
                        indices.push_back(base + 2);
                    }
                } else {
                    if (dir > 0) {
                        indices.push_back(base + 1);
                        indices.push_back(base + 2);
                        indices.push_back(base + 3);
                        indices.push_back(base + 1);
                        indices.push_back(base + 3);
                        indices.push_back(base + 0);
                    } else {
                        indices.push_back(base + 1);
                        indices.push_back(base + 3);
                        indices.push_back(base + 2);
                        indices.push_back(base + 1);
                        indices.push_back(base + 0);
                        indices.push_back(base + 3);
                    }
                }
            }
        }
    }
}
