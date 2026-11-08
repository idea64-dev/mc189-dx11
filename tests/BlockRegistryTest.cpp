// The block registry against the original: tests/reference/blocks.txt is written by the MCP code
// (tools/reference/net/minecraft/block/BlockDump.java); this writes the same text from the port and compares it
// block by block.

#include "block/Block.h"
#include "creativetab/CreativeTabs.h"
#include "core/JavaString.h"
#include "init/Bootstrap.h"
#include "item/ItemStack.h"
#include "util/Random.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

using namespace mc;

namespace {

std::string materialName(const Material& m)
{
    static const std::pair<const Material*, const char*> names[] {
        { &Material::air, "air" }, { &Material::grass, "grass" }, { &Material::ground, "ground" }, { &Material::wood, "wood" },
        { &Material::rock, "rock" }, { &Material::iron, "iron" }, { &Material::anvil, "anvil" }, { &Material::water, "water" },
        { &Material::lava, "lava" }, { &Material::leaves, "leaves" }, { &Material::plants, "plants" }, { &Material::vine, "vine" },
        { &Material::sponge, "sponge" }, { &Material::cloth, "cloth" }, { &Material::fire, "fire" }, { &Material::sand, "sand" },
        { &Material::circuits, "circuits" }, { &Material::carpet, "carpet" }, { &Material::glass, "glass" },
        { &Material::redstoneLight, "redstoneLight" }, { &Material::tnt, "tnt" }, { &Material::coral, "coral" }, { &Material::ice, "ice" },
        { &Material::packedIce, "packedIce" }, { &Material::snow, "snow" }, { &Material::craftedSnow, "craftedSnow" },
        { &Material::cactus, "cactus" }, { &Material::clay, "clay" }, { &Material::gourd, "gourd" }, { &Material::dragonEgg, "dragonEgg" },
        { &Material::portal, "portal" }, { &Material::cake, "cake" }, { &Material::web, "web" }, { &Material::piston, "piston" },
        { &Material::barrier, "barrier" },
    };
    for (const auto& [material, name] : names) {
        if (material == &m)
            return name;
    }
    return "?";
}

const char* b(bool value)
{
    return value ? "true" : "false";
}

std::string f(float value)
{
    return java::toString(value);
}

std::string d(double value)
{
    return java::toString(value);
}

const char* layerName(EnumWorldBlockLayer layer)
{
    constexpr const char* names[] { "SOLID", "CUTOUT_MIPPED", "CUTOUT", "TRANSLUCENT" };
    return names[static_cast<int>(layer)];
}

const char* offsetName(Block::EnumOffsetType type)
{
    constexpr const char* names[] { "NONE", "XZ", "XYZ" };
    return names[static_cast<int>(type)];
}

std::string itemName(const Item* item)
{
    return item ? Item::getNameForItem(item)->getResourcePath() : "null";
}

// The silk touch stack, then quantity and item for fortune 0 and 3 with a fresh Random per seed 0..3.
std::string drops(const Block& block, const IBlockState* state)
{
    const ItemStack silk = block.createStackedBlock(state);
    std::string s = "silk=" + itemName(silk.getItem()) + ":" + std::to_string(silk.getItemDamage());
    for (int fortune = 0; fortune <= 3; fortune += 3) {
        s += " f" + std::to_string(fortune) + "=";
        for (int64_t seed = 0; seed < 4; ++seed) {
            Random rand(seed);
            const int quantity = block.quantityDroppedWithBonus(fortune, rand);
            const Item* item = block.getItemDropped(state, rand, fortune);
            s += (seed == 0 ? "" : ",") + std::to_string(quantity) + "*" + itemName(item);
        }
    }
    return s;
}

std::string dumpBlock(const Block& block)
{
    std::ostringstream out;
    const int id = Block::getIdFromBlock(&block);
    out << "block " << id << " " << Block::getNameForBlock(&block) << " " << block.getClassName() << "\n";
    out << " unlocalized=" << block.getUnlocalizedName() << " tab=" << (block.getCreativeTabToDisplayOn() ? block.getCreativeTabToDisplayOn()->getTabLabel() : "none") << "\n";
    out << " material=" << materialName(block.getMaterial()) << " hardness=" << f(block.getBlockHardnessValue()) << " resistance=" << f(block.getBlockResistanceValue())
        << " light=" << block.getLightValue() << " opacity=" << block.getLightOpacity() << " translucent=" << b(block.isTranslucent())
        << " neighborBrightness=" << b(block.getUseNeighborBrightness()) << " fullBlock=" << b(block.isFullBlock()) << "\n";
    out << " stats=" << b(block.getEnableStats()) << " randomTick=" << b(block.getTickRandomly()) << " container=" << b(block.hasTileEntity())
        << " slipperiness=" << f(block.slipperiness) << " particleGravity=" << f(block.blockParticleGravity) << "\n";
    const Block::SoundType& sound = *block.stepSound;
    out << " sound=" << sound.soundName << "," << f(sound.getVolume()) << "," << f(sound.getFrequency()) << "," << sound.getBreakSound() << ","
        << sound.getStepSound() << "," << sound.getPlaceSound() << "\n";
    out << " bounds=" << d(block.getBlockBoundsMinX()) << "," << d(block.getBlockBoundsMinY()) << "," << d(block.getBlockBoundsMinZ()) << ","
        << d(block.getBlockBoundsMaxX()) << "," << d(block.getBlockBoundsMaxY()) << "," << d(block.getBlockBoundsMaxZ()) << "\n";
    out << " render=" << block.getRenderType() << " layer=" << layerName(block.getBlockLayer()) << " opaque=" << b(block.isOpaqueCube())
        << " fullCube=" << b(block.isFullCube()) << " normal=" << b(block.isNormalCube()) << " blockNormal=" << b(block.isBlockNormalCube())
        << " visuallyOpaque=" << b(block.isVisuallyOpaque()) << " power=" << b(block.canProvidePower()) << " mobility=" << block.getMobilityFlag()
        << " offset=" << offsetName(block.getOffsetType()) << " collidable=" << b(block.isCollidable()) << " updates=" << b(block.requiresUpdates())
        << " comparator=" << b(block.hasComparatorInputOverride()) << " blockColor=" << block.getBlockColor()
        << " ao=" << f(block.getAmbientOcclusionLightValue()) << " spawnIn=" << b(block.canSpawnInBlock()) << " silk=" << b(block.canSilkHarvest())
        << " explosion=" << f(block.getExplosionResistance()) << "\n";
    out << " properties";
    for (const IProperty* p : block.getBlockState().getProperties()) {
        out << " " << p->getName() << "=";
        bool first = true;
        for (const int raw : p->getAllowedRawValues()) {
            out << (first ? "" : ",") << p->getValueName(raw);
            first = false;
        }
    }
    out << " default=" << block.getDefaultState()->toString() << "\n";
    for (const IBlockState* state : block.getBlockState().getValidStates()) {
        out << " state " << state->toString() << " meta=" << block.getMetaFromState(state) << " id=" << Block::BLOCK_STATE_IDS.get(state)
            << " stateId=" << Block::getStateId(state) << " map=" << block.getMapColor(state).colorIndex << " renderColor=" << block.getRenderColor(state)
            << " dropped=" << block.damageDropped(state) << "\n";
        out << " drops " << drops(block, state) << "\n";
    }
    for (int meta = 0; meta < 16; ++meta) {
        std::string s;
        try {
            const IBlockState* state = block.getStateFromMeta(meta);
            s = state ? state->toString() : "null";
        } catch (const std::invalid_argument&) {
            s = "!IllegalArgumentException";
        } catch (const std::out_of_range&) {
            s = "!ArrayIndexOutOfBoundsException";
        }
        out << " meta " << meta << " " << s << "\n";
    }
    return out.str();
}

std::map<int, std::string> referenceSections(std::string& idLines)
{
    std::ifstream file(std::filesystem::path(__FILE__).parent_path() / "reference" / "blocks.txt");
    REQUIRE(file);
    std::map<int, std::string> sections;
    std::string line;
    int current = -1;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.starts_with("block ")) {
            current = std::stoi(line.substr(6));
            sections[current] = line + "\n";
        } else if (line.starts_with("id ")) {
            current = -1;
            idLines += line + "\n";
        } else if (current >= 0) {
            sections[current] += line + "\n";
        }
    }
    return sections;
}

} // namespace

TEST_CASE("Block registry matches the original")
{
    Bootstrap::registerAll();
    std::string idLines;
    const auto expected = referenceSections(idLines);

    int ported = 0;
    for (const Block* block : Block::blockRegistry.values()) {
        const int id = Block::getIdFromBlock(block);
        CAPTURE(id);
        const auto it = expected.find(id);
        REQUIRE(it != expected.end());
        const std::string actual = dumpBlock(*block);
        if (actual != it->second) {
            // Show the first differing line.
            std::istringstream a(actual), e(it->second);
            std::string la, le;
            while (std::getline(a, la) && std::getline(e, le)) {
                if (la != le)
                    break;
            }
            CHECK(la == le);
        }
        ++ported;
    }
    MESSAGE(ported << " of " << expected.size() << " blocks ported");

    if (ported == static_cast<int>(expected.size())) {
        std::string actualIds;
        for (int i = 0; i < 4096 * 16; ++i) {
            if (const IBlockState* state = Block::BLOCK_STATE_IDS.getByValue(i))
                actualIds += "id " + std::to_string(i) + " " + state->toString() + "\n";
        }
        CHECK(actualIds == idLines);
    }
}
