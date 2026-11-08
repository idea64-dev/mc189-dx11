// Values behind the world screens against Java 8 (taken from the MCP code and the JRE): the customized-world JSON
// (Gson lenient parsing, the Serializer's stop-at-first-bad-field, fixedBiome remapping, hashCode), String.format's
// %f/%d and SimpleDateFormat in a few locales.

#include "core/JavaLocale.h"
#include "core/JavaString.h"
#include "world/gen/ChunkProviderSettings.h"

#include <doctest/doctest.h>

using namespace mc;

namespace {

const std::string kTail = R"("dirtSize":33,"dirtCount":10,"dirtMinHeight":0,"dirtMaxHeight":256,"gravelSize":33,"gravelCount":8,"gravelMinHeight":0,"gravelMaxHeight":256,"graniteSize":33,"graniteCount":10,"graniteMinHeight":0,"graniteMaxHeight":80,"dioriteSize":33,"dioriteCount":10,"dioriteMinHeight":0,"dioriteMaxHeight":80,"andesiteSize":33,"andesiteCount":10,"andesiteMinHeight":0,"andesiteMaxHeight":80,"coalSize":17,"coalCount":20,"coalMinHeight":0,"coalMaxHeight":128,"ironSize":9,"ironCount":20,"ironMinHeight":0,"ironMaxHeight":64,"goldSize":9,"goldCount":2,"goldMinHeight":0,"goldMaxHeight":32,"redstoneSize":8,"redstoneCount":8,"redstoneMinHeight":0,"redstoneMaxHeight":16,"diamondSize":8,"diamondCount":1,"diamondMinHeight":0,"diamondMaxHeight":16,"lapisSize":7,"lapisCount":1,"lapisCenterHeight":16,"lapisSpread":16})";

} // namespace

TEST_CASE("Customized world settings JSON matches Gson")
{
    using Factory = ChunkProviderSettings::Factory;
    const Factory defaults;
    CHECK(defaults.toString()
        == R"({"coordinateScale":684.412,"heightScale":684.412,"lowerLimitScale":512.0,"upperLimitScale":512.0,"depthNoiseScaleX":200.0,"depthNoiseScaleZ":200.0,"depthNoiseScaleExponent":0.5,"mainNoiseScaleX":80.0,"mainNoiseScaleY":160.0,"mainNoiseScaleZ":80.0,"baseSize":8.5,"stretchY":12.0,"biomeDepthWeight":1.0,"biomeDepthOffset":0.0,"biomeScaleWeight":1.0,"biomeScaleOffset":0.0,"seaLevel":63,"useCaves":true,"useDungeons":true,"dungeonChance":8,"useStrongholds":true,"useVillages":true,"useMineShafts":true,"useTemples":true,"useMonuments":true,"useRavines":true,"useWaterLakes":true,"waterLakeChance":4,"useLavaLakes":true,"lavaLakeChance":80,"useLavaOceans":false,"fixedBiome":-1,"biomeSize":4,"riverSize":4,)"
            + kTail);

    // Lenient syntax; the string seaLevel stops the deserializer, so useCaves keeps its default.
    const Factory a = Factory::jsonToFactory("{coordinateScale=12.5;seaLevel:'x', useCaves:false}");
    CHECK(a.coordinateScale == 12.5f);
    CHECK(a.seaLevel == 63);
    CHECK(a.useCaves);
    CHECK(a.hashCode() == 1246096237);

    // fixedBiome above the Nether id skips the two unused ids; ints truncate; exponents parse.
    const Factory b = Factory::jsonToFactory("{fixedBiome:20, riverSize:2.9, baseSize:1e2}");
    CHECK(b.fixedBiome == 22);
    CHECK(b.riverSize == 2);
    CHECK(b.baseSize == 100.0f);
    CHECK(b.hashCode() == 939586724);

    CHECK(Factory::jsonToFactory("not json {") == defaults);
    CHECK(Factory::jsonToFactory("[1, 2]") == defaults);
    CHECK(Factory::jsonToFactory("{} trailing") == defaults);
    CHECK(Factory::jsonToFactory(defaults.toString()) == defaults);
}

TEST_CASE("String.format %f and %d and SimpleDateFormat follow the Java 8 locale data")
{
    // Java: String.format(locale, "%5.3f|%2.3f|%d", 1234.5678f, 0.5f, 42)
    CHECK(java::formatFixed("ru_RU", 1234.5678f, 5, 3) == "1234,568");
    CHECK(java::formatFixed("en_US", 1234.5678f, 5, 3) == "1234.568");
    CHECK(java::formatFixed("de", 0.5f, 2, 3) == "0,500");
    CHECK(java::formatFixed("hi_IN", 0.5f, 2, 3) == "०.५००");
    // Half up on the shortest digits: 2.0005 is 2.000499999... in binary, yet Java prints 2.001.
    CHECK(java::formatFixed("en_US", 2.0005, 5, 3) == "2.001");
    CHECK(java::formatFixed("en_US", 0.0005, 1, 3) == "0.001");
    CHECK(java::formatFixed("en_US", 1.5, 8, 3) == "   1.500");
    CHECK(java::formatFixed("en_US", 999.9996, 1, 3) == "1000.000");

    CHECK(java::formatDateTime("ru_RU", 2026, 11, 8, 7, 32) == "08.11.26 7:32");
    CHECK(java::formatDateTime("en_US", 2026, 11, 8, 19, 5) == "11/8/26 7:05 PM");
    CHECK(java::formatDateTime("en_GB", 2026, 1, 2, 0, 5) == "02/01/26 00:05");
    CHECK(java::formatDateTime("th_TH", 2026, 11, 8, 7, 32) == "8/11/2569, 7:32 น.");
    CHECK(java::formatDateTime("xx_YY", 2026, 11, 8, 7, 32) == "11/8/26 7:32 AM"); // root data
}
