// Chunk storage and the light engine against the original: tests/reference/world.txt is written by the MCP code
// (tools/reference/net/minecraft/world/WorldDump.java), which builds a client-side world of synthetic chunks and
// edits it by a fixed recipe. This follows the same recipe and compares the chunk hashes after every step, the
// light queries at sample positions, the chunk NBT and level.dat.

#include "block/AllBlocks.h"
#include "core/DataStream.h"
#include "core/JavaString.h"
#include "init/Blocks.h"
#include "init/Bootstrap.h"
#include "nbt/CompressedStreamTools.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/WorldSettings.h"
#include "world/chunk/Chunk.h"
#include "world/chunk/IChunkProvider.h"
#include "world/chunk/storage/AnvilChunkLoader.h"
#include "world/storage/WorldInfo.h"

#include "RefWorld.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace mc;
using namespace mc::test;

namespace {

std::vector<const IBlockState*> palette()
{
    return { Blocks::air->getDefaultState(), Blocks::stone->getDefaultState(), Blocks::dirt->getDefaultState(), Blocks::glass->getDefaultState(),
        Blocks::leaves->getDefaultState(), Blocks::water->getDefaultState(), Blocks::glowstone->getDefaultState(), Blocks::torch->getDefaultState(),
        Blocks::ice->getDefaultState(), Blocks::stone_slab->getDefaultState(), Blocks::oak_stairs->getDefaultState(), Blocks::lava->getDefaultState(),
        Blocks::lit_pumpkin->getDefaultState(), Blocks::web->getDefaultState(), Blocks::farmland->getDefaultState(),
        Blocks::redstone_torch->getDefaultState() };
}

std::unique_ptr<ChunkPrimer> primer(int cx, int cz, const std::vector<const IBlockState*>& pal)
{
    Random r(java::add(java::add(java::mul(static_cast<int64_t>(cx), static_cast<int64_t>(341873128712LL)),
                           java::mul(static_cast<int64_t>(cz), static_cast<int64_t>(132897987541LL))),
        static_cast<int64_t>(12345LL)));
    auto p = std::make_unique<ChunkPrimer>();
    for (int x = 0; x < 16; ++x) {
        for (int z = 0; z < 16; ++z) {
            const int h = 40 + r.nextInt(24);
            for (int y = 0; y < h; ++y) {
                const int v = r.nextInt(100);
                const int idx = v < 70 ? 1 : v < 75 ? 0 : v < 78 ? 3 : v < 81 ? 4 : v < 84 ? 5 : v < 85 ? 6 : v < 86 ? 7 : v < 88 ? 8 : v < 90 ? 9 : v < 92 ? 10
                    : v < 93 ? 11 : v < 94 ? 12 : v < 95 ? 13 : v < 97 ? 2 : v < 98 ? 14 : 15;
                p->setBlockState(x, y, z, pal[static_cast<size_t>(idx)]);
            }
            if (r.nextInt(8) == 0) {
                const int y = h + 5 + r.nextInt(20);
                p->setBlockState(x, y, z, pal[static_cast<size_t>(r.nextInt(static_cast<int>(pal.size())))]);
            }
        }
    }
    return p;
}

std::string dumpChunks(RefWorld& world, const std::string& step)
{
    std::string s = "step " + step;
    for (int cx = -3; cx <= 3; ++cx) {
        for (int cz = -3; cz <= 3; ++cz)
            s += " " + std::to_string(hash(*world.getChunkFromChunkCoords(cx, cz)));
    }
    return s;
}

std::vector<std::string> referenceLines()
{
    std::ifstream file(std::filesystem::path(__FILE__).parent_path() / "reference" / "world.txt");
    REQUIRE(file);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

} // namespace

TEST_CASE("Chunk storage and the light engine match the original")
{
    Bootstrap::registerAll();
    const std::vector<std::string> expected = referenceLines();
    std::vector<std::string> actual;

    auto info = std::make_shared<WorldInfo>(WorldSettings(12345LL, GameType::SURVIVAL, true, false, &WorldType::DEFAULT).enableCommands(), "Ref");
    RefWorld world(info);
    const auto pal = palette();
    actual.push_back("skylightSubtracted " + std::to_string(world.getSkylightSubtracted()));

    for (int cx = -3; cx <= 3; ++cx) {
        for (int cz = -3; cz <= 3; ++cz) {
            auto chunk = std::make_unique<Chunk>(&world, *primer(cx, cz, pal), cx, cz);
            chunk->generateSkylightMap();
            world.chunks().chunks[ChunkCoordIntPair::chunkXZ2Int(cx, cz)] = std::move(chunk);
        }
    }
    actual.push_back(dumpChunks(world, "generated"));

    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz)
            world.getChunkFromChunkCoords(cx, cz)->func_150809_p();
    }
    actual.push_back(dumpChunks(world, "lightPopulated"));

    Random ops(4242);
    std::string results = "set";
    for (int i = 0; i < 300; ++i) {
        const int x = ops.nextInt(48) - 24;
        const int y = ops.nextInt(80) + 10;
        const int z = ops.nextInt(48) - 24;
        const IBlockState* state = pal[static_cast<size_t>(ops.nextInt(static_cast<int>(pal.size())))];
        results += world.setBlockState(BlockPos(x, y, z), state, 2) ? '1' : '0';
    }
    actual.push_back(results);
    actual.push_back(dumpChunks(world, "edited"));

    Chunk* center = world.getChunkFromChunkCoords(0, 0);
    center->resetRelightChecks();
    for (int i = 0; i < 600; ++i)
        center->enqueueRelightChecks();
    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz)
            world.getChunkFromChunkCoords(cx, cz)->func_150804_b(false);
    }
    actual.push_back(dumpChunks(world, "rechecked"));

    Random samples(777);
    for (int i = 0; i < 200; ++i) {
        const int x = samples.nextInt(64) - 32;
        const int y = samples.nextInt(110);
        const int z = samples.nextInt(64) - 32;
        const BlockPos pos(x, y, z);
        std::ostringstream line;
        line << "sample " << x << " " << y << " " << z << " light=" << world.getLight(pos) << " neighbors=" << world.getLightFromNeighbors(pos)
             << " combined=" << world.getCombinedLight(pos, 0) << " sky=" << world.getLightFor(EnumSkyBlock::SKY, pos)
             << " block=" << world.getLightFor(EnumSkyBlock::BLOCK, pos) << " seesSky=" << java::toString(world.canSeeSky(pos))
             << " blockSeesSky=" << java::toString(world.canBlockSeeSky(pos)) << " brightness=" << java::toString(world.getLightBrightness(pos))
             << " precipitation=" << world.getPrecipitationHeight(pos).getY() << " top=" << world.getTopSolidOrLiquidBlock(pos).getY()
             << " height=" << world.getHeight(pos).getY() << " lowest=" << world.getChunksLowestHorizon(x, z);
        actual.push_back(line.str());
    }

    AnvilChunkLoader loader("unused");
    for (int c = 0; c < 2; ++c) {
        const auto level = loader.writeChunkToNBT(*world.getChunkFromChunkCoords(c, -c), world);
        DataOutput bytes;
        CompressedStreamTools::write(*level, bytes);
        int32_t h = 0;
        for (const uint8_t b : bytes.getBytes())
            h = java::add(java::mul(h, 31), static_cast<int32_t>(static_cast<int8_t>(b)));
        actual.push_back("chunkNBT " + std::to_string(c) + " " + std::to_string(bytes.getBytes().size()) + " " + std::to_string(h));

        // And back: the chunk read from that NBT writes the same NBT again.
        const auto reread = loader.readChunkFromNBT(&world, *level);
        CHECK(hash(*reread) != 0);
        DataOutput again;
        CompressedStreamTools::write(*loader.writeChunkToNBT(*reread, world), again);
        CHECK(again.getBytes() == bytes.getBytes());
    }

    const auto levelDat = info->getNBTTagCompound();
    levelDat->removeTag("LastPlayed");
    actual.push_back("levelDat " + levelDat->toString());

    REQUIRE(actual.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        CAPTURE(i);
        CHECK(actual[i] == expected[i]);
    }
}
