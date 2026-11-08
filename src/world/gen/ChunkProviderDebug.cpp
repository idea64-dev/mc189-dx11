#include "world/gen/ChunkProviderDebug.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/World.h"
#include "world/biome/WorldChunkManager.h"
#include "world/chunk/Chunk.h"

namespace mc {

namespace {

struct DebugGrid {
    std::vector<const IBlockState*> states;
    int width;
    int height;
};

const DebugGrid& grid()
{
    static const DebugGrid g = [] {
        DebugGrid result;
        for (const Block* block : Block::blockRegistry.values()) {
            const auto valid = block->getBlockState().getValidStates();
            result.states.insert(result.states.end(), valid.begin(), valid.end());
        }
        result.width = MathHelper::ceiling_float_int(MathHelper::sqrt_float(static_cast<float>(result.states.size())));
        result.height = MathHelper::ceiling_float_int(static_cast<float>(result.states.size()) / static_cast<float>(result.width));
        return result;
    }();
    return g;
}

} // namespace

ChunkProviderDebug::~ChunkProviderDebug() = default;

const IBlockState* ChunkProviderDebug::func_177461_b(int x, int z)
{
    const DebugGrid& g = grid();
    if (x > 0 && z > 0 && x % 2 != 0 && z % 2 != 0) {
        x = x / 2;
        z = z / 2;
        if (x <= g.width && z <= g.height) {
            const int i = MathHelper::abs_int(x * g.width + z);
            if (i < static_cast<int>(g.states.size()))
                return g.states[static_cast<size_t>(i)];
        }
    }
    return nullptr;
}

std::unique_ptr<Chunk> ChunkProviderDebug::generateChunk(int x, int z)
{
    auto chunkprimer = std::make_unique<ChunkPrimer>();
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            const int k = x * 16 + i;
            const int l = z * 16 + j;
            chunkprimer->setBlockState(i, 60, j, Blocks::barrier->getDefaultState());
            if (const IBlockState* iblockstate = func_177461_b(k, l))
                chunkprimer->setBlockState(i, 70, j, iblockstate);
        }
    }
    auto chunk = std::make_unique<Chunk>(world, *chunkprimer, x, z);
    chunk->generateSkylightMap();
    std::vector<BiomeGenBase*> abiomegenbase;
    world->getWorldChunkManager()->loadBlockGeneratorData(abiomegenbase, x * 16, z * 16, 16, 16);
    std::array<uint8_t, 256>& abyte = chunk->getBiomeArray();
    for (size_t i1 = 0; i1 < abyte.size(); ++i1)
        abyte[i1] = static_cast<uint8_t>(abiomegenbase[i1]->biomeID);
    chunk->generateSkylightMap();
    return chunk;
}

Chunk* ChunkProviderDebug::provideChunk(int x, int z)
{
    lastProvided = generateChunk(x, z);
    return lastProvided.get();
}

std::vector<BiomeGenBase::SpawnListEntry> ChunkProviderDebug::getPossibleCreatures(EnumCreatureType creatureType, const BlockPos& pos)
{
    return world->getBiomeGenForCoords(pos)->getSpawnableList(creatureType);
}

} // namespace mc
