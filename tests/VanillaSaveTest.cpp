// Reading a world the real game wrote. Not part of the regular run: it needs a save made by the vanilla client
// (tools/scenarios/make_test_world.txt leaves one in run/cmp/vanilla/game/saves/New World) and the chunk hashes the
// MCP code computes for it (net.minecraft.world.SaveDump, written to run/cmp/vanilla_save_chunks.txt). Skipped
// when either is missing.

#include "init/Bootstrap.h"
#include "world/WorldSettings.h"
#include "world/chunk/storage/AnvilChunkLoader.h"
#include "world/storage/SaveFormat.h"
#include "world/storage/WorldInfo.h"

#include "RefWorld.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace mc;
using namespace mc::test;

TEST_CASE("A save written by the vanilla client loads like the original reads it")
{
    const std::filesystem::path run = std::filesystem::path(__FILE__).parent_path().parent_path() / "run" / "cmp";
    const std::filesystem::path saves = run / "vanilla" / "game" / "saves";
    std::ifstream expected(run / "vanilla_save_chunks.txt");
    if (!expected || !std::filesystem::exists(saves / "New World")) {
        MESSAGE("no vanilla test save; skipped");
        return;
    }
    Bootstrap::registerAll();
    AnvilSaveConverter format(saves);
    std::shared_ptr<WorldInfo> info(format.getWorldInfo("New World").release());
    REQUIRE(info);
    RefWorld world(info);
    AnvilChunkLoader loader(saves / "New World");

    std::string line;
    std::getline(expected, line);
    CHECK(line == "seed " + std::to_string(info->getSeed()) + " name " + info->getWorldName() + " spawn " + std::to_string(info->getSpawnX()) + " "
            + std::to_string(info->getSpawnY()) + " " + std::to_string(info->getSpawnZ()));
    std::ostringstream actual;
    for (int cx = -32; cx < 32; ++cx) {
        for (int cz = -32; cz < 32; ++cz) {
            if (auto chunk = loader.loadChunk(&world, cx, cz))
                actual << "chunk " << cx << " " << cz << " " << hash(*chunk) << " " << chunk->getInhabitedTime() << "\n";
        }
    }
    std::ostringstream rest;
    rest << expected.rdbuf();
    std::string expectedText = rest.str();
    std::erase(expectedText, '\r');
    CHECK(actual.str() == expectedText);
}
