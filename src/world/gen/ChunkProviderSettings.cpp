#include "world/gen/ChunkProviderSettings.h"

#include "core/GsonLenient.h"
#include "core/Java.h"
#include "core/JavaString.h"
#include "world/biome/BiomeNames.h"

#include <stdexcept>
#include <variant>

namespace mc {

namespace {

using Factory = ChunkProviderSettings::Factory;
using Member = std::variant<float Factory::*, int Factory::*, bool Factory::*>;

struct Field {
    const char* name;
    Member member;
};

// In the order the Serializer reads and writes them.
const Field kFields[] {
    { "coordinateScale", &Factory::coordinateScale },
    { "heightScale", &Factory::heightScale },
    { "lowerLimitScale", &Factory::lowerLimitScale },
    { "upperLimitScale", &Factory::upperLimitScale },
    { "depthNoiseScaleX", &Factory::depthNoiseScaleX },
    { "depthNoiseScaleZ", &Factory::depthNoiseScaleZ },
    { "depthNoiseScaleExponent", &Factory::depthNoiseScaleExponent },
    { "mainNoiseScaleX", &Factory::mainNoiseScaleX },
    { "mainNoiseScaleY", &Factory::mainNoiseScaleY },
    { "mainNoiseScaleZ", &Factory::mainNoiseScaleZ },
    { "baseSize", &Factory::baseSize },
    { "stretchY", &Factory::stretchY },
    { "biomeDepthWeight", &Factory::biomeDepthWeight },
    { "biomeDepthOffset", &Factory::biomeDepthOffset },
    { "biomeScaleWeight", &Factory::biomeScaleWeight },
    { "biomeScaleOffset", &Factory::biomeScaleOffset },
    { "seaLevel", &Factory::seaLevel },
    { "useCaves", &Factory::useCaves },
    { "useDungeons", &Factory::useDungeons },
    { "dungeonChance", &Factory::dungeonChance },
    { "useStrongholds", &Factory::useStrongholds },
    { "useVillages", &Factory::useVillages },
    { "useMineShafts", &Factory::useMineShafts },
    { "useTemples", &Factory::useTemples },
    { "useMonuments", &Factory::useMonuments },
    { "useRavines", &Factory::useRavines },
    { "useWaterLakes", &Factory::useWaterLakes },
    { "waterLakeChance", &Factory::waterLakeChance },
    { "useLavaLakes", &Factory::useLavaLakes },
    { "lavaLakeChance", &Factory::lavaLakeChance },
    { "useLavaOceans", &Factory::useLavaOceans },
    { "fixedBiome", &Factory::fixedBiome },
    { "biomeSize", &Factory::biomeSize },
    { "riverSize", &Factory::riverSize },
    { "dirtSize", &Factory::dirtSize },
    { "dirtCount", &Factory::dirtCount },
    { "dirtMinHeight", &Factory::dirtMinHeight },
    { "dirtMaxHeight", &Factory::dirtMaxHeight },
    { "gravelSize", &Factory::gravelSize },
    { "gravelCount", &Factory::gravelCount },
    { "gravelMinHeight", &Factory::gravelMinHeight },
    { "gravelMaxHeight", &Factory::gravelMaxHeight },
    { "graniteSize", &Factory::graniteSize },
    { "graniteCount", &Factory::graniteCount },
    { "graniteMinHeight", &Factory::graniteMinHeight },
    { "graniteMaxHeight", &Factory::graniteMaxHeight },
    { "dioriteSize", &Factory::dioriteSize },
    { "dioriteCount", &Factory::dioriteCount },
    { "dioriteMinHeight", &Factory::dioriteMinHeight },
    { "dioriteMaxHeight", &Factory::dioriteMaxHeight },
    { "andesiteSize", &Factory::andesiteSize },
    { "andesiteCount", &Factory::andesiteCount },
    { "andesiteMinHeight", &Factory::andesiteMinHeight },
    { "andesiteMaxHeight", &Factory::andesiteMaxHeight },
    { "coalSize", &Factory::coalSize },
    { "coalCount", &Factory::coalCount },
    { "coalMinHeight", &Factory::coalMinHeight },
    { "coalMaxHeight", &Factory::coalMaxHeight },
    { "ironSize", &Factory::ironSize },
    { "ironCount", &Factory::ironCount },
    { "ironMinHeight", &Factory::ironMinHeight },
    { "ironMaxHeight", &Factory::ironMaxHeight },
    { "goldSize", &Factory::goldSize },
    { "goldCount", &Factory::goldCount },
    { "goldMinHeight", &Factory::goldMinHeight },
    { "goldMaxHeight", &Factory::goldMaxHeight },
    { "redstoneSize", &Factory::redstoneSize },
    { "redstoneCount", &Factory::redstoneCount },
    { "redstoneMinHeight", &Factory::redstoneMinHeight },
    { "redstoneMaxHeight", &Factory::redstoneMaxHeight },
    { "diamondSize", &Factory::diamondSize },
    { "diamondCount", &Factory::diamondCount },
    { "diamondMinHeight", &Factory::diamondMinHeight },
    { "diamondMaxHeight", &Factory::diamondMaxHeight },
    { "lapisSize", &Factory::lapisSize },
    { "lapisCount", &Factory::lapisCount },
    { "lapisCenterHeight", &Factory::lapisCenterHeight },
    { "lapisSpread", &Factory::lapisSpread },
};

} // namespace

void ChunkProviderSettings::Factory::func_177863_a()
{
    coordinateScale = 684.412f;
    heightScale = 684.412f;
    upperLimitScale = 512.0f;
    lowerLimitScale = 512.0f;
    depthNoiseScaleX = 200.0f;
    depthNoiseScaleZ = 200.0f;
    depthNoiseScaleExponent = 0.5f;
    mainNoiseScaleX = 80.0f;
    mainNoiseScaleY = 160.0f;
    mainNoiseScaleZ = 80.0f;
    baseSize = 8.5f;
    stretchY = 12.0f;
    biomeDepthWeight = 1.0f;
    biomeDepthOffset = 0.0f;
    biomeScaleWeight = 1.0f;
    biomeScaleOffset = 0.0f;
    seaLevel = 63;
    useCaves = true;
    useDungeons = true;
    dungeonChance = 8;
    useStrongholds = true;
    useVillages = true;
    useMineShafts = true;
    useTemples = true;
    useMonuments = true;
    useRavines = true;
    useWaterLakes = true;
    waterLakeChance = 4;
    useLavaLakes = true;
    lavaLakeChance = 80;
    useLavaOceans = false;
    fixedBiome = -1;
    biomeSize = 4;
    riverSize = 4;
    dirtSize = 33;
    dirtCount = 10;
    dirtMinHeight = 0;
    dirtMaxHeight = 256;
    gravelSize = 33;
    gravelCount = 8;
    gravelMinHeight = 0;
    gravelMaxHeight = 256;
    graniteSize = 33;
    graniteCount = 10;
    graniteMinHeight = 0;
    graniteMaxHeight = 80;
    dioriteSize = 33;
    dioriteCount = 10;
    dioriteMinHeight = 0;
    dioriteMaxHeight = 80;
    andesiteSize = 33;
    andesiteCount = 10;
    andesiteMinHeight = 0;
    andesiteMaxHeight = 80;
    coalSize = 17;
    coalCount = 20;
    coalMinHeight = 0;
    coalMaxHeight = 128;
    ironSize = 9;
    ironCount = 20;
    ironMinHeight = 0;
    ironMaxHeight = 64;
    goldSize = 9;
    goldCount = 2;
    goldMinHeight = 0;
    goldMaxHeight = 32;
    redstoneSize = 8;
    redstoneCount = 8;
    redstoneMinHeight = 0;
    redstoneMaxHeight = 16;
    diamondSize = 8;
    diamondCount = 1;
    diamondMinHeight = 0;
    diamondMaxHeight = 16;
    lapisSize = 7;
    lapisCount = 1;
    lapisCenterHeight = 16;
    lapisSpread = 16;
}

ChunkProviderSettings::Factory ChunkProviderSettings::Factory::jsonToFactory(std::string_view json)
{
    if (json.empty())
        return Factory();
    try {
        const auto element = gson::parse(json);
        // Java: the deserializer's getAsJsonObject() throws for anything else (and null yields a null factory,
        // which the callers would not survive -- defaults here).
        if (!element->isJsonObject())
            return Factory();
        Factory factory;
        // The Serializer stops at the first field it can't read and keeps what it has.
        try {
            for (const Field& field : kFields) {
                if (!element->has(field.name))
                    continue;
                const gson::JsonElement& value = element->get(field.name);
                if (std::holds_alternative<bool Factory::*>(field.member)) {
                    if (!value.isJsonPrimitive())
                        throw gson::JsonSyntaxException(field.name);
                    factory.*std::get<bool Factory::*>(field.member) = value.getAsBoolean();
                } else {
                    if (!value.isJsonPrimitive() || !value.isNumber())
                        throw gson::JsonSyntaxException(field.name);
                    if (std::holds_alternative<float Factory::*>(field.member))
                        factory.*std::get<float Factory::*>(field.member) = value.getAsFloat();
                    else
                        factory.*std::get<int Factory::*>(field.member) = value.getAsInt();
                }
                if (std::string_view(field.name) == "fixedBiome") {
                    if (factory.fixedBiome < 38 && factory.fixedBiome >= -1) {
                        if (factory.fixedBiome >= kHellBiomeId)
                            factory.fixedBiome += 2;
                    } else {
                        factory.fixedBiome = -1;
                    }
                }
            }
        } catch (const std::exception&) {
        }
        return factory;
    } catch (const std::exception&) {
        return Factory();
    }
}

std::string ChunkProviderSettings::Factory::toString() const
{
    std::string json = "{";
    bool first = true;
    for (const Field& field : kFields) {
        json += first ? "\"" : ",\"";
        first = false;
        json += field.name;
        json += "\":";
        if (std::holds_alternative<float Factory::*>(field.member))
            json += java::toString(this->*std::get<float Factory::*>(field.member));
        else if (std::holds_alternative<int Factory::*>(field.member))
            json += std::to_string(this->*std::get<int Factory::*>(field.member));
        else
            json += this->*std::get<bool Factory::*>(field.member) ? "true" : "false";
    }
    return json + "}";
}

bool ChunkProviderSettings::Factory::operator==(const Factory& other) const
{
    for (const Field& field : kFields) {
        if (std::holds_alternative<float Factory::*>(field.member)) {
            const auto m = std::get<float Factory::*>(field.member);
            // Float.compare: equal bits after NaN canonicalization.
            const float a = this->*m;
            const float b = other.*m;
            if (!(a != a && b != b) && java::floatToRawIntBits(a) != java::floatToRawIntBits(b))
                return false;
        } else if (std::holds_alternative<int Factory::*>(field.member)) {
            const auto m = std::get<int Factory::*>(field.member);
            if (this->*m != other.*m)
                return false;
        } else {
            const auto m = std::get<bool Factory::*>(field.member);
            if (this->*m != other.*m)
                return false;
        }
    }
    return true;
}

int32_t ChunkProviderSettings::Factory::hashCode() const
{
    // Float.floatToIntBits, with 0.0 and -0.0 counted as 0.
    const auto f = [](float v) { return v != 0.0f ? (v != v ? 0x7fc00000 : java::floatToRawIntBits(v)) : 0; };
    int32_t i = f(coordinateScale);
    i = java::add(java::mul(31, i), f(heightScale));
    i = java::add(java::mul(31, i), f(upperLimitScale));
    i = java::add(java::mul(31, i), f(lowerLimitScale));
    i = java::add(java::mul(31, i), f(depthNoiseScaleX));
    i = java::add(java::mul(31, i), f(depthNoiseScaleZ));
    i = java::add(java::mul(31, i), f(depthNoiseScaleExponent));
    i = java::add(java::mul(31, i), f(mainNoiseScaleX));
    i = java::add(java::mul(31, i), f(mainNoiseScaleY));
    i = java::add(java::mul(31, i), f(mainNoiseScaleZ));
    i = java::add(java::mul(31, i), f(baseSize));
    i = java::add(java::mul(31, i), f(stretchY));
    i = java::add(java::mul(31, i), f(biomeDepthWeight));
    i = java::add(java::mul(31, i), f(biomeDepthOffset));
    i = java::add(java::mul(31, i), f(biomeScaleWeight));
    i = java::add(java::mul(31, i), f(biomeScaleOffset));
    i = java::add(java::mul(31, i), seaLevel);
    i = java::add(java::mul(31, i), (useCaves ? 1 : 0));
    i = java::add(java::mul(31, i), (useDungeons ? 1 : 0));
    i = java::add(java::mul(31, i), dungeonChance);
    i = java::add(java::mul(31, i), (useStrongholds ? 1 : 0));
    i = java::add(java::mul(31, i), (useVillages ? 1 : 0));
    i = java::add(java::mul(31, i), (useMineShafts ? 1 : 0));
    i = java::add(java::mul(31, i), (useTemples ? 1 : 0));
    i = java::add(java::mul(31, i), (useMonuments ? 1 : 0));
    i = java::add(java::mul(31, i), (useRavines ? 1 : 0));
    i = java::add(java::mul(31, i), (useWaterLakes ? 1 : 0));
    i = java::add(java::mul(31, i), waterLakeChance);
    i = java::add(java::mul(31, i), (useLavaLakes ? 1 : 0));
    i = java::add(java::mul(31, i), lavaLakeChance);
    i = java::add(java::mul(31, i), (useLavaOceans ? 1 : 0));
    i = java::add(java::mul(31, i), fixedBiome);
    i = java::add(java::mul(31, i), biomeSize);
    i = java::add(java::mul(31, i), riverSize);
    i = java::add(java::mul(31, i), dirtSize);
    i = java::add(java::mul(31, i), dirtCount);
    i = java::add(java::mul(31, i), dirtMinHeight);
    i = java::add(java::mul(31, i), dirtMaxHeight);
    i = java::add(java::mul(31, i), gravelSize);
    i = java::add(java::mul(31, i), gravelCount);
    i = java::add(java::mul(31, i), gravelMinHeight);
    i = java::add(java::mul(31, i), gravelMaxHeight);
    i = java::add(java::mul(31, i), graniteSize);
    i = java::add(java::mul(31, i), graniteCount);
    i = java::add(java::mul(31, i), graniteMinHeight);
    i = java::add(java::mul(31, i), graniteMaxHeight);
    i = java::add(java::mul(31, i), dioriteSize);
    i = java::add(java::mul(31, i), dioriteCount);
    i = java::add(java::mul(31, i), dioriteMinHeight);
    i = java::add(java::mul(31, i), dioriteMaxHeight);
    i = java::add(java::mul(31, i), andesiteSize);
    i = java::add(java::mul(31, i), andesiteCount);
    i = java::add(java::mul(31, i), andesiteMinHeight);
    i = java::add(java::mul(31, i), andesiteMaxHeight);
    i = java::add(java::mul(31, i), coalSize);
    i = java::add(java::mul(31, i), coalCount);
    i = java::add(java::mul(31, i), coalMinHeight);
    i = java::add(java::mul(31, i), coalMaxHeight);
    i = java::add(java::mul(31, i), ironSize);
    i = java::add(java::mul(31, i), ironCount);
    i = java::add(java::mul(31, i), ironMinHeight);
    i = java::add(java::mul(31, i), ironMaxHeight);
    i = java::add(java::mul(31, i), goldSize);
    i = java::add(java::mul(31, i), goldCount);
    i = java::add(java::mul(31, i), goldMinHeight);
    i = java::add(java::mul(31, i), goldMaxHeight);
    i = java::add(java::mul(31, i), redstoneSize);
    i = java::add(java::mul(31, i), redstoneCount);
    i = java::add(java::mul(31, i), redstoneMinHeight);
    i = java::add(java::mul(31, i), redstoneMaxHeight);
    i = java::add(java::mul(31, i), diamondSize);
    i = java::add(java::mul(31, i), diamondCount);
    i = java::add(java::mul(31, i), diamondMinHeight);
    i = java::add(java::mul(31, i), diamondMaxHeight);
    i = java::add(java::mul(31, i), lapisSize);
    i = java::add(java::mul(31, i), lapisCount);
    i = java::add(java::mul(31, i), lapisCenterHeight);
    i = java::add(java::mul(31, i), lapisSpread);
    return i;
}

} // namespace mc
