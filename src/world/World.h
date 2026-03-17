#pragma once
#include "world/Chunk.h"
#include "world/ChunkMesh.h"
#include "generation/Heightmap.h"
#include "generation/BiomeGen.h"
#include "generation/CaveGen.h"
#include "generation/Noise.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <vector>

struct ChunkCoord {
    int x, z;
    bool operator==(const ChunkCoord& o) const { return x == o.x && z == o.z; }
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& c) const {
        std::size_t h1 = std::hash<int>()(c.x);
        std::size_t h2 = std::hash<int>()(c.z);
        return h1 ^ (h2 * 2654435761u);
    }
};

struct ChunkColumn {
    std::shared_ptr<Chunk>     chunk;
    std::unique_ptr<ChunkMesh> mesh;
    bool meshUploaded = false;
};

struct MeshTask {
    std::shared_ptr<Chunk> chunk;
    std::shared_ptr<Chunk> neighbours[4]; // -x, +x, -z, +z
};

class World {
public:
    static constexpr int RENDER_DISTANCE = 8;
    static constexpr int SEA_LEVEL       = 62;

    explicit World(uint32_t seed);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void update(const glm::vec3& playerPos);

    uint8_t getBlock(int wx, int wy, int wz) const;
    void    setBlock(int wx, int wy, int wz, uint8_t id);

    Chunk*       getChunk(int cx, int cz);
    const Chunk* getChunk(int cx, int cz) const;

    using ChunkMap = std::unordered_map<ChunkCoord, ChunkColumn, ChunkCoordHash>;
    const ChunkMap& getColumns() const { return m_columns; }

    uint32_t seed() const { return m_seed; }

    int getSurfaceHeight(int wx, int wz) const;

    // Retrieve and clear newly meshed chunk data (main-thread consumption)
    struct FinishedMesh {
        ChunkCoord coord;
        std::unique_ptr<ChunkMesh> mesh;
    };
    std::vector<FinishedMesh> takeFinishedMeshes();

private:
    void loadChunks(const glm::vec3& pos);
    void unloadChunks(const glm::vec3& pos);
    void generateTerrain(Chunk& chunk);
    void generateTrees(Chunk& chunk, int wx, int wz);
    void workerLoop();

    uint32_t  m_seed;
    Heightmap m_heightmap;
    BiomeGen  m_biomeGen;
    CaveGen   m_caveGen;
    Noise     m_treeNoise;

    ChunkMap  m_columns;
    mutable std::mutex m_mapMutex;

    // Worker thread-pool
    std::vector<std::thread>        m_workers;
    std::queue<std::shared_ptr<Chunk>> m_genQueue;
    std::queue<MeshTask>            m_meshQueue;
    std::mutex                      m_queueMutex;
    std::condition_variable         m_queueCV;
    std::atomic<bool>               m_running{true};

    // Completed meshes awaiting GPU upload
    std::vector<FinishedMesh>       m_finished;
    std::mutex                      m_finishedMutex;
};
