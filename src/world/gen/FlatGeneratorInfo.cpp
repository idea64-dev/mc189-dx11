#include "world/gen/FlatGeneratorInfo.h"

#include "block/AllBlocks.h"
#include "core/JavaString.h"
#include "init/Blocks.h"
#include "util/MathHelper.h"
#include "world/biome/BiomeGenBase.h"

#include <stdexcept>

namespace mc {

FlatLayerInfo::FlatLayerInfo(int version, int height, const Block* layerMaterialIn)
    : field_175902_a(version)
    , layerMaterial(layerMaterialIn->getDefaultState())
    , layerCount(height)
{
}

FlatLayerInfo::FlatLayerInfo(int version, int height, const Block* layerMaterialIn, int meta)
    : FlatLayerInfo(version, height, layerMaterialIn)
{
    layerMaterial = layerMaterialIn->getStateFromMeta(meta);
}

std::string FlatLayerInfo::toString() const
{
    std::string s;
    const Block* block = layerMaterial->getBlock();
    if (field_175902_a >= 3) {
        s = Block::getNameForBlock(block);
        if (layerCount > 1)
            s = std::to_string(layerCount) + "*" + s;
    } else {
        s = std::to_string(Block::getIdFromBlock(block));
        if (layerCount > 1)
            s = std::to_string(layerCount) + "x" + s;
    }
    const int i = block->getMetaFromState(layerMaterial);
    if (i > 0)
        s += ":" + std::to_string(i);
    return s;
}

void FlatGeneratorInfo::func_82645_d()
{
    int i = 0;
    for (FlatLayerInfo& flatlayerinfo : flatLayers) {
        flatlayerinfo.setMinY(i);
        i += flatlayerinfo.getLayerCount();
    }
}

std::string FlatGeneratorInfo::toString() const
{
    std::string out = "3;";
    for (size_t i = 0; i < flatLayers.size(); ++i) {
        if (i > 0)
            out += ",";
        out += flatLayers[i].toString();
    }
    out += ";";
    out += std::to_string(biomeToUse);
    if (!worldFeatures.empty()) {
        out += ";";
        int k = 0;
        for (const auto& [key, map] : worldFeatures) {
            if (k++ > 0)
                out += ",";
            out += java::toLowerCase(key);
            if (!map.empty()) {
                out += "(";
                int j = 0;
                for (const auto& [optionKey, optionValue] : map) {
                    if (j++ > 0)
                        out += " ";
                    out += optionKey;
                    out += "=";
                    out += optionValue;
                }
                out += ")";
            }
        }
    } else {
        out += ";";
    }
    return out;
}

namespace {

std::optional<FlatLayerInfo> func_180715_a(int version, const std::string& text, int minY)
{
    std::vector<std::string> astring = version >= 3 ? java::split(text, "*", 2) : java::split(text, "x", 2);
    int i = 1;
    int j = 0;
    if (astring.size() == 2) {
        try {
            i = java::parseInt(astring[0]);
            if (minY + i >= 256)
                i = 256 - minY;
            if (i < 0)
                i = 0;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    const Block* block = nullptr;
    try {
        if (astring.empty())
            return std::nullopt;
        const std::string s = astring.back();
        if (version < 3) {
            astring = java::split(s, ":", 2);
            if (astring.size() > 1)
                j = java::parseInt(astring[1]);
            block = Block::getBlockById(java::parseInt(astring.at(0)));
        } else {
            astring = java::split(s, ":", 3);
            block = astring.size() > 1 ? Block::getBlockFromName(astring[0] + ":" + astring[1]) : nullptr;
            if (block) {
                j = astring.size() > 2 ? java::parseInt(astring[2]) : 0;
            } else {
                block = Block::getBlockFromName(astring.at(0));
                if (block)
                    j = astring.size() > 1 ? java::parseInt(astring[1]) : 0;
            }
            if (!block)
                return std::nullopt;
        }
        if (block == Blocks::air)
            j = 0;
        if (j < 0 || j > 15)
            j = 0;
    } catch (const std::exception&) {
        return std::nullopt;
    }
    FlatLayerInfo flatlayerinfo(version, i, block, j);
    flatlayerinfo.setMinY(minY);
    return flatlayerinfo;
}

std::optional<std::vector<FlatLayerInfo>> func_180716_a(int version, const std::string& text)
{
    if (text.empty())
        return std::nullopt;
    std::vector<FlatLayerInfo> list;
    int i = 0;
    for (const std::string& s : java::split(text, ",")) {
        auto flatlayerinfo = func_180715_a(version, s, i);
        if (!flatlayerinfo)
            return std::nullopt;
        i += flatlayerinfo->getLayerCount();
        list.push_back(*flatlayerinfo);
    }
    return list;
}

} // namespace

FlatGeneratorInfo FlatGeneratorInfo::createFlatGeneratorFromString(const std::string& flatGeneratorSettings)
{
    const std::vector<std::string> astring = java::split(flatGeneratorSettings, ";", -1);
    const int i = astring.size() == 1 ? 0 : MathHelper::parseIntWithDefault(astring[0], 0);
    if (i < 0 || i > 3)
        return getDefaultFlatGenerator();
    FlatGeneratorInfo flatgeneratorinfo;
    size_t j = astring.size() == 1 ? 0 : 1;
    const auto list = func_180716_a(i, astring[j++]);
    if (!list || list->empty())
        return getDefaultFlatGenerator();
    flatgeneratorinfo.getFlatLayers() = *list;
    flatgeneratorinfo.func_82645_d();
    int k = BiomeGenBase::plains->biomeID;
    if (i > 0 && astring.size() > j)
        k = MathHelper::parseIntWithDefault(astring[j++], k);
    flatgeneratorinfo.setBiome(k);
    if (i > 0 && astring.size() > j) {
        for (const std::string& s : java::split(java::toLowerCase(astring[j++]), ",")) {
            const std::vector<std::string> astring2 = java::split(s, "(", 2);
            Options map;
            if (!astring2[0].empty()) {
                if (astring2.size() > 1 && astring2[1].ends_with(")") && astring2[1].size() > 1) {
                    for (const std::string& option : java::split(astring2[1].substr(0, astring2[1].size() - 1), " ")) {
                        const std::vector<std::string> astring4 = java::split(option, "=", 2);
                        if (astring4.size() == 2)
                            map.put(astring4[0], astring4[1]);
                    }
                }
                // Java puts the (mutable) map first and fills it after; same result.
                flatgeneratorinfo.getWorldFeatures().put(astring2[0], std::move(map));
            }
        }
    } else {
        flatgeneratorinfo.getWorldFeatures().put("village", Options());
    }
    return flatgeneratorinfo;
}

FlatGeneratorInfo FlatGeneratorInfo::getDefaultFlatGenerator()
{
    FlatGeneratorInfo flatgeneratorinfo;
    flatgeneratorinfo.setBiome(BiomeGenBase::plains->biomeID);
    flatgeneratorinfo.getFlatLayers().emplace_back(1, Blocks::bedrock);
    flatgeneratorinfo.getFlatLayers().emplace_back(2, Blocks::dirt);
    flatgeneratorinfo.getFlatLayers().emplace_back(1, Blocks::grass);
    flatgeneratorinfo.func_82645_d();
    flatgeneratorinfo.getWorldFeatures().put("village", Options());
    return flatgeneratorinfo;
}

} // namespace mc
