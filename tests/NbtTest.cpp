// NBT against the original game code: JsonToNBT literals parsed by MCP 1.8.9 (tests/reference/NbtReference.inc,
// from tools/reference/generate_game.ps1) must give the same toString(), hashCode() and file bytes here, and the
// Java bytes must read back to the same tag.

#include "core/Compression.h"
#include "nbt/CompressedStreamTools.h"
#include "nbt/JsonToNBT.h"

#include "reference/NbtReference.inc"

#include <doctest/doctest.h>

#include <string>
#include <vector>

using namespace mc;

namespace {

std::vector<uint8_t> unhex(std::string_view text)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < text.size(); i += 2)
        bytes.push_back(static_cast<uint8_t>(std::stoi(std::string(text.substr(i, 2)), nullptr, 16)));
    return bytes;
}

std::string unhexText(std::string_view text)
{
    const std::vector<uint8_t> bytes = unhex(text);
    return std::string(bytes.begin(), bytes.end());
}

} // namespace

TEST_CASE("JsonToNBT, toString, hashCode and writing match the original")
{
    for (const NbtCase& c : nbtCases) {
        const std::string input = unhexText(c.input);
        const std::string expected = unhexText(c.text);
        CAPTURE(input);
        if (expected.starts_with("!")) {
            try {
                JsonToNBT::getTagFromJson(input);
                FAIL("expected NBTException: " << expected);
            } catch (const NBTException& e) {
                CHECK(std::string("!") + e.what() == expected);
            }
            continue;
        }

        const auto tag = JsonToNBT::getTagFromJson(input);
        CHECK(tag->toString() == expected);
        CHECK(tag->hashCode() == c.hash);
        DataOutput output;
        CompressedStreamTools::write(*tag, output);
        CHECK(output.getBytes() == unhex(c.bytes));

        // Reading the original's bytes gives an equal tag in the same order.
        const std::vector<uint8_t> bytes = unhex(c.bytes);
        DataInput input2(bytes);
        NBTSizeTracker tracker = NBTSizeTracker::infinite();
        const auto read = CompressedStreamTools::read(input2, tracker);
        CHECK(read->toString() == expected);
        CHECK(read->equals(*tag));
        CHECK(read->copy()->toString() == expected);
    }
}

TEST_CASE("Compressed NBT round-trips through gzip")
{
    const auto tag = JsonToNBT::getTagFromJson("{Data:{LevelName:\"New World\",RandomSeed:-1234567890123L,GameRules:{doFireTick:\"true\"}},list:[1.5f,2.5f]}");
    const std::vector<uint8_t> gzip = CompressedStreamTools::writeCompressed(*tag);
    REQUIRE(gzip.size() > 10);
    CHECK(gzip[0] == 0x1F);
    CHECK(gzip[1] == 0x8B);
    const auto read = CompressedStreamTools::readCompressed(gzip);
    CHECK(read->equals(*tag));
    CHECK(read->getCompoundTag("Data")->getLong("RandomSeed") == -1234567890123LL);
    CHECK(read->getCompoundTag("Data")->getCompoundTag("GameRules")->getString("doFireTick") == "true");
    CHECK(read->getTagList("list", 5)->getFloatAt(1) == 2.5f);
    CHECK(read->getTagList("list", 3)->tagCount() == 0); // wrong element type gives a new empty list

    std::vector<uint8_t> corrupt = gzip;
    corrupt[corrupt.size() - 5] ^= 0xFF; // CRC
    CHECK_THROWS_AS(gzipDecompress(corrupt), IOException);
    CHECK(zlibDecompress(zlibCompress(gzip)) == gzip);
}

TEST_CASE("Compound accessors behave like the original")
{
    NBTTagCompound tag;
    tag.setFloat("f", -1.5f);
    tag.setDouble("d", 2.75);
    tag.setLong("l", 0x1234567890LL);
    tag.setString("s", "text");
    CHECK(tag.getInteger("f") == -2); // floor
    CHECK(tag.getByte("d") == 2);
    CHECK(tag.getInteger("l") == 0x34567890);
    CHECK(tag.getInteger("s") == 0); // not a number
    CHECK(tag.getString("f").empty());
    CHECK(tag.hasKey("f", 99));
    CHECK_FALSE(tag.hasKey("s", 99));
    CHECK(tag.getCompoundTag("missing")->hasNoTags());

    // getCompoundTag hands out the stored child, so edits through it show up in the parent.
    tag.setTag("child", std::make_shared<NBTTagCompound>());
    tag.getCompoundTag("child")->setInteger("x", 5);
    CHECK(tag.toString().find("child:{x:5}") != std::string::npos);

    NBTTagList list;
    list.appendTag(std::make_shared<NBTTagInt>(1));
    list.appendTag(std::make_shared<NBTTagString>("refused")); // wrong type: warning, ignored
    CHECK(list.tagCount() == 1);
    CHECK(list.getStringTagAt(0) == "1");
}
