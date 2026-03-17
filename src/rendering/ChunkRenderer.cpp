#include "rendering/ChunkRenderer.h"
#include "world/ChunkMesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

ChunkRenderer::ChunkRenderer() {}

ChunkRenderer::~ChunkRenderer() {
    for (auto& [cc, g] : m_gpu) {
        if (g.vao) glDeleteVertexArrays(1, &g.vao);
        if (g.vbo) glDeleteBuffers(1, &g.vbo);
        if (g.ebo) glDeleteBuffers(1, &g.ebo);
        if (g.wVao) glDeleteVertexArrays(1, &g.wVao);
        if (g.wVbo) glDeleteBuffers(1, &g.wVbo);
        if (g.wEbo) glDeleteBuffers(1, &g.wEbo);
    }
}

void ChunkRenderer::init(const std::string& vp, const std::string& fp) {
    m_shader = Shader(vp, fp);
}

void ChunkRenderer::uploadBuffers(GLuint& vao, GLuint& vbo, GLuint& ebo,
                                   const void* vData, size_t vBytes,
                                   const void* iData, size_t iBytes,
                                   uint32_t& outCount)
{
    if (!vao) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vBytes, vData, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBytes, iData, GL_DYNAMIC_DRAW);

    // ChunkVertex layout: pos(3) uv(2) normal(3) ao(1) texIndex(1) = 10 floats
    size_t stride = 10 * sizeof(float);
    size_t off = 0;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)off); off += 3*sizeof(float);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)off); off += 2*sizeof(float);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)off); off += 3*sizeof(float);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)off); off += sizeof(float);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)off);

    glBindVertexArray(0);
    outCount = static_cast<uint32_t>(iBytes / sizeof(uint32_t));
}

void ChunkRenderer::uploadMesh(const ChunkCoord& cc, const ChunkMesh& mesh) {
    auto& g = m_gpu[cc];

    if (!mesh.solidVerts().empty()) {
        uploadBuffers(g.vao, g.vbo, g.ebo,
            mesh.solidVerts().data(),
            mesh.solidVerts().size() * sizeof(ChunkVertex),
            mesh.solidIndices().data(),
            mesh.solidIndices().size() * sizeof(uint32_t),
            g.indexCount);
    } else {
        g.indexCount = 0;
    }

    if (!mesh.waterVerts().empty()) {
        uploadBuffers(g.wVao, g.wVbo, g.wEbo,
            mesh.waterVerts().data(),
            mesh.waterVerts().size() * sizeof(ChunkVertex),
            mesh.waterIndices().data(),
            mesh.waterIndices().size() * sizeof(uint32_t),
            g.wIndexCount);
    } else {
        g.wIndexCount = 0;
    }
}

void ChunkRenderer::removeMesh(const ChunkCoord& cc) {
    auto it = m_gpu.find(cc);
    if (it == m_gpu.end()) return;
    auto& g = it->second;
    if (g.vao) glDeleteVertexArrays(1, &g.vao);
    if (g.vbo) glDeleteBuffers(1, &g.vbo);
    if (g.ebo) glDeleteBuffers(1, &g.ebo);
    if (g.wVao) glDeleteVertexArrays(1, &g.wVao);
    if (g.wVbo) glDeleteBuffers(1, &g.wVbo);
    if (g.wEbo) glDeleteBuffers(1, &g.wEbo);
    m_gpu.erase(it);
}

void ChunkRenderer::render(const Camera& cam, float aspect,
                            const TextureAtlas& atlas,
                            const World& world,
                            float daylight,
                            const glm::vec3& fogColor)
{
    m_shader.use();

    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    m_shader.setMat4("uView", view);
    m_shader.setMat4("uProj", proj);
    m_shader.setFloat("uAtlasSize", static_cast<float>(atlas.tilesPerRow()));
    m_shader.setFloat("uDaylight", daylight);
    m_shader.setVec3("uFogColor", fogColor);
    m_shader.setFloat("uFogStart", static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE - 32));
    m_shader.setFloat("uFogEnd",   static_cast<float>(World::RENDER_DISTANCE * Chunk::SIZE));
    m_shader.setVec3("uCamPos", cam.getPosition());
    m_shader.setInt("uTexAtlas", 0);

    atlas.bind(0);

    Camera::Frustum frust = cam.getFrustum(aspect);

    // ── Opaque pass ──
    glDisable(GL_BLEND);
    for (auto& [cc, g] : m_gpu) {
        if (g.indexCount == 0) continue;
        // Frustum cull
        glm::vec3 cmin(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE);
        glm::vec3 cmax = cmin + glm::vec3(Chunk::SIZE, Chunk::HEIGHT, Chunk::SIZE);
        glm::vec3 center = (cmin + cmax) * 0.5f;
        float radius = glm::length(cmax - center);

        bool visible = true;
        for (int i = 0; i < 6; ++i) {
            float d = glm::dot(glm::vec3(frust.planes[i]), center) + frust.planes[i].w;
            if (d < -radius) { visible = false; break; }
        }
        if (!visible) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                          glm::vec3(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE));
        m_shader.setMat4("uModel", model);
        m_shader.setFloat("uIsWater", 0.0f);

        glBindVertexArray(g.vao);
        glDrawElements(GL_TRIANGLES, g.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    // ── Water pass (transparent, back-face rendered) ──
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (auto& [cc, g] : m_gpu) {
        if (g.wIndexCount == 0) continue;

        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                          glm::vec3(cc.x * Chunk::SIZE, 0, cc.z * Chunk::SIZE));
        m_shader.setMat4("uModel", model);
        m_shader.setFloat("uIsWater", 1.0f);

        glBindVertexArray(g.wVao);
        glDrawElements(GL_TRIANGLES, g.wIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
