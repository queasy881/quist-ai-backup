#pragma once
#include <glad/gl.h>
#include "rendering/Shader.h"
#include "rendering/TextureAtlas.h"
#include "world/World.h"
#include "player/Camera.h"

#include <unordered_map>

struct ChunkGPU {
    GLuint vao = 0, vbo = 0, ebo = 0;
    uint32_t indexCount = 0;
    GLuint wVao = 0, wVbo = 0, wEbo = 0;
    uint32_t wIndexCount = 0;
};

class ChunkRenderer {
public:
    ChunkRenderer();
    ~ChunkRenderer();

    void init(const std::string& vertPath, const std::string& fragPath);
    void uploadMesh(const ChunkCoord& cc, const ChunkMesh& mesh);
    void removeMesh(const ChunkCoord& cc);
    void render(const Camera& cam, float aspect, const TextureAtlas& atlas,
                const World& world, float daylight, const glm::vec3& fogColor);

    Shader& shader() { return m_shader; }

    // Expose GPU mesh map for deferred passes (shadow, gbuffer)
    const std::unordered_map<ChunkCoord, ChunkGPU, ChunkCoordHash>& gpuMeshes() const { return m_gpu; }

private:
    Shader m_shader;
    std::unordered_map<ChunkCoord, ChunkGPU, ChunkCoordHash> m_gpu;

    void uploadBuffers(GLuint& vao, GLuint& vbo, GLuint& ebo,
                       const void* vData, size_t vBytes,
                       const void* iData, size_t iBytes,
                       uint32_t& outCount);
};
