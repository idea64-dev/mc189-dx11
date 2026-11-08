#include "world/gen/structure/ComponentScatteredFeaturePieces.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/EnumDyeColor.h"
#include "item/ItemMisc.h"
#include "nbt/NBTTagCompound.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/gen/structure/MapGenScatteredFeature.h"
#include "world/gen/structure/MapGenStructure.h"

#include <algorithm>

namespace mc {

namespace ComponentScatteredFeaturePieces {

namespace {

// Java: JunglePyramid.Stones -- cobblestone, 60% of it mossy.
class Stones : public StructureComponent::BlockSelector {
public:
    void selectBlocks(Random& rand, int, int, int, bool) override
    {
        if (rand.nextFloat() < 0.4f)
            blockstate = Blocks::cobblestone->getDefaultState();
        else
            blockstate = Blocks::mossy_cobblestone->getDefaultState();
    }
};

Stones junglePyramidsRandomScatteredStones;

const std::vector<WeightedRandomChestContent>& itemsToGenerateInTemple()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::diamond, 0, 1, 3, 3 },
        { Items::iron_ingot, 0, 1, 5, 10 },
        { Items::gold_ingot, 0, 2, 7, 15 },
        { Items::emerald, 0, 1, 3, 2 },
        { Items::bone, 0, 4, 6, 20 },
        { Items::rotten_flesh, 0, 3, 7, 16 },
        { Items::saddle, 0, 1, 1, 3 },
        { Items::iron_horse_armor, 0, 1, 1, 1 },
        { Items::golden_horse_armor, 0, 1, 1, 1 },
        { Items::diamond_horse_armor, 0, 1, 1, 1 },
    };
    return list;
}

// The jungle temple's chests have the desert pyramid's loot.
const std::vector<WeightedRandomChestContent>& field_175816_i()
{
    return itemsToGenerateInTemple();
}

const std::vector<WeightedRandomChestContent>& field_175815_j()
{
    static const std::vector<WeightedRandomChestContent> list { { Items::arrow, 0, 2, 7, 30 } };
    return list;
}

} // namespace

void registerScatteredFeaturePieces()
{
    MapGenStructureIO::registerStructureComponent("TeDP", [] { return std::make_shared<DesertPyramid>(); });
    MapGenStructureIO::registerStructureComponent("TeJP", [] { return std::make_shared<JunglePyramid>(); });
    MapGenStructureIO::registerStructureComponent("TeSH", [] { return std::make_shared<SwampHut>(); });
}

DesertPyramid::DesertPyramid(Random& p_i2062_1_, int p_i2062_2_, int p_i2062_3_)
    : Feature(p_i2062_1_, p_i2062_2_, 64, p_i2062_3_, 21, 15, 21)
{
}

void DesertPyramid::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Feature::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("hasPlacedChest0", hasPlacedChest[0]);
    tagCompound.setBoolean("hasPlacedChest1", hasPlacedChest[1]);
    tagCompound.setBoolean("hasPlacedChest2", hasPlacedChest[2]);
    tagCompound.setBoolean("hasPlacedChest3", hasPlacedChest[3]);
}

void DesertPyramid::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Feature::readStructureFromNBT(tagCompound);
    hasPlacedChest[0] = tagCompound.getBoolean("hasPlacedChest0");
    hasPlacedChest[1] = tagCompound.getBoolean("hasPlacedChest1");
    hasPlacedChest[2] = tagCompound.getBoolean("hasPlacedChest2");
    hasPlacedChest[3] = tagCompound.getBoolean("hasPlacedChest3");
}

bool DesertPyramid::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, -4, 0, scatteredFeatureSizeX - 1, 0, scatteredFeatureSizeZ - 1, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    for (int i = 1; i <= 9; ++i)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, i, i, i, scatteredFeatureSizeX - 1 - i, i, scatteredFeatureSizeZ - 1 - i, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, i + 1, i, i + 1, scatteredFeatureSizeX - 2 - i, i, scatteredFeatureSizeZ - 2 - i, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    }
    for (int j2 = 0; j2 < scatteredFeatureSizeX; ++j2)
    {
        for (int j = 0; j < scatteredFeatureSizeZ; ++j)
        {
            int k = -5;
            replaceAirAndLiquidDownwards(worldIn, Blocks::sandstone->getDefaultState(), j2, k, j, structureBoundingBoxIn);
        }
    }
    int k2 = getMetadataWithOffset(Blocks::sandstone_stairs, 3);
    int l2 = getMetadataWithOffset(Blocks::sandstone_stairs, 2);
    int i3 = getMetadataWithOffset(Blocks::sandstone_stairs, 0);
    int l = getMetadataWithOffset(Blocks::sandstone_stairs, 1);
    int i1 = ~getDyeDamage(EnumDyeColor::ORANGE) & 15;
    int j1 = ~getDyeDamage(EnumDyeColor::BLUE) & 15;
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 9, 4, Blocks::sandstone->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 10, 1, 3, 10, 3, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(k2), 2, 10, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(l2), 2, 10, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(i3), 0, 10, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(l), 4, 10, 2, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 5, 0, 0, scatteredFeatureSizeX - 1, 9, 4, Blocks::sandstone->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 4, 10, 1, scatteredFeatureSizeX - 2, 10, 3, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(k2), scatteredFeatureSizeX - 3, 10, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(l2), scatteredFeatureSizeX - 3, 10, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(i3), scatteredFeatureSizeX - 5, 10, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(l), scatteredFeatureSizeX - 1, 10, 2, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 0, 0, 12, 4, 4, Blocks::sandstone->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 1, 0, 11, 3, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 9, 1, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 9, 2, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 9, 3, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 10, 3, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 11, 3, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 11, 2, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 11, 1, 1, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 8, 3, 3, Blocks::sandstone->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 2, 8, 2, 2, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, 1, 16, 3, 3, Blocks::sandstone->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, 2, 16, 2, 2, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 4, 5, scatteredFeatureSizeX - 6, 4, scatteredFeatureSizeZ - 6, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 4, 9, 11, 4, 11, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 1, 8, 8, 3, 8, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, 8, 12, 3, 8, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 1, 12, 8, 3, 12, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, 12, 12, 3, 12, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 5, 4, 4, 11, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 5, 1, 5, scatteredFeatureSizeX - 2, 4, 11, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 7, 9, 6, 7, 11, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 7, 7, 9, scatteredFeatureSizeX - 7, 7, 11, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, 9, 5, 7, 11, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 6, 5, 9, scatteredFeatureSizeX - 6, 7, 11, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 5, 5, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 5, 6, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 6, 6, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), scatteredFeatureSizeX - 6, 5, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), scatteredFeatureSizeX - 6, 6, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), scatteredFeatureSizeX - 7, 6, 10, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 4, 4, 2, 6, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 3, 4, 4, scatteredFeatureSizeX - 3, 6, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(k2), 2, 4, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(k2), 2, 3, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(k2), scatteredFeatureSizeX - 3, 4, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(k2), scatteredFeatureSizeX - 3, 3, 4, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 3, 2, 2, 3, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 3, 1, 3, scatteredFeatureSizeX - 2, 2, 3, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    setBlockState(worldIn, Blocks::sandstone_stairs->getDefaultState(), 1, 1, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getDefaultState(), scatteredFeatureSizeX - 2, 1, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SAND)), 1, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SAND)), scatteredFeatureSizeX - 2, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(l), 2, 1, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone_stairs->getStateFromMeta(i3), scatteredFeatureSizeX - 3, 1, 2, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 3, 5, 4, 3, 18, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 5, 3, 5, scatteredFeatureSizeX - 5, 3, 17, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 5, 4, 2, 16, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, scatteredFeatureSizeX - 6, 1, 5, scatteredFeatureSizeX - 5, 2, 16, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    for (int k1 = 5; k1 <= 17; k1 += 2)
    {
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 4, 1, k1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), 4, 2, k1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), scatteredFeatureSizeX - 5, 1, k1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), scatteredFeatureSizeX - 5, 2, k1, structureBoundingBoxIn);
    }
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 10, 0, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 10, 0, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 9, 0, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 11, 0, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 8, 0, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 12, 0, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 7, 0, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 13, 0, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 9, 0, 11, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 11, 0, 11, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 10, 0, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 10, 0, 13, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(j1), 10, 0, 10, structureBoundingBoxIn);
    for (int j3 = 0; j3 <= scatteredFeatureSizeX - 1; j3 += scatteredFeatureSizeX - 1)
    {
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 2, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 2, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 2, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 3, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 3, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 4, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), j3, 4, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 4, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 5, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 5, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 5, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 6, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), j3, 6, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 6, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 7, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 7, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), j3, 7, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 8, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 8, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), j3, 8, 3, structureBoundingBoxIn);
    }
    for (int k3 = 2; k3 <= scatteredFeatureSizeX - 3; k3 += scatteredFeatureSizeX - 3 - 2)
    {
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 - 1, 2, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3, 2, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 + 1, 2, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 - 1, 3, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3, 3, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 + 1, 3, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3 - 1, 4, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), k3, 4, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3 + 1, 4, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 - 1, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 + 1, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3 - 1, 6, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), k3, 6, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3 + 1, 6, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3 - 1, 7, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3, 7, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), k3 + 1, 7, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 - 1, 8, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3, 8, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), k3 + 1, 8, 0, structureBoundingBoxIn);
    }
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 4, 0, 12, 6, 0, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 8, 6, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 12, 6, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 9, 5, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), 10, 5, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stained_hardened_clay->getStateFromMeta(i1), 11, 5, 0, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, -14, 8, 12, -11, 12, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, -10, 8, 12, -10, 12, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, -9, 8, 12, -9, 12, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, -8, 8, 12, -1, 12, Blocks::sandstone->getDefaultState(), Blocks::sandstone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, -11, 9, 11, -1, 11, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::stone_pressure_plate->getDefaultState(), 10, -11, 10, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, -13, 9, 11, -13, 11, Blocks::tnt->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 8, -11, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 8, -10, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), 7, -10, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 7, -11, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 12, -11, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 12, -10, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), 13, -10, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 13, -11, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 10, -11, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 10, -10, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), 10, -10, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 10, -11, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 10, -11, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 10, -10, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::CHISELED)), 10, -10, 13, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH)), 10, -11, 13, structureBoundingBoxIn);
    for (const EnumFacing enumfacing : horizontalFacings())
    {
        if (!hasPlacedChest[getHorizontalIndex(enumfacing)])
        {
            int l1 = getFrontOffsetX(enumfacing) * 2;
            int i2 = getFrontOffsetZ(enumfacing) * 2;
            // Java evaluates the book before the count.
            const auto chestList1 = WeightedRandomChestContent::func_177629_a(itemsToGenerateInTemple(), { Items::enchanted_book->getRandom(randomIn) });
            const int chestMax1 = 2 + randomIn.nextInt(5);
            hasPlacedChest[getHorizontalIndex(enumfacing)] = generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 10 + l1, -11, 10 + i2, chestList1, chestMax1);
        }
    }
    return true;
}

Feature::Feature(Random& p_i2065_1_, int p_i2065_2_, int p_i2065_3_, int p_i2065_4_, int p_i2065_5_, int p_i2065_6_, int p_i2065_7_)
    : StructureComponent(0)
{
    scatteredFeatureSizeX = p_i2065_5_;
    scatteredFeatureSizeY = p_i2065_6_;
    scatteredFeatureSizeZ = p_i2065_7_;
    coordBaseMode = randomFacing(Plane::HORIZONTAL, p_i2065_1_);
    if (isMode(EnumFacing::NORTH) || isMode(EnumFacing::SOUTH))
        boundingBox = StructureBoundingBox(p_i2065_2_, p_i2065_3_, p_i2065_4_, p_i2065_2_ + p_i2065_5_ - 1, p_i2065_3_ + p_i2065_6_ - 1, p_i2065_4_ + p_i2065_7_ - 1);
    else
        boundingBox = StructureBoundingBox(p_i2065_2_, p_i2065_3_, p_i2065_4_, p_i2065_2_ + p_i2065_7_ - 1, p_i2065_3_ + p_i2065_6_ - 1, p_i2065_4_ + p_i2065_5_ - 1);
}

void Feature::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    tagCompound.setInteger("Width", scatteredFeatureSizeX);
    tagCompound.setInteger("Height", scatteredFeatureSizeY);
    tagCompound.setInteger("Depth", scatteredFeatureSizeZ);
    tagCompound.setInteger("HPos", field_74936_d);
}

void Feature::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    scatteredFeatureSizeX = tagCompound.getInteger("Width");
    scatteredFeatureSizeY = tagCompound.getInteger("Height");
    scatteredFeatureSizeZ = tagCompound.getInteger("Depth");
    field_74936_d = tagCompound.getInteger("HPos");
}

bool Feature::func_74935_a(World* worldIn, const StructureBoundingBox& p_74935_2_, int p_74935_3_)
{
    if (field_74936_d >= 0)
    {
        return true;
    }
    else
    {
        int i = 0;
        int j = 0;
        BlockPos mutablePos;
        for (int k = boundingBox.minZ; k <= boundingBox.maxZ; ++k)
        {
            for (int l = boundingBox.minX; l <= boundingBox.maxX; ++l)
            {
                mutablePos.set(l, 64, k);
                if (p_74935_2_.isVecInside(mutablePos))
                {
                    i += std::max(worldIn->getTopSolidOrLiquidBlock(mutablePos).getY(), worldIn->provider->getAverageGroundLevel());
                    ++j;
                }
            }
        }
        if (j == 0)
        {
            return false;
        }
        else
        {
            field_74936_d = i / j;
            boundingBox.offset(0, field_74936_d - boundingBox.minY + p_74935_3_, 0);
            return true;
        }
    }
}

JunglePyramid::JunglePyramid(Random& p_i2064_1_, int p_i2064_2_, int p_i2064_3_)
    : Feature(p_i2064_1_, p_i2064_2_, 64, p_i2064_3_, 12, 10, 15)
{
}

void JunglePyramid::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Feature::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("placedMainChest", placedMainChest);
    tagCompound.setBoolean("placedHiddenChest", placedHiddenChest);
    tagCompound.setBoolean("placedTrap1", placedTrap1);
    tagCompound.setBoolean("placedTrap2", placedTrap2);
}

void JunglePyramid::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Feature::readStructureFromNBT(tagCompound);
    placedMainChest = tagCompound.getBoolean("placedMainChest");
    placedHiddenChest = tagCompound.getBoolean("placedHiddenChest");
    placedTrap1 = tagCompound.getBoolean("placedTrap1");
    placedTrap2 = tagCompound.getBoolean("placedTrap2");
}

bool JunglePyramid::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (!func_74935_a(worldIn, structureBoundingBoxIn, 0))
    {
        return false;
    }
    else
    {
        int i = getMetadataWithOffset(Blocks::stone_stairs, 3);
        int j = getMetadataWithOffset(Blocks::stone_stairs, 2);
        int k = getMetadataWithOffset(Blocks::stone_stairs, 0);
        int l = getMetadataWithOffset(Blocks::stone_stairs, 1);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, -4, 0, scatteredFeatureSizeX - 1, 0, scatteredFeatureSizeZ - 1, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 1, 2, 9, 2, 2, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 1, 12, 9, 2, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 1, 3, 2, 2, 11, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 9, 1, 3, 9, 2, 11, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 3, 1, 10, 6, 1, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 3, 13, 10, 6, 13, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 3, 2, 1, 6, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 10, 3, 2, 10, 6, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 3, 2, 9, 3, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 6, 2, 9, 6, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 3, 7, 3, 8, 7, 11, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 8, 4, 7, 8, 10, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithAir(worldIn, structureBoundingBoxIn, 3, 1, 3, 8, 2, 11);
        fillWithAir(worldIn, structureBoundingBoxIn, 4, 3, 6, 7, 3, 9);
        fillWithAir(worldIn, structureBoundingBoxIn, 2, 4, 2, 9, 5, 12);
        fillWithAir(worldIn, structureBoundingBoxIn, 4, 6, 5, 7, 6, 9);
        fillWithAir(worldIn, structureBoundingBoxIn, 5, 7, 6, 6, 7, 8);
        fillWithAir(worldIn, structureBoundingBoxIn, 5, 1, 2, 6, 2, 2);
        fillWithAir(worldIn, structureBoundingBoxIn, 5, 2, 12, 6, 2, 12);
        fillWithAir(worldIn, structureBoundingBoxIn, 5, 5, 1, 6, 5, 1);
        fillWithAir(worldIn, structureBoundingBoxIn, 5, 5, 13, 6, 5, 13);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 5, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 10, 5, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 5, 9, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 10, 5, 9, structureBoundingBoxIn);
        for (int i1 = 0; i1 <= 14; i1 += 14)
        {
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 4, i1, 2, 5, i1, false, randomIn, junglePyramidsRandomScatteredStones);
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 4, i1, 4, 5, i1, false, randomIn, junglePyramidsRandomScatteredStones);
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 7, 4, i1, 7, 5, i1, false, randomIn, junglePyramidsRandomScatteredStones);
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 9, 4, i1, 9, 5, i1, false, randomIn, junglePyramidsRandomScatteredStones);
        }
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 5, 6, 0, 6, 6, 0, false, randomIn, junglePyramidsRandomScatteredStones);
        for (int k1 = 0; k1 <= 11; k1 += 11)
        {
            for (int j1 = 2; j1 <= 12; j1 += 2)
            {
                fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, k1, 4, j1, k1, 5, j1, false, randomIn, junglePyramidsRandomScatteredStones);
            }
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, k1, 6, 5, k1, 6, 5, false, randomIn, junglePyramidsRandomScatteredStones);
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, k1, 6, 9, k1, 6, 9, false, randomIn, junglePyramidsRandomScatteredStones);
        }
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 7, 2, 2, 9, 2, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 9, 7, 2, 9, 9, 2, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, 7, 12, 2, 9, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 9, 7, 12, 9, 9, 12, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 9, 4, 4, 9, 4, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 7, 9, 4, 7, 9, 4, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 9, 10, 4, 9, 10, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 7, 9, 10, 7, 9, 10, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 5, 9, 7, 6, 9, 7, false, randomIn, junglePyramidsRandomScatteredStones);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 5, 9, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 6, 9, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(j), 5, 9, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(j), 6, 9, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 4, 0, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 5, 0, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 6, 0, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 7, 0, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 4, 1, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 4, 2, 9, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 4, 3, 10, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 7, 1, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 7, 2, 9, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 7, 3, 10, structureBoundingBoxIn);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 9, 4, 1, 9, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 7, 1, 9, 7, 1, 9, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 10, 7, 2, 10, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 5, 4, 5, 6, 4, 5, false, randomIn, junglePyramidsRandomScatteredStones);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(k), 4, 4, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(l), 7, 4, 5, structureBoundingBoxIn);
        for (int l1 = 0; l1 < 4; ++l1)
        {
            setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(j), 5, 0 - l1, 6 + l1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(j), 6, 0 - l1, 6 + l1, structureBoundingBoxIn);
            fillWithAir(worldIn, structureBoundingBoxIn, 5, 0 - l1, 7 + l1, 6, 0 - l1, 9 + l1);
        }
        fillWithAir(worldIn, structureBoundingBoxIn, 1, -3, 12, 10, -1, 13);
        fillWithAir(worldIn, structureBoundingBoxIn, 1, -3, 1, 3, -1, 13);
        fillWithAir(worldIn, structureBoundingBoxIn, 1, -3, 1, 9, -1, 5);
        for (int i2 = 1; i2 <= 13; i2 += 2)
        {
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, -3, i2, 1, -2, i2, false, randomIn, junglePyramidsRandomScatteredStones);
        }
        for (int j2 = 2; j2 <= 12; j2 += 2)
        {
            fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, -1, j2, 3, -1, j2, false, randomIn, junglePyramidsRandomScatteredStones);
        }
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, -2, 1, 5, -2, 1, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 7, -2, 1, 9, -2, 1, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 6, -3, 1, 6, -3, 1, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 6, -1, 1, 6, -1, 1, false, randomIn, junglePyramidsRandomScatteredStones);
        setBlockState(worldIn, Blocks::tripwire_hook->getStateFromMeta(getMetadataWithOffset(Blocks::tripwire_hook, getHorizontalIndex(EnumFacing::EAST)))->withProperty(BlockTripWireHook::ATTACHED, true), 1, -3, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire_hook->getStateFromMeta(getMetadataWithOffset(Blocks::tripwire_hook, getHorizontalIndex(EnumFacing::WEST)))->withProperty(BlockTripWireHook::ATTACHED, true), 4, -3, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire->getDefaultState()->withProperty(BlockTripWire::ATTACHED, true), 2, -3, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire->getDefaultState()->withProperty(BlockTripWire::ATTACHED, true), 3, -3, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 7, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 5, -3, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 4, -3, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 3, -3, 1, structureBoundingBoxIn);
        if (!placedTrap1)
        {
            placedTrap1 = generateDispenserContents(worldIn, structureBoundingBoxIn, randomIn, 3, -2, 1, getIndex(EnumFacing::NORTH), field_175815_j(), 2);
        }
        setBlockState(worldIn, Blocks::vine->getStateFromMeta(15), 3, -2, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire_hook->getStateFromMeta(getMetadataWithOffset(Blocks::tripwire_hook, getHorizontalIndex(EnumFacing::NORTH)))->withProperty(BlockTripWireHook::ATTACHED, true), 7, -3, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire_hook->getStateFromMeta(getMetadataWithOffset(Blocks::tripwire_hook, getHorizontalIndex(EnumFacing::SOUTH)))->withProperty(BlockTripWireHook::ATTACHED, true), 7, -3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire->getDefaultState()->withProperty(BlockTripWire::ATTACHED, true), 7, -3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire->getDefaultState()->withProperty(BlockTripWire::ATTACHED, true), 7, -3, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::tripwire->getDefaultState()->withProperty(BlockTripWire::ATTACHED, true), 7, -3, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 8, -3, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 9, -3, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 9, -3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 9, -3, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 9, -2, 4, structureBoundingBoxIn);
        if (!placedTrap2)
        {
            placedTrap2 = generateDispenserContents(worldIn, structureBoundingBoxIn, randomIn, 9, -2, 3, getIndex(EnumFacing::WEST), field_175815_j(), 2);
        }
        setBlockState(worldIn, Blocks::vine->getStateFromMeta(15), 8, -1, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::vine->getStateFromMeta(15), 8, -2, 3, structureBoundingBoxIn);
        if (!placedMainChest)
        {
            // Java evaluates the book before the count.
            const auto chestList2 = WeightedRandomChestContent::func_177629_a(field_175816_i(), { Items::enchanted_book->getRandom(randomIn) });
            const int chestMax2 = 2 + randomIn.nextInt(5);
            placedMainChest = generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 8, -3, 3, chestList2, chestMax2);
        }
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 9, -3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 8, -3, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 4, -3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 5, -2, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 5, -1, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 6, -3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 7, -2, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 7, -1, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 8, -3, 5, structureBoundingBoxIn);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 9, -1, 1, 9, -1, 5, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithAir(worldIn, structureBoundingBoxIn, 8, -3, 8, 10, -1, 10);
        setBlockState(worldIn, Blocks::stonebrick->getStateFromMeta(BlockStoneBrick::CHISELED_META), 8, -2, 11, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getStateFromMeta(BlockStoneBrick::CHISELED_META), 9, -2, 11, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getStateFromMeta(BlockStoneBrick::CHISELED_META), 10, -2, 11, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::lever->getStateFromMeta(BlockLever::getMetadataForFacing(getFront(getMetadataWithOffset(Blocks::lever, getIndex(EnumFacing::NORTH))))), 8, -2, 12, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::lever->getStateFromMeta(BlockLever::getMetadataForFacing(getFront(getMetadataWithOffset(Blocks::lever, getIndex(EnumFacing::NORTH))))), 9, -2, 12, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::lever->getStateFromMeta(BlockLever::getMetadataForFacing(getFront(getMetadataWithOffset(Blocks::lever, getIndex(EnumFacing::NORTH))))), 10, -2, 12, structureBoundingBoxIn);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 8, -3, 8, 8, -3, 10, false, randomIn, junglePyramidsRandomScatteredStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 10, -3, 8, 10, -3, 10, false, randomIn, junglePyramidsRandomScatteredStones);
        setBlockState(worldIn, Blocks::mossy_cobblestone->getDefaultState(), 10, -2, 9, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 8, -2, 9, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 8, -2, 10, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::redstone_wire->getDefaultState(), 10, -1, 9, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sticky_piston->getStateFromMeta(getIndex(EnumFacing::UP)), 9, -2, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sticky_piston->getStateFromMeta(getMetadataWithOffset(Blocks::sticky_piston, getIndex(EnumFacing::WEST))), 10, -2, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::sticky_piston->getStateFromMeta(getMetadataWithOffset(Blocks::sticky_piston, getIndex(EnumFacing::WEST))), 10, -1, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::unpowered_repeater->getStateFromMeta(getMetadataWithOffset(Blocks::unpowered_repeater, getHorizontalIndex(EnumFacing::NORTH))), 10, -2, 10, structureBoundingBoxIn);
        if (!placedHiddenChest)
        {
            // Java evaluates the book before the count.
            const auto chestList3 = WeightedRandomChestContent::func_177629_a(field_175816_i(), { Items::enchanted_book->getRandom(randomIn) });
            const int chestMax3 = 2 + randomIn.nextInt(5);
            placedHiddenChest = generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 9, -3, 10, chestList3, chestMax3);
        }
        return true;
    }
}

SwampHut::SwampHut(Random& p_i2066_1_, int p_i2066_2_, int p_i2066_3_)
    : Feature(p_i2066_1_, p_i2066_2_, 64, p_i2066_3_, 7, 7, 9)
{
}

void SwampHut::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Feature::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Witch", hasWitch);
}

void SwampHut::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Feature::readStructureFromNBT(tagCompound);
    hasWitch = tagCompound.getBoolean("Witch");
}

bool SwampHut::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (!func_74935_a(worldIn, structureBoundingBoxIn, 0))
    {
        return false;
    }
    else
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 5, 1, 7, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 2, 5, 4, 7, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 0, 4, 1, 0, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 2, 2, 3, 3, 2, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 3, 1, 3, 6, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 3, 5, 3, 6, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 2, 7, 4, 3, 7, Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), Blocks::planks->getStateFromMeta(static_cast<int>(BlockPlanks::EnumType::SPRUCE)), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 2, 1, 3, 2, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 0, 2, 5, 3, 2, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 7, 1, 3, 7, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 0, 7, 5, 3, 7, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 2, 3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 3, 3, 7, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 3, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 5, 3, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::air->getDefaultState(), 5, 3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::flower_pot->getDefaultState()->withProperty(BlockFlowerPot::CONTENTS, BlockFlowerPot::EnumFlowerType::MUSHROOM_RED), 1, 3, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::crafting_table->getDefaultState(), 3, 2, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::cauldron->getDefaultState(), 4, 2, 6, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 2, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 5, 2, 1, structureBoundingBoxIn);
        int i = getMetadataWithOffset(Blocks::oak_stairs, 3);
        int j = getMetadataWithOffset(Blocks::oak_stairs, 1);
        int k = getMetadataWithOffset(Blocks::oak_stairs, 0);
        int l = getMetadataWithOffset(Blocks::oak_stairs, 2);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 1, 6, 4, 1, Blocks::spruce_stairs->getStateFromMeta(i), Blocks::spruce_stairs->getStateFromMeta(i), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 2, 0, 4, 7, Blocks::spruce_stairs->getStateFromMeta(k), Blocks::spruce_stairs->getStateFromMeta(k), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 4, 2, 6, 4, 7, Blocks::spruce_stairs->getStateFromMeta(j), Blocks::spruce_stairs->getStateFromMeta(j), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 8, 6, 4, 8, Blocks::spruce_stairs->getStateFromMeta(l), Blocks::spruce_stairs->getStateFromMeta(l), false);
        for (int i1 = 2; i1 <= 7; i1 += 5)
        {
            for (int j1 = 1; j1 <= 5; j1 += 4)
            {
                replaceAirAndLiquidDownwards(worldIn, Blocks::log->getDefaultState(), j1, -1, i1, structureBoundingBoxIn);
            }
        }
        if (!hasWitch)
        {
            int l1 = getXWithOffset(2, 5);
            int i2 = getYWithOffset(2);
            int k1 = getZWithOffset(2, 5);
            if (structureBoundingBoxIn.isVecInside(BlockPos(l1, i2, k1)))
            {
                hasWitch = true;
                // Java: an EntityWitch at (l1 + 0.5, i2, k1 + 0.5) -- entities come with stage 10. Its onInitialSpawn
                // only draws from the entity's own random, not the world's.
            }
        }
        return true;
    }
}


} // namespace ComponentScatteredFeaturePieces

// --- MapGenScatteredFeature ---

MapGenScatteredFeature::MapGenScatteredFeature()
{
    scatteredFeatureSpawnList.push_back({ "Witch", 1, 1, 1 });
}

MapGenScatteredFeature::MapGenScatteredFeature(const FlatGeneratorInfo::Options& options)
    : MapGenScatteredFeature()
{
    for (const auto& [key, value] : options) {
        if (key == "distance")
            maxDistanceBetweenScatteredFeatures = MathHelper::parseIntWithDefaultAndMax(value, maxDistanceBetweenScatteredFeatures, minDistanceBetweenScatteredFeatures + 1);
    }
}

bool MapGenScatteredFeature::canSpawnStructureAtCoords(int chunkX, int chunkZ)
{
    const int i = chunkX;
    const int j = chunkZ;
    if (chunkX < 0)
        chunkX -= maxDistanceBetweenScatteredFeatures - 1;
    if (chunkZ < 0)
        chunkZ -= maxDistanceBetweenScatteredFeatures - 1;
    int k = chunkX / maxDistanceBetweenScatteredFeatures;
    int l = chunkZ / maxDistanceBetweenScatteredFeatures;
    Random& random = worldObj->setRandomSeed(k, l, 14357617);
    k = k * maxDistanceBetweenScatteredFeatures;
    l = l * maxDistanceBetweenScatteredFeatures;
    k = k + random.nextInt(maxDistanceBetweenScatteredFeatures - minDistanceBetweenScatteredFeatures);
    l = l + random.nextInt(maxDistanceBetweenScatteredFeatures - minDistanceBetweenScatteredFeatures);
    if (i == k && j == l) {
        BiomeGenBase* biomegenbase = worldObj->getWorldChunkManager()->getBiomeGenerator(BlockPos(i * 16 + 8, 0, j * 16 + 8));
        if (!biomegenbase)
            return false;
        // Java: biomelist
        return biomegenbase == BiomeGenBase::desert || biomegenbase == BiomeGenBase::desertHills || biomegenbase == BiomeGenBase::jungle
            || biomegenbase == BiomeGenBase::jungleHills || biomegenbase == BiomeGenBase::swampland;
    }
    return false;
}

std::shared_ptr<StructureStart> MapGenScatteredFeature::getStructureStart(int chunkX, int chunkZ)
{
    return std::make_shared<Start>(worldObj, rand, chunkX, chunkZ);
}

bool MapGenScatteredFeature::func_175798_a(const BlockPos& pos)
{
    StructureStart* structurestart = func_175797_c(pos);
    if (dynamic_cast<Start*>(structurestart) && !structurestart->getComponents().empty())
        return dynamic_cast<const ComponentScatteredFeaturePieces::SwampHut*>(structurestart->getComponents().front().get()) != nullptr;
    return false;
}

MapGenScatteredFeature::Start::Start(World* worldIn, Random& rand, int chunkX, int chunkZ)
    : StructureStart(chunkX, chunkZ)
{
    BiomeGenBase* biomegenbase = worldIn->getBiomeGenForCoords(BlockPos(chunkX * 16 + 8, 0, chunkZ * 16 + 8));
    if (biomegenbase != BiomeGenBase::jungle && biomegenbase != BiomeGenBase::jungleHills) {
        if (biomegenbase == BiomeGenBase::swampland)
            components.push_back(std::make_shared<ComponentScatteredFeaturePieces::SwampHut>(rand, chunkX * 16, chunkZ * 16));
        else if (biomegenbase == BiomeGenBase::desert || biomegenbase == BiomeGenBase::desertHills)
            components.push_back(std::make_shared<ComponentScatteredFeaturePieces::DesertPyramid>(rand, chunkX * 16, chunkZ * 16));
    } else {
        components.push_back(std::make_shared<ComponentScatteredFeaturePieces::JunglePyramid>(rand, chunkX * 16, chunkZ * 16));
    }
    updateBoundingBox();
}

} // namespace mc
