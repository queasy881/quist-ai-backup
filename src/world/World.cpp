#include "world/World.h"
#include "world/BlockRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

World::World(uint32_t seed)
    : m_seed(seed)
    , m_heightmap(seed)
    , m_biomeGen(seed)
    , m_caveGen(seed)
    , m_treeNoise(seed + 9999)
{
    unsigned hw = std::max(2u, std::thread::hardware_concurrency());
    for (unsigned i = 0; i < hw; ++i)
        m_workers.emplace_back(&World::workerLoop, this);
}

World::~World() {
    m_running = false;
    m_queueCV.notify_all();
    for (auto& t : m_workers) if (t.joinable()) t.join();
}

// ── worker loop ──────────────────────────────────────────────

void World::workerLoop() {
    while (m_running) {
        enum class TaskKind { None, Gen, Mesh };
        TaskKind kind = TaskKind::None;
        std::shared_ptr<Chunk> genChunk;
        MeshTask meshTask{};

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [&]{
                return !m_running || !m_genQueue.empty() || !m_meshQueue.empty();
            });
            if (!m_running) return;

            if (!m_genQueue.empty()) {
                genChunk = m_genQueue.front(); m_genQueue.pop();
                kind = TaskKind::Gen;
            } else if (!m_meshQueue.empty()) {
                meshTask = std::move(m_meshQueue.front()); m_meshQueue.pop();
                kind = TaskKind::Mesh;
            }
        }

        if (kind == TaskKind::Gen) {
            generateTerrain(*genChunk);
            genChunk->setState(Chunk::State::Generated);
        }
        else if (kind == TaskKind::Mesh) {
            meshTask.chunk->setState(Chunk::State::Meshing);
            auto mesh = std::make_unique<ChunkMesh>();
            mesh->build(*meshTask.chunk,
                        meshTask.neighbours[0].get(),
                        meshTask.neighbours[1].get(),
                        meshTask.neighbours[2].get(),
                        meshTask.neighbours[3].get());
            meshTask.chunk->setState(Chunk::State::Meshed);

            ChunkCoord cc{ meshTask.chunk->getX(), meshTask.chunk->getZ() };
            {
                std::lock_guard<std::mutex> lg(m_finishedMutex);
                m_finished.push_back({ cc, std::move(mesh) });
            }
        }
    }
}

// ── update (main thread) ────────────────────────────────────

void World::update(const glm::vec3& playerPos) {
    loadChunks(playerPos);

    // Queue meshing for generated chunks whose 4 neighbours are also generated
    {
        std::lock_guard<std::mutex> lm(m_mapMutex);
        for (auto& [coord, col] : m_columns) {
            if (!col.chunk) continue;
            auto st = col.chunk->getState();
            bool needsMesh = (st == Chunk::State::Generated)
                          || (st == Chunk::State::Ready && col.chunk->isDirty());
            if (!needsMesh) continue;

            auto find = [&](int cx, int cz) -> std::shared_ptr<Chunk> {
                auto it = m_columns.find({cx, cz});
                if (it == m_columns.end()) return nullptr;
                if (it->second.chunk->getState() < Chunk::State::Generated) return nullptr;
                return it->second.chunk;
            };

            auto nxn = find(coord.x - 1, coord.z);
            auto nxp = find(coord.x + 1, coord.z);
            auto nzn = find(coord.x, coord.z - 1);
            auto nzp = find(coord.x, coord.z + 1);
            if (!nxn || !nxp || !nzn || !nzp) continue;

            col.chunk->setDirty(false);
            col.chunk->setState(Chunk::State::Meshing);

            MeshTask task;
            task.chunk = col.chunk;
            task.neighbours[0] = nxn;
            task.neighbours[1] = nxp;
            task.neighbours[2] = nzn;
            task.neighbours[3] = nzp;

            {
                std::lock_guard<std::mutex> lq(m_queueMutex);
                m_meshQueue.push(std::move(task));
            }
            m_queueCV.notify_one();
        }
    }

    // Install finished meshes
    auto finished = takeFinishedMeshes();
    {
        std::lock_guard<std::mutex> lm(m_mapMutex);
        for (auto& fm : finished) {
            auto it = m_columns.find(fm.coord);
            if (it != m_columns.end()) {
                it->second.mesh = std::move(fm.mesh);
                it->second.meshUploaded = false;
                it->second.chunk->setState(Chunk::State::Ready);
            }
        }
    }

    unloadChunks(playerPos);
}

std::vector<World::FinishedMesh> World::takeFinishedMeshes() {
    std::lock_guard<std::mutex> lg(m_finishedMutex);
    std::vector<FinishedMesh> out;
    std::swap(out, m_finished);
    return out;
}

// ── chunk loading ────────────────────────────────────────────

void World::loadChunks(const glm::vec3& pos) {
    int pcx = static_cast<int>(std::floor(pos.x)) >> 4;
    int pcz = static_cast<int>(std::floor(pos.z)) >> 4;

    // Spiral load
    for (int r = 0; r <= RENDER_DISTANCE; ++r) {
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                if (std::abs(dx) != r && std::abs(dz) != r) continue; // shell only
                int cx = pcx + dx, cz = pcz + dz;
                ChunkCoord cc{cx, cz};

                std::lock_guard<std::mutex> lm(m_mapMutex);
                if (m_columns.count(cc)) continue;

                auto chunk = std::make_shared<Chunk>(cx, cz);
                chunk->setState(Chunk::State::Generating);
                m_columns[cc] = ChunkColumn{ chunk, nullptr, false };

                {
                    std::lock_guard<std::mutex> lq(m_queueMutex);
                    m_genQueue.push(chunk);
                }
                m_queueCV.notify_one();
            }
        }
    }
}

void World::unloadChunks(const glm::vec3& pos) {
    int pcx = static_cast<int>(std::floor(pos.x)) >> 4;
    int pcz = static_cast<int>(std::floor(pos.z)) >> 4;
    int limit = RENDER_DISTANCE + 2;

    std::lock_guard<std::mutex> lm(m_mapMutex);
    for (auto it = m_columns.begin(); it != m_columns.end(); ) {
        int dx = it->first.x - pcx;
        int dz = it->first.z - pcz;
        if (std::abs(dx) > limit || std::abs(dz) > limit)
            it = m_columns.erase(it);
        else
            ++it;
    }
}

// ── terrain generation (called on worker) ────────────────────

void World::generateTerrain(Chunk& chunk) {
    int wx = chunk.getX() * Chunk::SIZE;
    int wz = chunk.getZ() * Chunk::SIZE;

    for (int x = 0; x < Chunk::SIZE; ++x) {
        for (int z = 0; z < Chunk::SIZE; ++z) {
            float fx = static_cast<float>(wx + x);
            float fz = static_cast<float>(wz + z);

            BiomeData biome = m_biomeGen.getBiome(fx, fz);
            int height = m_heightmap.getHeight(fx, fz,
                                               biome.heightMultiplier,
                                               biome.roughness);

            for (int y = 0; y < Chunk::HEIGHT; ++y) {
                uint8_t block;

                if (y == 0) {
                    block = static_cast<uint8_t>(BlockID::Stone); // bedrock
                } else if (y < height - biome.surfaceDepth) {
                    block = y < 16
                        ? static_cast<uint8_t>(BlockID::DeepStone)
                        : static_cast<uint8_t>(BlockID::Stone);
                } else if (y < height) {
                    block = biome.subSurfaceBlock;
                } else if (y == height) {
                    block = biome.surfaceBlock;
                } else if (y <= SEA_LEVEL && y > height) {
                    block = static_cast<uint8_t>(BlockID::Water);
                } else {
                    block = static_cast<uint8_t>(BlockID::Air);
                }

                chunk.setBlock(x, y, z, block);
            }

            // Carve caves
            for (int y = 2; y < height; ++y) {
                if (m_caveGen.isCave(wx + x, y, wz + z)) {
                    if (y <= SEA_LEVEL && y > height - 5)
                        chunk.setBlock(x, y, z, static_cast<uint8_t>(BlockID::Water));
                    else
                        chunk.setBlock(x, y, z, static_cast<uint8_t>(BlockID::Air));
                }
            }
        }
    }

    // Trees
    generateTrees(chunk, wx, wz);
}

void World::generateTrees(Chunk& chunk, int wx, int wz) {
    for (int x = 2; x < Chunk::SIZE - 2; ++x) {
        for (int z = 2; z < Chunk::SIZE - 2; ++z) {
            float fx = static_cast<float>(wx + x);
            float fz = static_cast<float>(wz + z);

            BiomeData biome = m_biomeGen.getBiome(fx, fz);
            if (biome.type != BiomeType::Forest && biome.type != BiomeType::Plains)
                continue;

            float treeVal = m_treeNoise.perlin2D(fx * 0.5f, fz * 0.5f);
            float threshold = (biome.type == BiomeType::Forest) ? 0.35f : 0.55f;
            if (treeVal < threshold) continue;

            // Find surface
            int sy = -1;
            for (int y = Chunk::HEIGHT - 1; y > SEA_LEVEL; --y) {
                uint8_t b = chunk.getBlock(x, y, z);
                if (b == static_cast<uint8_t>(BlockID::Grass) ||
                    b == static_cast<uint8_t>(BlockID::Snow)) {
                    sy = y; break;
                }
            }
            if (sy < 0 || sy > Chunk::HEIGHT - 10) continue;

            // Trunk: 4-6 blocks tall
            int trunkH = 4 + (static_cast<int>(std::abs(treeVal * 1000)) % 3);
            for (int ty = 1; ty <= trunkH; ++ty)
                chunk.setBlock(x, sy + ty, z, static_cast<uint8_t>(BlockID::Wood));

            // Leaves sphere
            int top = sy + trunkH;
            for (int ly = -1; ly <= 2; ++ly) {
                int r = (ly <= 0) ? 2 : 1;
                for (int lx = -r; lx <= r; ++lx) {
                    for (int lz = -r; lz <= r; ++lz) {
                        if (lx == 0 && lz == 0 && ly <= 0) continue; // trunk space
                        int bx = x + lx, by = top + ly, bz = z + lz;
                        if (bx < 0 || bx >= Chunk::SIZE || bz < 0 || bz >= Chunk::SIZE) continue;
                        if (by < 0 || by >= Chunk::HEIGHT) continue;
                        if (chunk.getBlock(bx, by, bz) == 0)
                            chunk.setBlock(bx, by, bz, static_cast<uint8_t>(BlockID::Leaves));
                    }
                }
            }
        }
    }
}

// ── surface height query ─────────────────────────────────────

int World::getSurfaceHeight(int wx, int wz) const {
    BiomeData bd = m_biomeGen.getBiome(static_cast<float>(wx), static_cast<float>(wz));
    int h = m_heightmap.getHeight(static_cast<float>(wx), static_cast<float>(wz),
                                   bd.heightMultiplier, bd.roughness);
    return h;
}

// ── block access ─────────────────────────────────────────────

uint8_t World::getBlock(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= Chunk::HEIGHT) return 0;
    int cx = (wx >= 0 ? wx : wx - 15) / Chunk::SIZE;
    int cz = (wz >= 0 ? wz : wz - 15) / Chunk::SIZE;
    int lx = ((wx % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;
    int lz = ((wz % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;

    std::lock_guard<std::mutex> lm(m_mapMutex);
    auto it = m_columns.find({cx, cz});
    if (it == m_columns.end()) return 0;
    return it->second.chunk->getBlock(lx, wy, lz);
}

void World::setBlock(int wx, int wy, int wz, uint8_t id) {
    if (wy < 0 || wy >= Chunk::HEIGHT) return;
    int cx = (wx >= 0 ? wx : wx - 15) / Chunk::SIZE;
    int cz = (wz >= 0 ? wz : wz - 15) / Chunk::SIZE;
    int lx = ((wx % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;
    int lz = ((wz % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;

    std::lock_guard<std::mutex> lm(m_mapMutex);
    auto it = m_columns.find({cx, cz});
    if (it == m_columns.end()) return;
    it->second.chunk->setBlock(lx, wy, lz, id);

    // Mark neighbouring chunks dirty if at edge
    auto markDirty = [&](int ncx, int ncz) {
        auto nit = m_columns.find({ncx, ncz});
        if (nit != m_columns.end())
            nit->second.chunk->setDirty(true);
    };
    if (lx == 0)                markDirty(cx - 1, cz);
    if (lx == Chunk::SIZE - 1)  markDirty(cx + 1, cz);
    if (lz == 0)                markDirty(cx, cz - 1);
    if (lz == Chunk::SIZE - 1)  markDirty(cx, cz + 1);
}

Chunk* World::getChunk(int cx, int cz) {
    std::lock_guard<std::mutex> lm(m_mapMutex);
    auto it = m_columns.find({cx, cz});
    return it != m_columns.end() ? it->second.chunk.get() : nullptr;
}

const Chunk* World::getChunk(int cx, int cz) const {
    std::lock_guard<std::mutex> lm(m_mapMutex);
    auto it = m_columns.find({cx, cz});
    return it != m_columns.end() ? it->second.chunk.get() : nullptr;
}
