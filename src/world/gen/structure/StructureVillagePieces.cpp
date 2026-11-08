#include "world/gen/structure/StructureVillagePieces.h"

#include "block/AllBlocks.h"
#include "block/material/Material.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemArmor.h"
#include "item/EnumDyeColor.h"
#include "item/Item.h"
#include "nbt/NBTTagCompound.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/WorldProvider.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/gen/structure/MapGenStructure.h"
#include "world/gen/structure/MapGenVillage.h"

#include <algorithm>
#include <cstdlib>

namespace mc {

namespace StructureVillagePieces {

namespace {

const std::vector<WeightedRandomChestContent>& villageBlacksmithChestContents()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::diamond, 0, 1, 3, 3 },
        { Items::iron_ingot, 0, 1, 5, 10 },
        { Items::gold_ingot, 0, 1, 3, 5 },
        { Items::bread, 0, 1, 3, 15 },
        { Items::apple, 0, 1, 3, 15 },
        { Items::iron_pickaxe, 0, 1, 1, 5 },
        { Items::iron_sword, 0, 1, 1, 5 },
        { Items::iron_chestplate, 0, 1, 1, 5 },
        { Items::iron_helmet, 0, 1, 1, 5 },
        { Items::iron_leggings, 0, 1, 1, 5 },
        { Items::iron_boots, 0, 1, 1, 5 },
        { Item::getItemFromBlock(Blocks::obsidian), 0, 3, 7, 5 },
        { Item::getItemFromBlock(Blocks::sapling), 0, 3, 7, 5 },
        { Items::saddle, 0, 1, 1, 3 },
        { Items::iron_horse_armor, 0, 1, 1, 1 },
        { Items::golden_horse_armor, 0, 1, 1, 1 },
        { Items::diamond_horse_armor, 0, 1, 1, 1 },
    };
    return list;
}

int func_75079_a(const PieceWeightList& list)
{
    bool flag = false;
    int i = 0;
    for (const auto& weight : list) {
        if (weight->villagePiecesLimit > 0 && weight->villagePiecesSpawned < weight->villagePiecesLimit)
            flag = true;
        i += weight->villagePieceWeight;
    }
    return flag ? i : -1;
}

std::shared_ptr<Village> func_176067_c(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    const int i = func_75079_a(start->structureVillageWeightedPieceList);
    if (i <= 0)
        return nullptr;
    int j = 0;
    while (j < 5) {
        ++j;
        int k = rand.nextInt(i);
        for (const std::shared_ptr<PieceWeight> weight : start->structureVillageWeightedPieceList) {
            k -= weight->villagePieceWeight;
            if (k < 0) {
                if (!weight->canSpawnMoreVillagePiecesOfType(type)
                    || (weight == start->structVillagePieceWeight && start->structureVillageWeightedPieceList.size() > 1))
                    break;
                // Java: func_176065_a dispatches on the piece class.
                std::shared_ptr<Village> village = weight->villagePieceClass(start, list, rand, x, y, z, facing, type);
                if (village) {
                    ++weight->villagePiecesSpawned;
                    start->structVillagePieceWeight = weight;
                    if (!weight->canSpawnMoreVillagePieces())
                        std::erase(start->structureVillageWeightedPieceList, weight);
                    return village;
                }
            }
        }
    }
    if (const auto structureboundingbox = Torch::func_175856_a(start, list, rand, x, y, z, facing))
        return std::make_shared<Torch>(start, type, rand, *structureboundingbox, facing);
    return nullptr;
}

std::shared_ptr<StructureComponent> func_176066_d(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    if (type > 50)
        return nullptr;
    if (std::abs(x - start->getBoundingBox().minX) <= 112 && std::abs(z - start->getBoundingBox().minZ) <= 112) {
        std::shared_ptr<StructureComponent> structurecomponent = func_176067_c(start, list, rand, x, y, z, facing, type + 1);
        if (structurecomponent) {
            const StructureBoundingBox& bb = structurecomponent->getBoundingBox();
            const int i = (bb.minX + bb.maxX) / 2;
            const int j = (bb.minZ + bb.maxZ) / 2;
            const int k = bb.maxX - bb.minX;
            const int l = bb.maxZ - bb.minZ;
            const int i1 = k > l ? k : l;
            if (start->getWorldChunkManager()->areBiomesViable(i, j, i1 / 2 + 4, MapGenVillage::villageSpawnBiomes())) {
                list.push_back(structurecomponent);
                start->field_74932_i.push_back(structurecomponent);
                return structurecomponent;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<StructureComponent> func_176069_e(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    if (type > 3 + start->terrainType)
        return nullptr;
    if (std::abs(x - start->getBoundingBox().minX) <= 112 && std::abs(z - start->getBoundingBox().minZ) <= 112) {
        const auto structureboundingbox = Path::func_175848_a(start, list, rand, x, y, z, facing);
        if (structureboundingbox && structureboundingbox->minY > 10) {
            auto structurecomponent = std::make_shared<Path>(start, type, rand, *structureboundingbox, facing);
            const StructureBoundingBox& bb = structurecomponent->getBoundingBox();
            const int i = (bb.minX + bb.maxX) / 2;
            const int j = (bb.minZ + bb.maxZ) / 2;
            const int k = bb.maxX - bb.minX;
            const int l = bb.maxZ - bb.minZ;
            const int i1 = k > l ? k : l;
            if (start->getWorldChunkManager()->areBiomesViable(i, j, i1 / 2 + 4, MapGenVillage::villageSpawnBiomes())) {
                list.push_back(structurecomponent);
                start->field_74930_j.push_back(structurecomponent);
                return structurecomponent;
            }
        }
    }
    return nullptr;
}

// The finders of the houses: the box for the piece's size, if it's free and (except for House4Garden) high enough.
template <class T>
std::shared_ptr<Village> findPiece(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type, int sizeX, int sizeY,
    int sizeZ, bool checkDepth = true)
{
    const auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(x, y, z, 0, 0, 0, sizeX, sizeY, sizeZ, facing);
    if ((!checkDepth || Village::canVillageGoDeeper(structureboundingbox)) && !StructureComponent::findIntersecting(list, structureboundingbox))
        return std::make_shared<T>(start, type, rand, structureboundingbox, facing);
    return nullptr;
}

} // namespace

void registerVillagePieces()
{
    MapGenStructureIO::registerStructureComponent("ViBH", [] { return std::make_shared<House1>(); });
    MapGenStructureIO::registerStructureComponent("ViDF", [] { return std::make_shared<Field1>(); });
    MapGenStructureIO::registerStructureComponent("ViF", [] { return std::make_shared<Field2>(); });
    MapGenStructureIO::registerStructureComponent("ViL", [] { return std::make_shared<Torch>(); });
    MapGenStructureIO::registerStructureComponent("ViPH", [] { return std::make_shared<Hall>(); });
    MapGenStructureIO::registerStructureComponent("ViSH", [] { return std::make_shared<House4Garden>(); });
    MapGenStructureIO::registerStructureComponent("ViSmH", [] { return std::make_shared<WoodHut>(); });
    MapGenStructureIO::registerStructureComponent("ViST", [] { return std::make_shared<Church>(); });
    MapGenStructureIO::registerStructureComponent("ViS", [] { return std::make_shared<House2>(); });
    MapGenStructureIO::registerStructureComponent("ViStart", [] { return std::make_shared<Start>(); });
    MapGenStructureIO::registerStructureComponent("ViSR", [] { return std::make_shared<Path>(); });
    MapGenStructureIO::registerStructureComponent("ViTRH", [] { return std::make_shared<House3>(); });
    MapGenStructureIO::registerStructureComponent("ViW", [] { return std::make_shared<Well>(); });
}

PieceWeightList getStructureVillageWeightedPieceList(Random& random, int size)
{
    PieceWeightList list;
    const auto add = [&](PieceWeight::Creator creator, int weight, int min, int max) {
        const int limit = MathHelper::getRandomIntegerInRange(random, min, max);
        list.push_back(std::make_shared<PieceWeight>(creator, weight, limit));
    };
    add(&House4Garden::func_175858_a, 4, 2 + size, 4 + size * 2);
    add(&Church::func_175854_a, 20, 0 + size, 1 + size);
    add(&House1::func_175850_a, 20, 0 + size, 2 + size);
    add(&WoodHut::func_175853_a, 3, 2 + size, 5 + size * 3);
    add(&Hall::func_175857_a, 15, 0 + size, 2 + size);
    add(&Field1::func_175851_a, 3, 1 + size, 4 + size);
    add(&Field2::func_175852_a, 3, 2 + size, 4 + size * 2);
    add(&House2::func_175855_a, 15, 0, 1 + size);
    add(&House3::func_175849_a, 8, 0 + size, 3 + size * 2);
    std::erase_if(list, [](const std::shared_ptr<PieceWeight>& w) { return w->villagePiecesLimit == 0; });
    return list;
}

// --- Village ---

Village::Village(Start* start, int type)
    : StructureComponent(type)
{
    if (start)
        isDesertVillage = start->inDesert;
}

void Village::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    tagCompound.setInteger("HPos", field_143015_k);
    tagCompound.setInteger("VCount", villagersSpawned);
    tagCompound.setBoolean("Desert", isDesertVillage);
}

void Village::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    field_143015_k = tagCompound.getInteger("HPos");
    villagersSpawned = tagCompound.getInteger("VCount");
    isDesertVillage = tagCompound.getBoolean("Desert");
}

std::shared_ptr<StructureComponent> Village::getNextComponentNN(Start* start, StructureComponentList& list, Random& rand, int y, int offset)
{
    if (!coordBaseMode)
        return nullptr;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        return func_176066_d(start, list, rand, boundingBox.minX - 1, boundingBox.minY + y, boundingBox.minZ + offset, EnumFacing::WEST, getComponentType());
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        return func_176066_d(start, list, rand, boundingBox.minX + offset, boundingBox.minY + y, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType());
    default:
        return nullptr;
    }
}

std::shared_ptr<StructureComponent> Village::getNextComponentPP(Start* start, StructureComponentList& list, Random& rand, int y, int offset)
{
    if (!coordBaseMode)
        return nullptr;
    switch (*coordBaseMode) {
    case EnumFacing::NORTH:
    case EnumFacing::SOUTH:
        return func_176066_d(start, list, rand, boundingBox.maxX + 1, boundingBox.minY + y, boundingBox.minZ + offset, EnumFacing::EAST, getComponentType());
    case EnumFacing::WEST:
    case EnumFacing::EAST:
        return func_176066_d(start, list, rand, boundingBox.minX + offset, boundingBox.minY + y, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType());
    default:
        return nullptr;
    }
}

int Village::getAverageGroundLevel(World* worldIn, const StructureBoundingBox& structurebb)
{
    int i = 0;
    int j = 0;
    BlockPos mutablePos;
    for (int k = boundingBox.minZ; k <= boundingBox.maxZ; ++k) {
        for (int l = boundingBox.minX; l <= boundingBox.maxX; ++l) {
            mutablePos.set(l, 64, k);
            if (structurebb.isVecInside(mutablePos)) {
                i += std::max(worldIn->getTopSolidOrLiquidBlock(mutablePos).getY(), worldIn->provider->getAverageGroundLevel());
                ++j;
            }
        }
    }
    if (j == 0)
        return -1;
    return i / j;
}

void Village::spawnVillagers(World* worldIn, const StructureBoundingBox& structurebb, int x, int y, int z, int count)
{
    if (villagersSpawned < count) {
        for (int i = villagersSpawned; i < count; ++i) {
            const int j = getXWithOffset(x + i, z);
            const int k = getYWithOffset(y);
            const int l = getZWithOffset(x + i, z);
            if (!structurebb.isVecInside(BlockPos(j, k, l)))
                break;
            ++villagersSpawned;
            // Java: an EntityVillager at (j + 0.5, k, l + 0.5) with the profession func_180779_c(i, ...) -- entities
            // come with stage 10. Its onInitialSpawn picks a profession from the world's random first; that draw is
            // kept.
            worldIn->rand.nextInt(5);
        }
    }
}

const IBlockState* Village::func_175847_a(const IBlockState* state) const
{
    if (isDesertVillage) {
        const Block* block = state->getBlock();
        if (block == Blocks::log || block == Blocks::log2)
            return Blocks::sandstone->getDefaultState();
        if (block == Blocks::cobblestone)
            return Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::DEFAULT));
        if (block == Blocks::planks)
            return Blocks::sandstone->getStateFromMeta(static_cast<int>(BlockSandStone::EnumType::SMOOTH));
        if (block == Blocks::oak_stairs)
            return Blocks::sandstone_stairs->getDefaultState()->withProperty(BlockStairs::FACING, state->getValue(BlockStairs::FACING));
        if (block == Blocks::stone_stairs)
            return Blocks::sandstone_stairs->getDefaultState()->withProperty(BlockStairs::FACING, state->getValue(BlockStairs::FACING));
        if (block == Blocks::gravel)
            return Blocks::sandstone->getDefaultState();
    }
    return state;
}

void Village::setBlockState(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn)
{
    StructureComponent::setBlockState(worldIn, func_175847_a(blockstateIn), x, y, z, boundingboxIn);
}

void Village::fillWithBlocks(World* worldIn, const StructureBoundingBox& boundingboxIn, int xMin, int yMin, int zMin, int xMax, int yMax, int zMax,
    const IBlockState* boundaryBlockState, const IBlockState* insideBlockState, bool existingOnly)
{
    const IBlockState* iblockstate = func_175847_a(boundaryBlockState);
    const IBlockState* iblockstate1 = func_175847_a(insideBlockState);
    StructureComponent::fillWithBlocks(worldIn, boundingboxIn, xMin, yMin, zMin, xMax, yMax, zMax, iblockstate, iblockstate1, existingOnly);
}

void Village::replaceAirAndLiquidDownwards(World* worldIn, const IBlockState* blockstateIn, int x, int y, int z, const StructureBoundingBox& boundingboxIn)
{
    StructureComponent::replaceAirAndLiquidDownwards(worldIn, func_175847_a(blockstateIn), x, y, z, boundingboxIn);
}

// --- the houses ---

#define MC_PIECE_CTOR(NAME)                                                                                                                         \
    NAME::NAME(Start* start, int type, Random&, const StructureBoundingBox& structurebb, EnumFacing facing)                                          \
        : Village(start, type)                                                                                                                      \
    {                                                                                                                                               \
        coordBaseMode = facing;                                                                                                                     \
        boundingBox = structurebb;                                                                                                                  \
    }

MC_PIECE_CTOR(Church)
MC_PIECE_CTOR(Hall)
MC_PIECE_CTOR(House1)
MC_PIECE_CTOR(House2)
MC_PIECE_CTOR(House3)
MC_PIECE_CTOR(Torch)

#undef MC_PIECE_CTOR

std::shared_ptr<Village> Church::func_175854_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<Church>(start, list, rand, x, y, z, facing, type, 5, 12, 9);
}

bool Church::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 12 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 3, 3, 7, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 1, 3, 9, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 3, 0, 8, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 3, 10, 0, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 10, 3, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 4, 10, 3, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 4, 0, 4, 7, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 4, 4, 4, 7, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 8, 3, 4, 8, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 4, 3, 10, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 5, 3, 5, 7, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 9, 0, 4, 9, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 0, 4, 4, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 11, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 11, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 2, 11, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 2, 11, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 1, 1, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 1, 1, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 2, 1, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 3, 1, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 3, 1, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 1, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 2, 1, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 3, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 1)), 1, 2, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 0)), 3, 2, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 3, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 3, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 6, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 7, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 6, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 7, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 6, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 7, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 6, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 7, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 3, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 3, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 3, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, getOpposite(*coordBaseMode)), 2, 4, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, rotateY(*coordBaseMode)), 1, 4, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, rotateYCCW(*coordBaseMode)), 3, 4, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, *coordBaseMode), 2, 4, 5, structureBoundingBoxIn);
    int i = getMetadataWithOffset(Blocks::ladder, 4);

    for (int j = 1; j <= 9; ++j)
    {
        setBlockState(worldIn, Blocks::ladder->getStateFromMeta(i), 3, j, 3, structureBoundingBoxIn);
    }

    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 2, 0, structureBoundingBoxIn);
    placeDoorCurrentPosition(worldIn, structureBoundingBoxIn, randomIn, 2, 1, 0, getHorizontal(getMetadataWithOffset(Blocks::oak_door, 1)));

    if (isMaterial(getBlockStateFromPos(worldIn, 2, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, 2, -1, -1, structureBoundingBoxIn), Material::air))
    {
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 2, 0, -1, structureBoundingBoxIn);
    }

    for (int l = 0; l < 9; ++l)
    {
        for (int k = 0; k < 5; ++k)
        {
            clearCurrentPositionBlocksUpwards(worldIn, k, 12, l, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), k, -1, l, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 2, 1, 2, 1);
    return true;
}

// Field1

Field1::Field1(Start* start, int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing)
    : Village(start, type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
    cropTypeA = func_151559_a(rand);
    cropTypeB = func_151559_a(rand);
    cropTypeC = func_151559_a(rand);
    cropTypeD = func_151559_a(rand);
}

void Field1::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Village::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("CA", Block::blockRegistry.getIDForObject(cropTypeA));
    tagCompound.setInteger("CB", Block::blockRegistry.getIDForObject(cropTypeB));
    tagCompound.setInteger("CC", Block::blockRegistry.getIDForObject(cropTypeC));
    tagCompound.setInteger("CD", Block::blockRegistry.getIDForObject(cropTypeD));
}

void Field1::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Village::readStructureFromNBT(tagCompound);
    cropTypeA = Block::getBlockById(tagCompound.getInteger("CA"));
    cropTypeB = Block::getBlockById(tagCompound.getInteger("CB"));
    cropTypeC = Block::getBlockById(tagCompound.getInteger("CC"));
    cropTypeD = Block::getBlockById(tagCompound.getInteger("CD"));
}

Block* Field1::func_151559_a(Random& rand)
{
    switch (rand.nextInt(5)) {
    case 0: return Blocks::carrots;
    case 1: return Blocks::potatoes;
    default: return Blocks::wheat;
    }
}

std::shared_ptr<Village> Field1::func_175851_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<Field1>(start, list, rand, x, y, z, facing, type, 13, 4, 9);
}

bool Field1::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 4 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 12, 4, 8, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 1, 2, 0, 7, Blocks::farmland->getDefaultState(), Blocks::farmland->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 1, 5, 0, 7, Blocks::farmland->getDefaultState(), Blocks::farmland->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 0, 1, 8, 0, 7, Blocks::farmland->getDefaultState(), Blocks::farmland->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 0, 1, 11, 0, 7, Blocks::farmland->getDefaultState(), Blocks::farmland->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 0, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 0, 0, 6, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 0, 0, 12, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 11, 0, 0, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 8, 11, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 0, 1, 3, 0, 7, Blocks::water->getDefaultState(), Blocks::water->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 0, 1, 9, 0, 7, Blocks::water->getDefaultState(), Blocks::water->getDefaultState(), false);

    for (int i = 1; i <= 7; ++i)
    {
        setBlockState(worldIn, cropTypeA->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 1, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeA->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 2, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeB->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 4, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeB->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 5, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeC->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 7, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeC->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 8, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeD->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 10, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeD->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 11, 1, i, structureBoundingBoxIn);
    }

    for (int k = 0; k < 9; ++k)
    {
        for (int j = 0; j < 13; ++j)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j, 4, k, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::dirt->getDefaultState(), j, -1, k, structureBoundingBoxIn);
        }
    }

    return true;
}

// Field2

Field2::Field2(Start* start, int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing)
    : Village(start, type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
    cropTypeA = func_151560_a(rand);
    cropTypeB = func_151560_a(rand);
}

void Field2::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Village::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("CA", Block::blockRegistry.getIDForObject(cropTypeA));
    tagCompound.setInteger("CB", Block::blockRegistry.getIDForObject(cropTypeB));
}

void Field2::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Village::readStructureFromNBT(tagCompound);
    cropTypeA = Block::getBlockById(tagCompound.getInteger("CA"));
    cropTypeB = Block::getBlockById(tagCompound.getInteger("CB"));
}

Block* Field2::func_151560_a(Random& rand)
{
    switch (rand.nextInt(5)) {
    case 0: return Blocks::carrots;
    case 1: return Blocks::potatoes;
    default: return Blocks::wheat;
    }
}

std::shared_ptr<Village> Field2::func_175852_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<Field2>(start, list, rand, x, y, z, facing, type, 7, 4, 9);
}

bool Field2::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 4 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 6, 4, 8, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 1, 2, 0, 7, Blocks::farmland->getDefaultState(), Blocks::farmland->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 0, 1, 5, 0, 7, Blocks::farmland->getDefaultState(), Blocks::farmland->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 0, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 0, 0, 6, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 5, 0, 0, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 8, 5, 0, 8, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 0, 1, 3, 0, 7, Blocks::water->getDefaultState(), Blocks::water->getDefaultState(), false);

    for (int i = 1; i <= 7; ++i)
    {
        setBlockState(worldIn, cropTypeA->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 1, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeA->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 2, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeB->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 4, 1, i, structureBoundingBoxIn);
        setBlockState(worldIn, cropTypeB->getStateFromMeta(MathHelper::getRandomIntegerInRange(randomIn, 2, 7)), 5, 1, i, structureBoundingBoxIn);
    }

    for (int k = 0; k < 9; ++k)
    {
        for (int j = 0; j < 7; ++j)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j, 4, k, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::dirt->getDefaultState(), j, -1, k, structureBoundingBoxIn);
        }
    }

    return true;
}

// Hall

std::shared_ptr<Village> Hall::func_175857_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<Hall>(start, list, rand, x, y, z, facing, type, 9, 7, 11);
}

bool Hall::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 7 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 7, 4, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 6, 8, 4, 10, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 0, 6, 8, 0, 10, Blocks::dirt->getDefaultState(), Blocks::dirt->getDefaultState(), false);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 6, 0, 6, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 6, 2, 1, 10, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 1, 6, 8, 1, 10, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 10, 7, 1, 10, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 1, 7, 0, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 0, 3, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 0, 0, 8, 3, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 7, 1, 0, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 5, 7, 1, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 7, 3, 0, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 5, 7, 3, 5, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 1, 8, 4, 1, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 4, 8, 4, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 2, 8, 5, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 0, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 0, 4, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 4, 3, structureBoundingBoxIn);
    int i = getMetadataWithOffset(Blocks::oak_stairs, 3);
    int j = getMetadataWithOffset(Blocks::oak_stairs, 2);

    for (int k = -1; k <= 2; ++k)
    {
        for (int l = 0; l <= 8; ++l)
        {
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(i), l, 4 + k, k, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j), l, 4 + k, 5 - k, structureBoundingBoxIn);
        }
    }

    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 2, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 8, 2, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 8, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 3, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 5, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 6, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 2, 1, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::wooden_pressure_plate->getDefaultState(), 2, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 1, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::oak_stairs, 3)), 2, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::oak_stairs, 1)), 1, 1, 3, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 0, 1, 7, 0, 3, Blocks::double_stone_slab->getDefaultState(), Blocks::double_stone_slab->getDefaultState(), false);
    setBlockState(worldIn, Blocks::double_stone_slab->getDefaultState(), 6, 1, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::double_stone_slab->getDefaultState(), 6, 1, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, *coordBaseMode), 2, 3, 1, structureBoundingBoxIn);
    placeDoorCurrentPosition(worldIn, structureBoundingBoxIn, randomIn, 2, 1, 0, getHorizontal(getMetadataWithOffset(Blocks::oak_door, 1)));

    if (isMaterial(getBlockStateFromPos(worldIn, 2, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, 2, -1, -1, structureBoundingBoxIn), Material::air))
    {
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 2, 0, -1, structureBoundingBoxIn);
    }

    setBlockState(worldIn, Blocks::air->getDefaultState(), 6, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 6, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, getOpposite(*coordBaseMode)), 6, 3, 4, structureBoundingBoxIn);
    placeDoorCurrentPosition(worldIn, structureBoundingBoxIn, randomIn, 6, 1, 5, getHorizontal(getMetadataWithOffset(Blocks::oak_door, 1)));

    for (int i1 = 0; i1 < 5; ++i1)
    {
        for (int j1 = 0; j1 < 9; ++j1)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j1, 7, i1, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), j1, -1, i1, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 4, 1, 2, 2);
    return true;
}

// House1

std::shared_ptr<Village> House1::func_175850_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<House1>(start, list, rand, x, y, z, facing, type, 9, 9, 6);
}

bool House1::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 9 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 7, 5, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 8, 0, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 8, 5, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 6, 1, 8, 6, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 7, 2, 8, 7, 3, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    int i = getMetadataWithOffset(Blocks::oak_stairs, 3);
    int j = getMetadataWithOffset(Blocks::oak_stairs, 2);

    for (int k = -1; k <= 2; ++k)
    {
        for (int l = 0; l <= 8; ++l)
        {
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(i), l, 6 + k, k, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j), l, 6 + k, 5 - k, structureBoundingBoxIn);
        }
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 1, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 5, 8, 1, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 1, 0, 8, 1, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 0, 7, 1, 0, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 0, 0, 4, 0, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 5, 0, 4, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 2, 5, 8, 4, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 2, 0, 8, 4, 0, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 2, 1, 0, 4, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 5, 7, 4, 5, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 2, 1, 8, 4, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 7, 4, 0, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 5, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 6, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 5, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 6, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 3, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 3, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 3, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 3, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 3, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 5, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 6, 2, 5, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 1, 7, 4, 1, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 4, 7, 4, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 4, 7, 3, 4, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 7, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::oak_stairs, 0)), 7, 1, 3, structureBoundingBoxIn);
    int j1 = getMetadataWithOffset(Blocks::oak_stairs, 3);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j1), 6, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j1), 5, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j1), 4, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j1), 3, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 6, 1, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::wooden_pressure_plate->getDefaultState(), 6, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 1, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::wooden_pressure_plate->getDefaultState(), 4, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::crafting_table->getDefaultState(), 7, 1, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 2, 0, structureBoundingBoxIn);
    placeDoorCurrentPosition(worldIn, structureBoundingBoxIn, randomIn, 1, 1, 0, getHorizontal(getMetadataWithOffset(Blocks::oak_door, 1)));

    if (isMaterial(getBlockStateFromPos(worldIn, 1, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, 1, -1, -1, structureBoundingBoxIn), Material::air))
    {
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 1, 0, -1, structureBoundingBoxIn);
    }

    for (int k1 = 0; k1 < 6; ++k1)
    {
        for (int i1 = 0; i1 < 9; ++i1)
        {
            clearCurrentPositionBlocksUpwards(worldIn, i1, 9, k1, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), i1, -1, k1, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 2, 1, 2, 1);
    return true;
}

// House2

std::shared_ptr<Village> House2::func_175855_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<House2>(start, list, rand, x, y, z, facing, type, 10, 6, 7);
}

void House2::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Village::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Chest", hasMadeChest);
}

void House2::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Village::readStructureFromNBT(tagCompound);
    hasMadeChest = tagCompound.getBoolean("Chest");
}

bool House2::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 6 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 9, 4, 6, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 9, 0, 6, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 0, 9, 4, 6, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 0, 9, 5, 6, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 1, 8, 5, 5, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 2, 3, 0, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 4, 0, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 0, 3, 4, 0, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 6, 0, 4, 6, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 3, 3, 1, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 2, 3, 3, 2, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 3, 5, 3, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 3, 5, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 6, 5, 3, 6, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 0, 5, 3, 0, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 1, 0, 9, 3, 0, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, 4, 9, 4, 6, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    setBlockState(worldIn, Blocks::flowing_lava->getDefaultState(), 7, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::flowing_lava->getDefaultState(), 8, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), 9, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), 9, 2, 4, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 2, 4, 8, 2, 5, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 6, 1, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::furnace->getDefaultState(), 6, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::furnace->getDefaultState(), 6, 3, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::double_stone_slab->getDefaultState(), 8, 1, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 2, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 2, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 2, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::wooden_pressure_plate->getDefaultState(), 2, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 1, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::oak_stairs, 3)), 2, 1, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::oak_stairs, 1)), 1, 1, 4, structureBoundingBoxIn);

    if (!hasMadeChest && structureBoundingBoxIn.isVecInside(BlockPos(getXWithOffset(5, 5), getYWithOffset(1), getZWithOffset(5, 5))))
    {
        hasMadeChest = true;
        generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 5, 1, 5, villageBlacksmithChestContents(), 3 + randomIn.nextInt(6));
    }

    for (int i = 6; i <= 8; ++i)
    {
        if (isMaterial(getBlockStateFromPos(worldIn, i, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, i, -1, -1, structureBoundingBoxIn), Material::air))
        {
            setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), i, 0, -1, structureBoundingBoxIn);
        }
    }

    for (int k = 0; k < 7; ++k)
    {
        for (int j = 0; j < 10; ++j)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j, 6, k, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), j, -1, k, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 7, 1, 1, 1);
    return true;
}

// House3

std::shared_ptr<Village> House3::func_175849_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<House3>(start, list, rand, x, y, z, facing, type, 9, 7, 12);
}

bool House3::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 7 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 7, 4, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 1, 6, 8, 4, 10, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 0, 5, 8, 0, 10, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 1, 7, 0, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 0, 3, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 0, 0, 8, 3, 10, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 0, 7, 2, 0, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 5, 2, 1, 5, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 2, 0, 6, 2, 3, 10, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 0, 10, 7, 3, 10, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 0, 7, 3, 0, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 2, 5, 2, 3, 5, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 1, 8, 4, 1, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 4, 3, 4, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 2, 8, 5, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 0, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 0, 4, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 4, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 4, 4, structureBoundingBoxIn);
    int i = getMetadataWithOffset(Blocks::oak_stairs, 3);
    int j = getMetadataWithOffset(Blocks::oak_stairs, 2);

    for (int k = -1; k <= 2; ++k)
    {
        for (int l = 0; l <= 8; ++l)
        {
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(i), l, 4 + k, k, structureBoundingBoxIn);

            if ((k > -1 || l <= 1) && (k > 0 || l <= 3) && (k > 1 || l <= 4 || l >= 6))
            {
                setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(j), l, 4 + k, 5 - k, structureBoundingBoxIn);
            }
        }
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 4, 5, 3, 4, 10, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 7, 4, 2, 7, 4, 10, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 5, 4, 4, 5, 10, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 5, 4, 6, 5, 10, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 6, 3, 5, 6, 10, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    int k1 = getMetadataWithOffset(Blocks::oak_stairs, 0);

    for (int l1 = 4; l1 >= 1; --l1)
    {
        setBlockState(worldIn, Blocks::planks->getDefaultState(), l1, 2 + l1, 7 - l1, structureBoundingBoxIn);

        for (int i1 = 8 - l1; i1 <= 10; ++i1)
        {
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(k1), l1, 2 + l1, i1, structureBoundingBoxIn);
        }
    }

    int i2 = getMetadataWithOffset(Blocks::oak_stairs, 1);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 6, 6, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 7, 5, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(i2), 6, 6, 4, structureBoundingBoxIn);

    for (int j2 = 6; j2 <= 8; ++j2)
    {
        for (int j1 = 5; j1 <= 10; ++j1)
        {
            setBlockState(worldIn, Blocks::oak_stairs->getStateFromMeta(i2), j2, 12 - j2, j1, structureBoundingBoxIn);
        }
    }

    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 2, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 4, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 5, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 6, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 8, 2, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 8, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 2, 5, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 8, 2, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 8, 2, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 8, 2, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 2, 2, 6, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 2, 7, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 2, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 2, 2, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 4, 4, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 5, 4, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 6, 4, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 5, 5, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, *coordBaseMode), 2, 3, 1, structureBoundingBoxIn);
    placeDoorCurrentPosition(worldIn, structureBoundingBoxIn, randomIn, 2, 1, 0, getHorizontal(getMetadataWithOffset(Blocks::oak_door, 1)));
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, -1, 3, 2, -1, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);

    if (isMaterial(getBlockStateFromPos(worldIn, 2, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, 2, -1, -1, structureBoundingBoxIn), Material::air))
    {
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 2, 0, -1, structureBoundingBoxIn);
    }

    for (int k2 = 0; k2 < 5; ++k2)
    {
        for (int i3 = 0; i3 < 9; ++i3)
        {
            clearCurrentPositionBlocksUpwards(worldIn, i3, 7, k2, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), i3, -1, k2, structureBoundingBoxIn);
        }
    }

    for (int l2 = 5; l2 < 11; ++l2)
    {
        for (int j3 = 2; j3 < 9; ++j3)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j3, 7, l2, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), j3, -1, l2, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 4, 1, 2, 2);
    return true;
}

// House4Garden

House4Garden::House4Garden(Start* start, int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing)
    : Village(start, type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
    isRoofAccessible = rand.nextBoolean();
}

void House4Garden::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Village::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Terrace", isRoofAccessible);
}

void House4Garden::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Village::readStructureFromNBT(tagCompound);
    isRoofAccessible = tagCompound.getBoolean("Terrace");
}

std::shared_ptr<Village> House4Garden::func_175858_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<House4Garden>(start, list, rand, x, y, z, facing, type, 5, 6, 5, false);
}

bool House4Garden::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 6 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 0, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 4, 0, 4, 4, 4, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 1, 3, 4, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 0, 3, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 1, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 3, 4, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 3, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 4, 3, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 4, 3, 3, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 2, 2, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 4, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 1, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 1, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 1, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 2, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 3, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 3, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::planks->getDefaultState(), 3, 1, 0, structureBoundingBoxIn);

    if (isMaterial(getBlockStateFromPos(worldIn, 2, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, 2, -1, -1, structureBoundingBoxIn), Material::air))
    {
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 2, 0, -1, structureBoundingBoxIn);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 3, 3, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);

    if (isRoofAccessible)
    {
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 0, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 2, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 3, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 5, 0, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 0, 5, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 5, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 2, 5, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 3, 5, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 5, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 5, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 5, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 5, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 0, 5, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 0, 5, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 0, 5, 3, structureBoundingBoxIn);
    }

    if (isRoofAccessible)
    {
        int i = getMetadataWithOffset(Blocks::ladder, 3);
        setBlockState(worldIn, Blocks::ladder->getStateFromMeta(i), 3, 1, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::ladder->getStateFromMeta(i), 3, 2, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::ladder->getStateFromMeta(i), 3, 3, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::ladder->getStateFromMeta(i), 3, 4, 3, structureBoundingBoxIn);
    }

    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, *coordBaseMode), 2, 3, 1, structureBoundingBoxIn);

    for (int k = 0; k < 5; ++k)
    {
        for (int j = 0; j < 5; ++j)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j, 6, k, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), j, -1, k, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 1, 1, 2, 1);
    return true;
}

// WoodHut

WoodHut::WoodHut(Start* start, int type, Random& rand, const StructureBoundingBox& structurebb, EnumFacing facing)
    : Village(start, type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
    isTallHouse = rand.nextBoolean();
    tablePosition = rand.nextInt(3);
}

void WoodHut::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Village::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("T", tablePosition);
    tagCompound.setBoolean("C", isTallHouse);
}

void WoodHut::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Village::readStructureFromNBT(tagCompound);
    tablePosition = tagCompound.getInteger("T");
    isTallHouse = tagCompound.getBoolean("C");
}

std::shared_ptr<Village> WoodHut::func_175853_a(Start* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    return findPiece<WoodHut>(start, list, rand, x, y, z, facing, type, 4, 6, 5);
}

bool WoodHut::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 6 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 3, 5, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 3, 0, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 1, 2, 0, 3, Blocks::dirt->getDefaultState(), Blocks::dirt->getDefaultState(), false);

    if (isTallHouse)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 1, 2, 4, 3, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    }
    else
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 1, 2, 5, 3, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    }

    setBlockState(worldIn, Blocks::log->getDefaultState(), 1, 4, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 2, 4, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 1, 4, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 2, 4, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 4, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 0, 4, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 3, 4, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 3, 4, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::log->getDefaultState(), 3, 4, 3, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 0, 0, 3, 0, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 0, 3, 3, 0, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 4, 0, 3, 4, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 4, 3, 3, 4, Blocks::log->getDefaultState(), Blocks::log->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 3, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 1, 3, 3, 3, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 0, 2, 3, 0, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 4, 2, 3, 4, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 0, 2, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::glass_pane->getDefaultState(), 3, 2, 2, structureBoundingBoxIn);

    if (tablePosition > 0)
    {
        setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), tablePosition, 1, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::wooden_pressure_plate->getDefaultState(), tablePosition, 2, 3, structureBoundingBoxIn);
    }

    setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 1, 2, 0, structureBoundingBoxIn);
    placeDoorCurrentPosition(worldIn, structureBoundingBoxIn, randomIn, 1, 1, 0, getHorizontal(getMetadataWithOffset(Blocks::oak_door, 1)));

    if (isMaterial(getBlockStateFromPos(worldIn, 1, 0, -1, structureBoundingBoxIn), Material::air) && !isMaterial(getBlockStateFromPos(worldIn, 1, -1, -1, structureBoundingBoxIn), Material::air))
    {
        setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(getMetadataWithOffset(Blocks::stone_stairs, 3)), 1, 0, -1, structureBoundingBoxIn);
    }

    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            clearCurrentPositionBlocksUpwards(worldIn, j, 6, i, structureBoundingBoxIn);
            replaceAirAndLiquidDownwards(worldIn, Blocks::cobblestone->getDefaultState(), j, -1, i, structureBoundingBoxIn);
        }
    }

    spawnVillagers(worldIn, structureBoundingBoxIn, 1, 1, 2, 1);
    return true;
}

// Torch

std::optional<StructureBoundingBox> Torch::func_175856_a(Start*, StructureComponentList& list, Random&, int x, int y, int z, EnumFacing facing)
{
    const auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(x, y, z, 0, 0, 0, 3, 4, 2, facing);
    if (findIntersecting(list, structureboundingbox))
        return std::nullopt;
    return structureboundingbox;
}

bool Torch::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 4 - 1, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 2, 3, 1, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 0, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 1, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 2, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::wool->getStateFromMeta(getDyeDamage(EnumDyeColor::WHITE)), 1, 3, 0, structureBoundingBoxIn);
    bool flag = isMode(EnumFacing::EAST) || isMode(EnumFacing::NORTH);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, rotateY(*coordBaseMode)), flag ? 2 : 0, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, *coordBaseMode), 1, 3, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, rotateYCCW(*coordBaseMode)), flag ? 0 : 2, 3, 0, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::torch->getDefaultState()->withProperty(BlockTorch::FACING, getOpposite(*coordBaseMode)), 1, 3, -1, structureBoundingBoxIn);
    return true;
}

// --- Path ---

Path::Path(Start* start, int type, Random&, const StructureBoundingBox& structurebb, EnumFacing facing)
    : Road(start, type)
{
    coordBaseMode = facing;
    boundingBox = structurebb;
    length = std::max(structurebb.getXSize(), structurebb.getZSize());
}

void Path::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Village::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("Length", length);
}

void Path::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Village::readStructureFromNBT(tagCompound);
    length = tagCompound.getInteger("Length");
}

void Path::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    Start* start = static_cast<Start*>(componentIn);
    bool flag = false;
    for (int i = rand.nextInt(5); i < length - 8; i += 2 + rand.nextInt(5)) {
        if (const auto structurecomponent = getNextComponentNN(start, listIn, rand, 0, i)) {
            i += std::max(structurecomponent->getBoundingBox().getXSize(), structurecomponent->getBoundingBox().getZSize());
            flag = true;
        }
    }
    for (int j = rand.nextInt(5); j < length - 8; j += 2 + rand.nextInt(5)) {
        if (const auto structurecomponent1 = getNextComponentPP(start, listIn, rand, 0, j)) {
            j += std::max(structurecomponent1->getBoundingBox().getXSize(), structurecomponent1->getBoundingBox().getZSize());
            flag = true;
        }
    }
    if (flag && rand.nextInt(3) > 0 && coordBaseMode) {
        switch (*coordBaseMode) {
        case EnumFacing::NORTH:
            func_176069_e(start, listIn, rand, boundingBox.minX - 1, boundingBox.minY, boundingBox.minZ, EnumFacing::WEST, getComponentType());
            break;
        case EnumFacing::SOUTH:
            func_176069_e(start, listIn, rand, boundingBox.minX - 1, boundingBox.minY, boundingBox.maxZ - 2, EnumFacing::WEST, getComponentType());
            break;
        case EnumFacing::WEST:
            func_176069_e(start, listIn, rand, boundingBox.minX, boundingBox.minY, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType());
            break;
        case EnumFacing::EAST:
            func_176069_e(start, listIn, rand, boundingBox.maxX - 2, boundingBox.minY, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType());
            break;
        default:
            break;
        }
    }
    if (flag && rand.nextInt(3) > 0 && coordBaseMode) {
        switch (*coordBaseMode) {
        case EnumFacing::NORTH:
            func_176069_e(start, listIn, rand, boundingBox.maxX + 1, boundingBox.minY, boundingBox.minZ, EnumFacing::EAST, getComponentType());
            break;
        case EnumFacing::SOUTH:
            func_176069_e(start, listIn, rand, boundingBox.maxX + 1, boundingBox.minY, boundingBox.maxZ - 2, EnumFacing::EAST, getComponentType());
            break;
        case EnumFacing::WEST:
            func_176069_e(start, listIn, rand, boundingBox.minX, boundingBox.minY, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType());
            break;
        case EnumFacing::EAST:
            func_176069_e(start, listIn, rand, boundingBox.maxX - 2, boundingBox.minY, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType());
            break;
        default:
            break;
        }
    }
}

std::optional<StructureBoundingBox> Path::func_175848_a(Start*, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing)
{
    for (int i = 7 * MathHelper::getRandomIntegerInRange(rand, 3, 5); i >= 7; i -= 7) {
        const auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(x, y, z, 0, 0, 0, 3, 3, i, facing);
        if (!findIntersecting(list, structureboundingbox))
            return structureboundingbox;
    }
    return std::nullopt;
}

bool Path::addComponentParts(World* worldIn, Random&, const StructureBoundingBox& structureBoundingBoxIn)
{
    const IBlockState* iblockstate = func_175847_a(Blocks::gravel->getDefaultState());
    const IBlockState* iblockstate1 = func_175847_a(Blocks::cobblestone->getDefaultState());
    for (int i = boundingBox.minX; i <= boundingBox.maxX; ++i) {
        for (int j = boundingBox.minZ; j <= boundingBox.maxZ; ++j) {
            BlockPos blockpos(i, 64, j);
            if (structureBoundingBoxIn.isVecInside(blockpos)) {
                blockpos = worldIn->getTopSolidOrLiquidBlock(blockpos).down();
                worldIn->setBlockState(blockpos, iblockstate, 2);
                worldIn->setBlockState(blockpos.down(), iblockstate1, 2);
            }
        }
    }
    return true;
}

// --- Well and Start ---

Well::Well(Start* start, int type, Random& rand, int x, int z)
    : Village(start, type)
{
    coordBaseMode = randomFacing(Plane::HORIZONTAL, rand);
    // Java switches on the facing but builds the same box either way.
    boundingBox = StructureBoundingBox(x, 64, z, x + 6 - 1, 78, z + 6 - 1);
}

void Well::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    Start* start = static_cast<Start*>(componentIn);
    func_176069_e(start, listIn, rand, boundingBox.minX - 1, boundingBox.maxY - 4, boundingBox.minZ + 1, EnumFacing::WEST, getComponentType());
    func_176069_e(start, listIn, rand, boundingBox.maxX + 1, boundingBox.maxY - 4, boundingBox.minZ + 1, EnumFacing::EAST, getComponentType());
    func_176069_e(start, listIn, rand, boundingBox.minX + 1, boundingBox.maxY - 4, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType());
    func_176069_e(start, listIn, rand, boundingBox.minX + 1, boundingBox.maxY - 4, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType());
}

bool Well::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (field_143015_k < 0)
    {
        field_143015_k = getAverageGroundLevel(worldIn, structureBoundingBoxIn);

        if (field_143015_k < 0)
        {
            return true;
        }

        boundingBox.offset(0, field_143015_k - boundingBox.maxY + 3, 0);
    }

    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 0, 1, 4, 12, 4, Blocks::cobblestone->getDefaultState(), Blocks::flowing_water->getDefaultState(), false);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 12, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 3, 12, 2, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 2, 12, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::air->getDefaultState(), 3, 12, 3, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 13, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 14, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 13, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 14, 1, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 13, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 1, 14, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 13, 4, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 4, 14, 4, structureBoundingBoxIn);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 15, 1, 4, 15, 4, Blocks::cobblestone->getDefaultState(), Blocks::cobblestone->getDefaultState(), false);

    for (int i = 0; i <= 5; ++i)
    {
        for (int j = 0; j <= 5; ++j)
        {
            if (j == 0 || j == 5 || i == 0 || i == 5)
            {
                setBlockState(worldIn, Blocks::gravel->getDefaultState(), j, 11, i, structureBoundingBoxIn);
                clearCurrentPositionBlocksUpwards(worldIn, j, 12, i, structureBoundingBoxIn);
            }
        }
    }

    return true;
}

Start::Start(WorldChunkManager* chunkManagerIn, int type, Random& rand, int x, int z, PieceWeightList pieces, int size)
    : Well(nullptr, 0, rand, x, z)
    , worldChunkMngr(chunkManagerIn)
    , terrainType(size)
    , structureVillageWeightedPieceList(std::move(pieces))
{
    BiomeGenBase* biomegenbase = chunkManagerIn->getBiomeGenerator(BlockPos(x, 0, z), BiomeGenBase::field_180279_ad);
    inDesert = biomegenbase == BiomeGenBase::desert || biomegenbase == BiomeGenBase::desertHills;
    func_175846_a(inDesert);
}

} // namespace StructureVillagePieces

// --- MapGenVillage ---

const std::vector<BiomeGenBase*>& MapGenVillage::villageSpawnBiomes()
{
    static const std::vector<BiomeGenBase*> list { BiomeGenBase::plains, BiomeGenBase::desert, BiomeGenBase::savanna };
    return list;
}

MapGenVillage::MapGenVillage(const FlatGeneratorInfo::Options& options)
    : MapGenVillage()
{
    for (const auto& [key, value] : options) {
        if (key == "size")
            terrainType = MathHelper::parseIntWithDefaultAndMax(value, terrainType, 0);
        else if (key == "distance")
            field_82665_g = MathHelper::parseIntWithDefaultAndMax(value, field_82665_g, field_82666_h + 1);
    }
}

bool MapGenVillage::canSpawnStructureAtCoords(int chunkX, int chunkZ)
{
    const int i = chunkX;
    const int j = chunkZ;
    if (chunkX < 0)
        chunkX -= field_82665_g - 1;
    if (chunkZ < 0)
        chunkZ -= field_82665_g - 1;
    int k = chunkX / field_82665_g;
    int l = chunkZ / field_82665_g;
    Random& random = worldObj->setRandomSeed(k, l, 10387312);
    k = k * field_82665_g;
    l = l * field_82665_g;
    k = k + random.nextInt(field_82665_g - field_82666_h);
    l = l + random.nextInt(field_82665_g - field_82666_h);
    if (i == k && j == l)
        return worldObj->getWorldChunkManager()->areBiomesViable(i * 16 + 8, j * 16 + 8, 0, villageSpawnBiomes());
    return false;
}

std::shared_ptr<StructureStart> MapGenVillage::getStructureStart(int chunkX, int chunkZ)
{
    return std::make_shared<Start>(worldObj, rand, chunkX, chunkZ, terrainType);
}

MapGenVillage::Start::Start(World* worldIn, Random& rand, int x, int z, int size)
    : StructureStart(x, z)
{
    using namespace StructureVillagePieces;
    PieceWeightList list = getStructureVillageWeightedPieceList(rand, size);
    auto start = std::make_shared<StructureVillagePieces::Start>(worldIn->getWorldChunkManager(), 0, rand, (x << 4) + 2, (z << 4) + 2, std::move(list), size);
    components.push_back(start);
    start->buildComponent(start.get(), components, rand);
    StructureComponentList& list1 = start->field_74930_j;
    StructureComponentList& list2 = start->field_74932_i;
    while (!list1.empty() || !list2.empty()) {
        if (list1.empty()) {
            const int i = rand.nextInt(static_cast<int>(list2.size()));
            const std::shared_ptr<StructureComponent> structurecomponent = list2[static_cast<size_t>(i)];
            list2.erase(list2.begin() + i);
            structurecomponent->buildComponent(start.get(), components, rand);
        } else {
            const int j = rand.nextInt(static_cast<int>(list1.size()));
            const std::shared_ptr<StructureComponent> structurecomponent2 = list1[static_cast<size_t>(j)];
            list1.erase(list1.begin() + j);
            structurecomponent2->buildComponent(start.get(), components, rand);
        }
    }
    updateBoundingBox();
    int k = 0;
    for (const auto& structurecomponent1 : components) {
        if (!dynamic_cast<const Road*>(structurecomponent1.get()))
            ++k;
    }
    hasMoreThanTwoComponents = k > 2;
}

void MapGenVillage::Start::writeToNBT(NBTTagCompound& tagCompound) const
{
    tagCompound.setBoolean("Valid", hasMoreThanTwoComponents);
}

void MapGenVillage::Start::readFromNBT(const NBTTagCompound& tagCompound)
{
    hasMoreThanTwoComponents = tagCompound.getBoolean("Valid");
}

} // namespace mc
