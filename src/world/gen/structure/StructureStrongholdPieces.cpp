#include "world/gen/structure/StructureStrongholdPieces.h"

#include "block/AllBlocks.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemArmor.h"
#include "item/ItemMisc.h"
#include "nbt/NBTTagCompound.h"
#include "tileentity/TileEntities.h"
#include "util/MathHelper.h"
#include "util/Random.h"
#include "world/World.h"
#include "world/biome/BiomeGenBase.h"
#include "world/biome/WorldChunkManager.h"
#include "world/gen/structure/MapGenStronghold.h"
#include "world/gen/structure/MapGenStructure.h"

#include "core/Java.h"
#include "core/JavaMath.h"

#include <algorithm>
#include <array>
#include <cstdlib>

namespace mc {

namespace StructureStrongholdPieces {

namespace {

// Java: StructureStrongholdPieces.Stones -- the mix of stone bricks in the walls.
class Stones : public StructureComponent::BlockSelector {
public:
    void selectBlocks(Random& rand, int, int, int, bool wall) override
    {
        if (wall) {
            const float f = rand.nextFloat();
            if (f < 0.2f)
                blockstate = Blocks::stonebrick->getStateFromMeta(BlockStoneBrick::CRACKED_META);
            else if (f < 0.5f)
                blockstate = Blocks::stonebrick->getStateFromMeta(BlockStoneBrick::MOSSY_META);
            else if (f < 0.55f)
                blockstate = Blocks::monster_egg->getStateFromMeta(static_cast<int>(BlockSilverfish::EnumType::STONEBRICK));
            else
                blockstate = Blocks::stonebrick->getDefaultState();
        } else {
            blockstate = Blocks::air->getDefaultState();
        }
    }
};

Stones strongholdStones;

std::array<PieceWeight, 11> pieceWeightArray {
    PieceWeight(&Straight::func_175862_a, 40, 0),
    PieceWeight(&Prison::func_175860_a, 5, 5),
    PieceWeight(&LeftTurn::func_175867_a, 20, 0),
    PieceWeight(&RightTurn::func_175867_a, 20, 0),
    PieceWeight(&RoomCrossing::func_175859_a, 10, 6),
    PieceWeight(&StairsStraight::func_175861_a, 5, 5),
    PieceWeight(&Stairs::func_175863_a, 5, 5),
    PieceWeight(&Crossing::func_175866_a, 5, 4),
    PieceWeight(&ChestCorridor::func_175868_a, 5, 4),
    PieceWeight(&Library::func_175864_a, 10, 2, 4),
    PieceWeight(&PortalRoom::func_175865_a, 20, 1, 5),
};

std::vector<PieceWeight*> structurePieceList;
PieceWeight::Creator strongComponentType = nullptr;
int totalWeight = 0;

const std::vector<WeightedRandomChestContent>& strongholdChestContents()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::ender_pearl, 0, 1, 1, 10 },
        { Items::diamond, 0, 1, 3, 3 },
        { Items::iron_ingot, 0, 1, 5, 10 },
        { Items::gold_ingot, 0, 1, 3, 5 },
        { Items::redstone, 0, 4, 9, 5 },
        { Items::bread, 0, 1, 3, 15 },
        { Items::apple, 0, 1, 3, 15 },
        { Items::iron_pickaxe, 0, 1, 1, 5 },
        { Items::iron_sword, 0, 1, 1, 5 },
        { Items::iron_chestplate, 0, 1, 1, 5 },
        { Items::iron_helmet, 0, 1, 1, 5 },
        { Items::iron_leggings, 0, 1, 1, 5 },
        { Items::iron_boots, 0, 1, 1, 5 },
        { Items::golden_apple, 0, 1, 1, 1 },
        { Items::saddle, 0, 1, 1, 1 },
        { Items::iron_horse_armor, 0, 1, 1, 1 },
        { Items::golden_horse_armor, 0, 1, 1, 1 },
        { Items::diamond_horse_armor, 0, 1, 1, 1 },
    };
    return list;
}

const std::vector<WeightedRandomChestContent>& strongholdLibraryChestContents()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::book, 0, 1, 3, 20 },
        { Items::paper, 0, 2, 7, 20 },
        { Items::map, 0, 1, 1, 1 },
        { Items::compass, 0, 1, 1, 1 },
    };
    return list;
}

const std::vector<WeightedRandomChestContent>& strongholdRoomCrossingChestContents()
{
    static const std::vector<WeightedRandomChestContent> list {
        { Items::iron_ingot, 0, 1, 5, 10 },
        { Items::gold_ingot, 0, 1, 3, 5 },
        { Items::redstone, 0, 4, 9, 5 },
        { Items::coal, 0, 3, 8, 10 },
        { Items::bread, 0, 1, 3, 15 },
        { Items::apple, 0, 1, 3, 15 },
        { Items::iron_pickaxe, 0, 1, 1, 1 },
    };
    return list;
}

bool canAddStructurePieces()
{
    bool flag = false;
    totalWeight = 0;
    for (const PieceWeight* weight : structurePieceList) {
        if (weight->instancesLimit > 0 && weight->instancesSpawned < weight->instancesLimit)
            flag = true;
        totalWeight += weight->pieceWeight;
    }
    return flag;
}

std::shared_ptr<Stronghold> func_175955_b(Stairs2* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    if (!canAddStructurePieces())
        return nullptr;
    if (strongComponentType) {
        // Java: func_175954_a dispatches on the piece class.
        std::shared_ptr<Stronghold> stronghold = strongComponentType(list, rand, x, y, z, facing, type);
        strongComponentType = nullptr;
        if (stronghold)
            return stronghold;
    }
    int j = 0;
    while (j < 5) {
        ++j;
        int i = rand.nextInt(totalWeight);
        for (PieceWeight* weight : structurePieceList) {
            i -= weight->pieceWeight;
            if (i < 0) {
                if (!weight->canSpawnMoreStructuresOfType(type) || weight == start->strongholdPieceWeight)
                    break;
                std::shared_ptr<Stronghold> stronghold1 = weight->pieceClass(list, rand, x, y, z, facing, type);
                if (stronghold1) {
                    ++weight->instancesSpawned;
                    start->strongholdPieceWeight = weight;
                    if (!weight->canSpawnMoreStructures())
                        std::erase(structurePieceList, weight);
                    return stronghold1;
                }
            }
        }
    }
    const auto structureboundingbox = Corridor::func_175869_a(list, rand, x, y, z, facing);
    if (structureboundingbox && structureboundingbox->minY > 1)
        return std::make_shared<Corridor>(type, rand, *structureboundingbox, facing);
    return nullptr;
}

std::shared_ptr<StructureComponent> func_175953_c(Stairs2* start, StructureComponentList& list, Random& rand, int x, int y, int z, EnumFacing facing, int type)
{
    if (type > 50)
        return nullptr;
    if (std::abs(x - start->getBoundingBox().minX) <= 112 && std::abs(z - start->getBoundingBox().minZ) <= 112) {
        std::shared_ptr<StructureComponent> structurecomponent = func_175955_b(start, list, rand, x, y, z, facing, type + 1);
        if (structurecomponent) {
            list.push_back(structurecomponent);
            start->field_75026_c.push_back(structurecomponent);
        }
        return structurecomponent;
    }
    return nullptr;
}

std::string_view doorName(Stronghold::Door door)
{
    switch (door) {
    case Stronghold::Door::WOOD_DOOR: return "WOOD_DOOR";
    case Stronghold::Door::GRATES: return "GRATES";
    case Stronghold::Door::IRON_DOOR: return "IRON_DOOR";
    default: return "OPENING";
    }
}

} // namespace

void registerStrongholdPieces()
{
    MapGenStructureIO::registerStructureComponent("SHCC", [] { return std::make_shared<ChestCorridor>(); });
    MapGenStructureIO::registerStructureComponent("SHFC", [] { return std::make_shared<Corridor>(); });
    MapGenStructureIO::registerStructureComponent("SH5C", [] { return std::make_shared<Crossing>(); });
    MapGenStructureIO::registerStructureComponent("SHLT", [] { return std::make_shared<LeftTurn>(); });
    MapGenStructureIO::registerStructureComponent("SHLi", [] { return std::make_shared<Library>(); });
    MapGenStructureIO::registerStructureComponent("SHPR", [] { return std::make_shared<PortalRoom>(); });
    MapGenStructureIO::registerStructureComponent("SHPH", [] { return std::make_shared<Prison>(); });
    MapGenStructureIO::registerStructureComponent("SHRT", [] { return std::make_shared<RightTurn>(); });
    MapGenStructureIO::registerStructureComponent("SHRC", [] { return std::make_shared<RoomCrossing>(); });
    MapGenStructureIO::registerStructureComponent("SHSD", [] { return std::make_shared<Stairs>(); });
    MapGenStructureIO::registerStructureComponent("SHStart", [] { return std::make_shared<Stairs2>(); });
    MapGenStructureIO::registerStructureComponent("SHS", [] { return std::make_shared<Straight>(); });
    MapGenStructureIO::registerStructureComponent("SHSSD", [] { return std::make_shared<StairsStraight>(); });
}

void prepareStructurePieces()
{
    structurePieceList.clear();
    for (PieceWeight& weight : pieceWeightArray) {
        weight.instancesSpawned = 0;
        structurePieceList.push_back(&weight);
    }
    strongComponentType = nullptr;
}

// --- Stronghold ---

void Stronghold::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    tagCompound.setString("EntryDoor", std::string(doorName(field_143013_d)));
}

void Stronghold::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    // Java: Door.valueOf, which throws for an unknown name.
    const std::string name = tagCompound.getString("EntryDoor");
    if (name == "WOOD_DOOR")
        field_143013_d = Door::WOOD_DOOR;
    else if (name == "GRATES")
        field_143013_d = Door::GRATES;
    else if (name == "IRON_DOOR")
        field_143013_d = Door::IRON_DOOR;
    else
        field_143013_d = Door::OPENING;
}

Stronghold::Door Stronghold::getRandomDoor(Random& rand)
{
    switch (rand.nextInt(5)) {
    case 2: return Door::WOOD_DOOR;
    case 3: return Door::GRATES;
    case 4: return Door::IRON_DOOR;
    default: return Door::OPENING;
    }
}

// --- Stairs2 ---

Stairs2::Stairs2(int, Random& rand, int x, int z)
    : Stairs(0, rand, x, z)
{
}

BlockPos Stairs2::getBoundingBoxCenter() const
{
    return strongholdPortalRoom ? strongholdPortalRoom->getBoundingBoxCenter() : Stairs::getBoundingBoxCenter();
}

// --- the pieces ---

ChestCorridor::ChestCorridor(int p_i45582_1_, Random& p_i45582_2_, const StructureBoundingBox& p_i45582_3_, EnumFacing p_i45582_4_)
    : Stronghold(p_i45582_1_)
{
    coordBaseMode = p_i45582_4_;
    field_143013_d = getRandomDoor(p_i45582_2_);
    boundingBox = p_i45582_3_;
}

void ChestCorridor::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Chest", hasMadeChest);
}

void ChestCorridor::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    hasMadeChest = tagCompound.getBoolean("Chest");
}

void ChestCorridor::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
}

std::shared_ptr<Stronghold> ChestCorridor::func_175868_a(StructureComponentList& p_175868_0_, Random& p_175868_1_, int p_175868_2_, int p_175868_3_, int p_175868_4_, EnumFacing p_175868_5_, int p_175868_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175868_2_, p_175868_3_, p_175868_4_, -1, -1, 0, 5, 5, 7, p_175868_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175868_0_, structureboundingbox) == nullptr ? std::make_shared<ChestCorridor>(p_175868_6_, p_175868_1_, structureboundingbox, p_175868_5_) : nullptr;
}

bool ChestCorridor::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 4, 6, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 1, 0);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, Door::OPENING, 1, 1, 6);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, 2, 3, 1, 4, Blocks::stonebrick->getDefaultState(), Blocks::stonebrick->getDefaultState(), false);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SMOOTHBRICK)), 3, 1, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SMOOTHBRICK)), 3, 1, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SMOOTHBRICK)), 3, 2, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SMOOTHBRICK)), 3, 2, 4, structureBoundingBoxIn);
        for (int i = 2; i <= 4; ++i)
        {
            setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::SMOOTHBRICK)), 2, 1, i, structureBoundingBoxIn);
        }
        if (!hasMadeChest && structureBoundingBoxIn.isVecInside(BlockPos(getXWithOffset(3, 3), getYWithOffset(2), getZWithOffset(3, 3))))
        {
            hasMadeChest = true;
            // Java evaluates the book before the count.
            const auto chestList1 = WeightedRandomChestContent::func_177629_a(strongholdChestContents(), { Items::enchanted_book->getRandom(randomIn) });
            const int chestMax1 = 2 + randomIn.nextInt(2);
            generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 3, 2, 3, chestList1, chestMax1);
        }
        return true;
    }
}

Corridor::Corridor(int p_i45581_1_, Random& p_i45581_2_, const StructureBoundingBox& p_i45581_3_, EnumFacing p_i45581_4_)
    : Stronghold(p_i45581_1_)
{
    coordBaseMode = p_i45581_4_;
    boundingBox = p_i45581_3_;
    field_74993_a = p_i45581_4_ != EnumFacing::NORTH && p_i45581_4_ != EnumFacing::SOUTH ? p_i45581_3_.getXSize() : p_i45581_3_.getZSize();
}

void Corridor::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("Steps", field_74993_a);
}

void Corridor::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    field_74993_a = tagCompound.getInteger("Steps");
}

std::optional<StructureBoundingBox> Corridor::func_175869_a(StructureComponentList& p_175869_0_, Random& p_175869_1_, int p_175869_2_, int p_175869_3_, int p_175869_4_, EnumFacing p_175869_5_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175869_2_, p_175869_3_, p_175869_4_, -1, -1, 0, 5, 5, 4, p_175869_5_);
    auto structurecomponent = findIntersecting(p_175869_0_, structureboundingbox);
    if (structurecomponent == nullptr)
    {
        return std::nullopt;
    }
    else
    {
        if (structurecomponent->getBoundingBox().minY == structureboundingbox.minY)
        {
            for (int j = 3; j >= 1; --j)
            {
                structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175869_2_, p_175869_3_, p_175869_4_, -1, -1, 0, 5, 5, j - 1, p_175869_5_);
                if (!structurecomponent->getBoundingBox().intersectsWith(structureboundingbox))
                {
                    return StructureBoundingBox::getComponentToAddBoundingBox(p_175869_2_, p_175869_3_, p_175869_4_, -1, -1, 0, 5, 5, j, p_175869_5_);
                }
            }
        }
        return std::nullopt;
    }
}

bool Corridor::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        for (int i = 0; i < field_74993_a; ++i)
        {
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 0, 0, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 0, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 2, 0, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 0, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 4, 0, i, structureBoundingBoxIn);
            for (int j = 1; j <= 3; ++j)
            {
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 0, j, i, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::air->getDefaultState(), 1, j, i, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::air->getDefaultState(), 2, j, i, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::air->getDefaultState(), 3, j, i, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 4, j, i, structureBoundingBoxIn);
            }
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 0, 4, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 4, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 2, 4, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 4, i, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 4, 4, i, structureBoundingBoxIn);
        }
        return true;
    }
}

Crossing::Crossing(int p_i45580_1_, Random& p_i45580_2_, const StructureBoundingBox& p_i45580_3_, EnumFacing p_i45580_4_)
    : Stronghold(p_i45580_1_)
{
    coordBaseMode = p_i45580_4_;
    field_143013_d = getRandomDoor(p_i45580_2_);
    boundingBox = p_i45580_3_;
    field_74996_b = p_i45580_2_.nextBoolean();
    field_74997_c = p_i45580_2_.nextBoolean();
    field_74995_d = p_i45580_2_.nextBoolean();
    field_74999_h = p_i45580_2_.nextInt(3) > 0;
}

void Crossing::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("leftLow", field_74996_b);
    tagCompound.setBoolean("leftHigh", field_74997_c);
    tagCompound.setBoolean("rightLow", field_74995_d);
    tagCompound.setBoolean("rightHigh", field_74999_h);
}

void Crossing::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    field_74996_b = tagCompound.getBoolean("leftLow");
    field_74997_c = tagCompound.getBoolean("leftHigh");
    field_74995_d = tagCompound.getBoolean("rightLow");
    field_74999_h = tagCompound.getBoolean("rightHigh");
}

void Crossing::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    int i = 3;
    int j = 5;
    if (isMode(EnumFacing::WEST) || isMode(EnumFacing::NORTH))
    {
        i = 8 - i;
        j = 8 - j;
    }
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 5, 1);
    if (field_74996_b)
    {
        getNextComponentX(static_cast<Stairs2*>(componentIn), listIn, rand, i, 1);
    }
    if (field_74997_c)
    {
        getNextComponentX(static_cast<Stairs2*>(componentIn), listIn, rand, j, 7);
    }
    if (field_74995_d)
    {
        getNextComponentZ(static_cast<Stairs2*>(componentIn), listIn, rand, i, 1);
    }
    if (field_74999_h)
    {
        getNextComponentZ(static_cast<Stairs2*>(componentIn), listIn, rand, j, 7);
    }
}

std::shared_ptr<Stronghold> Crossing::func_175866_a(StructureComponentList& p_175866_0_, Random& p_175866_1_, int p_175866_2_, int p_175866_3_, int p_175866_4_, EnumFacing p_175866_5_, int p_175866_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175866_2_, p_175866_3_, p_175866_4_, -4, -3, 0, 10, 9, 11, p_175866_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175866_0_, structureboundingbox) == nullptr ? std::make_shared<Crossing>(p_175866_6_, p_175866_1_, structureboundingbox, p_175866_5_) : nullptr;
}

bool Crossing::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 9, 8, 10, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 4, 3, 0);
        if (field_74996_b)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, 1, 0, 5, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        if (field_74995_d)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 3, 1, 9, 5, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        if (field_74997_c)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 5, 7, 0, 7, 9, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        if (field_74999_h)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 5, 7, 9, 7, 9, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 10, 7, 3, 10, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 2, 1, 8, 2, 6, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 5, 4, 4, 9, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 8, 1, 5, 8, 4, 9, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 4, 7, 3, 4, 9, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 3, 5, 3, 3, 6, false, randomIn, strongholdStones);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 3, 4, 3, 3, 4, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 4, 6, 3, 4, 6, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 5, 1, 7, 7, 1, 8, false, randomIn, strongholdStones);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 9, 7, 1, 9, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 2, 7, 7, 2, 7, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 5, 7, 4, 5, 9, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 8, 5, 7, 8, 5, 9, Blocks::stone_slab->getDefaultState(), Blocks::stone_slab->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 5, 7, 7, 5, 9, Blocks::double_stone_slab->getDefaultState(), Blocks::double_stone_slab->getDefaultState(), false);
        setBlockState(worldIn, Blocks::torch->getDefaultState(), 6, 5, 6, structureBoundingBoxIn);
        return true;
    }
}

LeftTurn::LeftTurn(int p_i45579_1_, Random& p_i45579_2_, const StructureBoundingBox& p_i45579_3_, EnumFacing p_i45579_4_)
    : Stronghold(p_i45579_1_)
{
    coordBaseMode = p_i45579_4_;
    field_143013_d = getRandomDoor(p_i45579_2_);
    boundingBox = p_i45579_3_;
}

void LeftTurn::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    if (!isMode(EnumFacing::NORTH) && !isMode(EnumFacing::EAST))
    {
        getNextComponentZ(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
    }
    else
    {
        getNextComponentX(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
    }
}

std::shared_ptr<Stronghold> LeftTurn::func_175867_a(StructureComponentList& p_175867_0_, Random& p_175867_1_, int p_175867_2_, int p_175867_3_, int p_175867_4_, EnumFacing p_175867_5_, int p_175867_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175867_2_, p_175867_3_, p_175867_4_, -1, -1, 0, 5, 5, 5, p_175867_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175867_0_, structureboundingbox) == nullptr ? std::make_shared<LeftTurn>(p_175867_6_, p_175867_1_, structureboundingbox, p_175867_5_) : nullptr;
}

bool LeftTurn::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 4, 4, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 1, 0);
        if (!isMode(EnumFacing::NORTH) && !isMode(EnumFacing::EAST))
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 4, 3, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 3, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        return true;
    }
}

Library::Library(int p_i45578_1_, Random& p_i45578_2_, const StructureBoundingBox& p_i45578_3_, EnumFacing p_i45578_4_)
    : Stronghold(p_i45578_1_)
{
    coordBaseMode = p_i45578_4_;
    field_143013_d = getRandomDoor(p_i45578_2_);
    boundingBox = p_i45578_3_;
    isLargeRoom = p_i45578_3_.getYSize() > 6;
}

void Library::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Tall", isLargeRoom);
}

void Library::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    isLargeRoom = tagCompound.getBoolean("Tall");
}

std::shared_ptr<Stronghold> Library::func_175864_a(StructureComponentList& p_175864_0_, Random& p_175864_1_, int p_175864_2_, int p_175864_3_, int p_175864_4_, EnumFacing p_175864_5_, int p_175864_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175864_2_, p_175864_3_, p_175864_4_, -4, -1, 0, 14, 11, 15, p_175864_5_);
    if (!canStrongholdGoDeeper(structureboundingbox) || findIntersecting(p_175864_0_, structureboundingbox) != nullptr)
    {
        structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175864_2_, p_175864_3_, p_175864_4_, -4, -1, 0, 14, 6, 15, p_175864_5_);
        if (!canStrongholdGoDeeper(structureboundingbox) || findIntersecting(p_175864_0_, structureboundingbox) != nullptr)
        {
            return nullptr;
        }
    }
    return std::make_shared<Library>(p_175864_6_, p_175864_1_, structureboundingbox, p_175864_5_);
}

bool Library::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        int i = 11;
        if (!isLargeRoom)
        {
            i = 6;
        }
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 13, i - 1, 14, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 4, 1, 0);
        func_175805_a(worldIn, structureBoundingBoxIn, randomIn, 0.07f, 2, 1, 1, 11, 4, 13, Blocks::web->getDefaultState(), Blocks::web->getDefaultState(), false);
        int j = 1;
        int k = 12;
        for (int l = 1; l <= 13; ++l)
        {
            if ((l - 1) % 4 == 0)
            {
                fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, l, 1, 4, l, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
                fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, l, 12, 4, l, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 2, 3, l, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 11, 3, l, structureBoundingBoxIn);
                if (isLargeRoom)
                {
                    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 6, l, 1, 9, l, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
                    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 6, l, 12, 9, l, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
                }
            }
            else
            {
                fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, l, 1, 4, l, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
                fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 1, l, 12, 4, l, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
                if (isLargeRoom)
                {
                    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 6, l, 1, 9, l, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
                    fillWithBlocks(worldIn, structureBoundingBoxIn, 12, 6, l, 12, 9, l, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
                }
            }
        }
        for (int k1 = 3; k1 < 12; k1 += 2)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 1, k1, 4, 3, k1, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 6, 1, k1, 7, 3, k1, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 1, k1, 10, 3, k1, Blocks::bookshelf->getDefaultState(), Blocks::bookshelf->getDefaultState(), false);
        }
        if (isLargeRoom)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 5, 1, 3, 5, 13, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 5, 1, 12, 5, 13, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 5, 1, 9, 5, 2, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 5, 12, 9, 5, 13, Blocks::planks->getDefaultState(), Blocks::planks->getDefaultState(), false);
            setBlockState(worldIn, Blocks::planks->getDefaultState(), 9, 5, 11, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 5, 11, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::planks->getDefaultState(), 9, 5, 10, structureBoundingBoxIn);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 3, 6, 2, 3, 6, 12, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 6, 2, 10, 6, 10, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 6, 2, 9, 6, 2, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 6, 12, 8, 6, 12, Blocks::oak_fence->getDefaultState(), Blocks::oak_fence->getDefaultState(), false);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 9, 6, 11, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 8, 6, 11, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), 9, 6, 10, structureBoundingBoxIn);
            int l1 = getMetadataWithOffset(Blocks::ladder, 3);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 1, 13, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 2, 13, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 3, 13, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 4, 13, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 5, 13, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 6, 13, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::ladder->getStateFromMeta(l1), 10, 7, 13, structureBoundingBoxIn);
            int i1 = 7;
            int j1 = 7;
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 - 1, 9, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1, 9, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 - 1, 8, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1, 8, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 - 1, 7, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1, 7, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 - 2, 7, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 + 1, 7, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 - 1, 7, j1 - 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1 - 1, 7, j1 + 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1, 7, j1 - 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::oak_fence->getDefaultState(), i1, 7, j1 + 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::torch->getDefaultState(), i1 - 2, 8, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::torch->getDefaultState(), i1 + 1, 8, j1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::torch->getDefaultState(), i1 - 1, 8, j1 - 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::torch->getDefaultState(), i1 - 1, 8, j1 + 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::torch->getDefaultState(), i1, 8, j1 - 1, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::torch->getDefaultState(), i1, 8, j1 + 1, structureBoundingBoxIn);
        }
        // Java evaluates the book before the count.
        const auto chestList2 = WeightedRandomChestContent::func_177629_a(strongholdLibraryChestContents(), { Items::enchanted_book->getRandom(randomIn, 1, 5, 2) });
        const int chestMax2 = 1 + randomIn.nextInt(4);
        generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 3, 3, 5, chestList2, chestMax2);
        if (isLargeRoom)
        {
            setBlockState(worldIn, Blocks::air->getDefaultState(), 12, 9, 1, structureBoundingBoxIn);
            // Java evaluates the book before the count.
            const auto chestList3 = WeightedRandomChestContent::func_177629_a(strongholdLibraryChestContents(), { Items::enchanted_book->getRandom(randomIn, 1, 5, 2) });
            const int chestMax3 = 1 + randomIn.nextInt(4);
            generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 12, 8, 1, chestList3, chestMax3);
        }
        return true;
    }
}

PortalRoom::PortalRoom(int p_i45577_1_, Random& p_i45577_2_, const StructureBoundingBox& p_i45577_3_, EnumFacing p_i45577_4_)
    : Stronghold(p_i45577_1_)
{
    coordBaseMode = p_i45577_4_;
    boundingBox = p_i45577_3_;
}

void PortalRoom::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Mob", hasSpawner);
}

void PortalRoom::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    hasSpawner = tagCompound.getBoolean("Mob");
}

void PortalRoom::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    if (componentIn != nullptr)
    {
        static_cast<Stairs2*>(componentIn)->strongholdPortalRoom = this;
    }
}

std::shared_ptr<Stronghold> PortalRoom::func_175865_a(StructureComponentList& p_175865_0_, Random& p_175865_1_, int p_175865_2_, int p_175865_3_, int p_175865_4_, EnumFacing p_175865_5_, int p_175865_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175865_2_, p_175865_3_, p_175865_4_, -4, -1, 0, 11, 8, 16, p_175865_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175865_0_, structureboundingbox) == nullptr ? std::make_shared<PortalRoom>(p_175865_6_, p_175865_1_, structureboundingbox, p_175865_5_) : nullptr;
}

bool PortalRoom::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 10, 7, 15, false, randomIn, strongholdStones);
    placeDoor(worldIn, randomIn, structureBoundingBoxIn, Door::GRATES, 4, 1, 0);
    int i = 6;
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, i, 1, 1, i, 14, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 9, i, 1, 9, i, 14, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, i, 1, 8, i, 2, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 2, i, 14, 8, i, 14, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 2, 1, 4, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 8, 1, 1, 9, 1, 4, false, randomIn, strongholdStones);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 1, 1, 1, 3, Blocks::flowing_lava->getDefaultState(), Blocks::flowing_lava->getDefaultState(), false);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 9, 1, 1, 9, 1, 3, Blocks::flowing_lava->getDefaultState(), Blocks::flowing_lava->getDefaultState(), false);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 3, 1, 8, 7, 1, 12, false, randomIn, strongholdStones);
    fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 9, 6, 1, 11, Blocks::flowing_lava->getDefaultState(), Blocks::flowing_lava->getDefaultState(), false);
    for (int j = 3; j < 14; j += 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 3, j, 0, 4, j, Blocks::iron_bars->getDefaultState(), Blocks::iron_bars->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 3, j, 10, 4, j, Blocks::iron_bars->getDefaultState(), Blocks::iron_bars->getDefaultState(), false);
    }
    for (int k1 = 2; k1 < 9; k1 += 2)
    {
        fillWithBlocks(worldIn, structureBoundingBoxIn, k1, 3, 15, k1, 4, 15, Blocks::iron_bars->getDefaultState(), Blocks::iron_bars->getDefaultState(), false);
    }
    int l1 = getMetadataWithOffset(Blocks::stone_brick_stairs, 3);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 5, 6, 1, 7, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 2, 6, 6, 2, 7, false, randomIn, strongholdStones);
    fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 3, 7, 6, 3, 7, false, randomIn, strongholdStones);
    for (int k = 4; k <= 6; ++k)
    {
        setBlockState(worldIn, Blocks::stone_brick_stairs->getStateFromMeta(l1), k, 1, 4, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_brick_stairs->getStateFromMeta(l1), k, 2, 5, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_brick_stairs->getStateFromMeta(l1), k, 3, 6, structureBoundingBoxIn);
    }
    int i2 = getHorizontalIndex(EnumFacing::NORTH);
    int l = getHorizontalIndex(EnumFacing::SOUTH);
    int i1 = getHorizontalIndex(EnumFacing::EAST);
    int j1 = getHorizontalIndex(EnumFacing::WEST);
    if (coordBaseMode)
    {
        switch (*coordBaseMode)
        {
            case EnumFacing::SOUTH:
                i2 = getHorizontalIndex(EnumFacing::SOUTH);
                l = getHorizontalIndex(EnumFacing::NORTH);
                break;
            case EnumFacing::WEST:
                i2 = getHorizontalIndex(EnumFacing::WEST);
                l = getHorizontalIndex(EnumFacing::EAST);
                i1 = getHorizontalIndex(EnumFacing::SOUTH);
                j1 = getHorizontalIndex(EnumFacing::NORTH);
                break;
            case EnumFacing::EAST:
                i2 = getHorizontalIndex(EnumFacing::EAST);
                l = getHorizontalIndex(EnumFacing::WEST);
                i1 = getHorizontalIndex(EnumFacing::SOUTH);
                j1 = getHorizontalIndex(EnumFacing::NORTH);
        }
    }
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(i2)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 4, 3, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(i2)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 5, 3, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(i2)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 6, 3, 8, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(l)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 4, 3, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(l)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 5, 3, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(l)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 6, 3, 12, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(i1)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 3, 3, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(i1)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 3, 3, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(i1)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 3, 3, 11, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(j1)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 7, 3, 9, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(j1)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 7, 3, 10, structureBoundingBoxIn);
    setBlockState(worldIn, Blocks::end_portal_frame->getStateFromMeta(j1)->withProperty(BlockEndPortalFrame::EYE, randomIn.nextFloat() > 0.9f), 7, 3, 11, structureBoundingBoxIn);
    if (!hasSpawner)
    {
        i = getYWithOffset(3);
        const BlockPos blockpos(getXWithOffset(5, 6), i, getZWithOffset(5, 6));
        if (structureBoundingBoxIn.isVecInside(blockpos))
        {
            hasSpawner = true;
            worldIn->setBlockState(blockpos, Blocks::mob_spawner->getDefaultState(), 2);
            if (auto* spawner = dynamic_cast<TileEntityMobSpawner*>(worldIn->getTileEntity(blockpos)))
                spawner->getSpawnerBaseLogic().setEntityName("Silverfish");
        }
    }
    return true;
}

Prison::Prison(int p_i45576_1_, Random& p_i45576_2_, const StructureBoundingBox& p_i45576_3_, EnumFacing p_i45576_4_)
    : Stronghold(p_i45576_1_)
{
    coordBaseMode = p_i45576_4_;
    field_143013_d = getRandomDoor(p_i45576_2_);
    boundingBox = p_i45576_3_;
}

void Prison::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
}

std::shared_ptr<Stronghold> Prison::func_175860_a(StructureComponentList& p_175860_0_, Random& p_175860_1_, int p_175860_2_, int p_175860_3_, int p_175860_4_, EnumFacing p_175860_5_, int p_175860_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175860_2_, p_175860_3_, p_175860_4_, -1, -1, 0, 9, 5, 11, p_175860_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175860_0_, structureboundingbox) == nullptr ? std::make_shared<Prison>(p_175860_6_, p_175860_1_, structureboundingbox, p_175860_5_) : nullptr;
}

bool Prison::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 8, 4, 10, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 1, 0);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 1, 1, 10, 3, 3, 10, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 4, 3, 1, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 3, 4, 3, 3, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 7, 4, 3, 7, false, randomIn, strongholdStones);
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 4, 1, 9, 4, 3, 9, false, randomIn, strongholdStones);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 4, 4, 3, 6, Blocks::iron_bars->getDefaultState(), Blocks::iron_bars->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 5, 1, 5, 7, 3, 5, Blocks::iron_bars->getDefaultState(), Blocks::iron_bars->getDefaultState(), false);
        setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), 4, 3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), 4, 3, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::iron_door->getStateFromMeta(getMetadataWithOffset(Blocks::iron_door, 3)), 4, 1, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::iron_door->getStateFromMeta(getMetadataWithOffset(Blocks::iron_door, 3) + 8), 4, 2, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::iron_door->getStateFromMeta(getMetadataWithOffset(Blocks::iron_door, 3)), 4, 1, 8, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::iron_door->getStateFromMeta(getMetadataWithOffset(Blocks::iron_door, 3) + 8), 4, 2, 8, structureBoundingBoxIn);
        return true;
    }
}

void RightTurn::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    if (!isMode(EnumFacing::NORTH) && !isMode(EnumFacing::EAST))
    {
        getNextComponentX(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
    }
    else
    {
        getNextComponentZ(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
    }
}

bool RightTurn::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 4, 4, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 1, 0);
        if (!isMode(EnumFacing::NORTH) && !isMode(EnumFacing::EAST))
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 1, 0, 3, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        else
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 1, 4, 3, 3, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        return true;
    }
}

RoomCrossing::RoomCrossing(int p_i45575_1_, Random& p_i45575_2_, const StructureBoundingBox& p_i45575_3_, EnumFacing p_i45575_4_)
    : Stronghold(p_i45575_1_)
{
    coordBaseMode = p_i45575_4_;
    field_143013_d = getRandomDoor(p_i45575_2_);
    boundingBox = p_i45575_3_;
    roomType = p_i45575_2_.nextInt(5);
}

void RoomCrossing::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setInteger("Type", roomType);
}

void RoomCrossing::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    roomType = tagCompound.getInteger("Type");
}

void RoomCrossing::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 4, 1);
    getNextComponentX(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 4);
    getNextComponentZ(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 4);
}

std::shared_ptr<Stronghold> RoomCrossing::func_175859_a(StructureComponentList& p_175859_0_, Random& p_175859_1_, int p_175859_2_, int p_175859_3_, int p_175859_4_, EnumFacing p_175859_5_, int p_175859_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175859_2_, p_175859_3_, p_175859_4_, -4, -1, 0, 11, 7, 11, p_175859_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175859_0_, structureboundingbox) == nullptr ? std::make_shared<RoomCrossing>(p_175859_6_, p_175859_1_, structureboundingbox, p_175859_5_) : nullptr;
}

bool RoomCrossing::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 10, 6, 10, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 4, 1, 0);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 10, 6, 3, 10, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 4, 0, 3, 6, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        fillWithBlocks(worldIn, structureBoundingBoxIn, 10, 1, 4, 10, 3, 6, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        switch (roomType)
        {
            case 0:
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 5, 1, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 5, 2, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 5, 3, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 4, 3, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 6, 3, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 5, 3, 4, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 5, 3, 6, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 4, 1, 4, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 4, 1, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 4, 1, 6, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 6, 1, 4, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 6, 1, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 6, 1, 6, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 5, 1, 4, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stone_slab->getDefaultState(), 5, 1, 6, structureBoundingBoxIn);
                break;
            case 1:
                for (int i1 = 0; i1 < 5; ++i1)
                {
                    setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 1, 3 + i1, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 7, 1, 3 + i1, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3 + i1, 1, 3, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3 + i1, 1, 7, structureBoundingBoxIn);
                }
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 5, 1, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 5, 2, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 5, 3, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::flowing_water->getDefaultState(), 5, 4, 5, structureBoundingBoxIn);
                break;
            case 2:
                for (int i = 1; i <= 9; ++i)
                {
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 1, 3, i, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 9, 3, i, structureBoundingBoxIn);
                }
                for (int j = 1; j <= 9; ++j)
                {
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), j, 3, 1, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), j, 3, 9, structureBoundingBoxIn);
                }
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 5, 1, 4, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 5, 1, 6, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 5, 3, 4, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 5, 3, 6, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 1, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 6, 1, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, 3, 5, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 6, 3, 5, structureBoundingBoxIn);
                for (int k = 1; k <= 3; ++k)
                {
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, k, 4, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 6, k, 4, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 4, k, 6, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::cobblestone->getDefaultState(), 6, k, 6, structureBoundingBoxIn);
                }
                setBlockState(worldIn, Blocks::torch->getDefaultState(), 5, 3, 5, structureBoundingBoxIn);
                for (int l = 2; l <= 8; ++l)
                {
                    setBlockState(worldIn, Blocks::planks->getDefaultState(), 2, 3, l, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::planks->getDefaultState(), 3, 3, l, structureBoundingBoxIn);
                    if (l <= 3 || l >= 7)
                    {
                        setBlockState(worldIn, Blocks::planks->getDefaultState(), 4, 3, l, structureBoundingBoxIn);
                        setBlockState(worldIn, Blocks::planks->getDefaultState(), 5, 3, l, structureBoundingBoxIn);
                        setBlockState(worldIn, Blocks::planks->getDefaultState(), 6, 3, l, structureBoundingBoxIn);
                    }
                    setBlockState(worldIn, Blocks::planks->getDefaultState(), 7, 3, l, structureBoundingBoxIn);
                    setBlockState(worldIn, Blocks::planks->getDefaultState(), 8, 3, l, structureBoundingBoxIn);
                }
                setBlockState(worldIn, Blocks::ladder->getStateFromMeta(getMetadataWithOffset(Blocks::ladder, getIndex(EnumFacing::WEST))), 9, 1, 3, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::ladder->getStateFromMeta(getMetadataWithOffset(Blocks::ladder, getIndex(EnumFacing::WEST))), 9, 2, 3, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::ladder->getStateFromMeta(getMetadataWithOffset(Blocks::ladder, getIndex(EnumFacing::WEST))), 9, 3, 3, structureBoundingBoxIn);
                // Java evaluates the book before the count.
                const auto chestList4 = WeightedRandomChestContent::func_177629_a(strongholdRoomCrossingChestContents(), { Items::enchanted_book->getRandom(randomIn) });
                const int chestMax4 = 1 + randomIn.nextInt(4);
                generateChestContents(worldIn, structureBoundingBoxIn, randomIn, 3, 4, 8, chestList4, chestMax4);
        }
        return true;
    }
}

Stairs::Stairs(int p_i2081_1_, Random& p_i2081_2_, int p_i2081_3_, int p_i2081_4_)
    : Stronghold(p_i2081_1_)
{
    field_75024_a = true;
    coordBaseMode = randomFacing(Plane::HORIZONTAL, p_i2081_2_);
    field_143013_d = Door::OPENING;
    // Java switches on the facing but builds the same box either way.
    boundingBox = StructureBoundingBox(p_i2081_3_, 64, p_i2081_4_, p_i2081_3_ + 5 - 1, 74, p_i2081_4_ + 5 - 1);
}

Stairs::Stairs(int p_i45574_1_, Random& p_i45574_2_, const StructureBoundingBox& p_i45574_3_, EnumFacing p_i45574_4_)
    : Stronghold(p_i45574_1_)
{
    field_75024_a = false;
    coordBaseMode = p_i45574_4_;
    field_143013_d = getRandomDoor(p_i45574_2_);
    boundingBox = p_i45574_3_;
}

void Stairs::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Source", field_75024_a);
}

void Stairs::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    field_75024_a = tagCompound.getBoolean("Source");
}

void Stairs::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    if (field_75024_a)
    {
        strongComponentType = &Crossing::func_175866_a;
    }
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
}

std::shared_ptr<Stronghold> Stairs::func_175863_a(StructureComponentList& p_175863_0_, Random& p_175863_1_, int p_175863_2_, int p_175863_3_, int p_175863_4_, EnumFacing p_175863_5_, int p_175863_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175863_2_, p_175863_3_, p_175863_4_, -1, -7, 0, 5, 11, 5, p_175863_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175863_0_, structureboundingbox) == nullptr ? std::make_shared<Stairs>(p_175863_6_, p_175863_1_, structureboundingbox, p_175863_5_) : nullptr;
}

bool Stairs::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 10, 4, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 7, 0);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, Door::OPENING, 1, 1, 4);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 2, 6, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 5, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::STONE)), 1, 6, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 5, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 4, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::STONE)), 1, 5, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 2, 4, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 3, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::STONE)), 3, 4, 3, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 3, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 2, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::STONE)), 3, 3, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 2, 2, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 1, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::STONE)), 1, 2, 1, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 1, 2, structureBoundingBoxIn);
        setBlockState(worldIn, Blocks::stone_slab->getStateFromMeta(static_cast<int>(BlockStoneSlab::EnumType::STONE)), 1, 1, 3, structureBoundingBoxIn);
        return true;
    }
}

StairsStraight::StairsStraight(int p_i45572_1_, Random& p_i45572_2_, const StructureBoundingBox& p_i45572_3_, EnumFacing p_i45572_4_)
    : Stronghold(p_i45572_1_)
{
    coordBaseMode = p_i45572_4_;
    field_143013_d = getRandomDoor(p_i45572_2_);
    boundingBox = p_i45572_3_;
}

void StairsStraight::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
}

std::shared_ptr<Stronghold> StairsStraight::func_175861_a(StructureComponentList& p_175861_0_, Random& p_175861_1_, int p_175861_2_, int p_175861_3_, int p_175861_4_, EnumFacing p_175861_5_, int p_175861_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175861_2_, p_175861_3_, p_175861_4_, -1, -7, 0, 5, 11, 8, p_175861_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175861_0_, structureboundingbox) == nullptr ? std::make_shared<StairsStraight>(p_175861_6_, p_175861_1_, structureboundingbox, p_175861_5_) : nullptr;
}

bool StairsStraight::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 10, 7, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 7, 0);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, Door::OPENING, 1, 1, 7);
        int i = getMetadataWithOffset(Blocks::stone_stairs, 2);
        for (int j = 0; j < 6; ++j)
        {
            setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 1, 6 - j, 1 + j, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 2, 6 - j, 1 + j, structureBoundingBoxIn);
            setBlockState(worldIn, Blocks::stone_stairs->getStateFromMeta(i), 3, 6 - j, 1 + j, structureBoundingBoxIn);
            if (j < 5)
            {
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 1, 5 - j, 1 + j, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 2, 5 - j, 1 + j, structureBoundingBoxIn);
                setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), 3, 5 - j, 1 + j, structureBoundingBoxIn);
            }
        }
        return true;
    }
}

Straight::Straight(int p_i45573_1_, Random& p_i45573_2_, const StructureBoundingBox& p_i45573_3_, EnumFacing p_i45573_4_)
    : Stronghold(p_i45573_1_)
{
    coordBaseMode = p_i45573_4_;
    field_143013_d = getRandomDoor(p_i45573_2_);
    boundingBox = p_i45573_3_;
    expandsX = p_i45573_2_.nextInt(2) == 0;
    expandsZ = p_i45573_2_.nextInt(2) == 0;
}

void Straight::writeStructureToNBT(NBTTagCompound& tagCompound) const
{
    Stronghold::writeStructureToNBT(tagCompound);
    tagCompound.setBoolean("Left", expandsX);
    tagCompound.setBoolean("Right", expandsZ);
}

void Straight::readStructureFromNBT(const NBTTagCompound& tagCompound)
{
    Stronghold::readStructureFromNBT(tagCompound);
    expandsX = tagCompound.getBoolean("Left");
    expandsZ = tagCompound.getBoolean("Right");
}

void Straight::buildComponent(StructureComponent* componentIn, StructureComponentList& listIn, Random& rand)
{
    getNextComponentNormal(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 1);
    if (expandsX)
    {
        getNextComponentX(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 2);
    }
    if (expandsZ)
    {
        getNextComponentZ(static_cast<Stairs2*>(componentIn), listIn, rand, 1, 2);
    }
}

std::shared_ptr<Stronghold> Straight::func_175862_a(StructureComponentList& p_175862_0_, Random& p_175862_1_, int p_175862_2_, int p_175862_3_, int p_175862_4_, EnumFacing p_175862_5_, int p_175862_6_)
{
    auto structureboundingbox = StructureBoundingBox::getComponentToAddBoundingBox(p_175862_2_, p_175862_3_, p_175862_4_, -1, -1, 0, 5, 5, 7, p_175862_5_);
    return canStrongholdGoDeeper(structureboundingbox) && findIntersecting(p_175862_0_, structureboundingbox) == nullptr ? std::make_shared<Straight>(p_175862_6_, p_175862_1_, structureboundingbox, p_175862_5_) : nullptr;
}

bool Straight::addComponentParts(World* worldIn, Random& randomIn, const StructureBoundingBox& structureBoundingBoxIn)
{
    if (isLiquidInStructureBoundingBox(worldIn, structureBoundingBoxIn))
    {
        return false;
    }
    else
    {
        fillWithRandomizedBlocks(worldIn, structureBoundingBoxIn, 0, 0, 0, 4, 4, 6, true, randomIn, strongholdStones);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, field_143013_d, 1, 1, 0);
        placeDoor(worldIn, randomIn, structureBoundingBoxIn, Door::OPENING, 1, 1, 6);
        randomlyPlaceBlock(worldIn, structureBoundingBoxIn, randomIn, 0.1f, 1, 2, 1, Blocks::torch->getDefaultState());
        randomlyPlaceBlock(worldIn, structureBoundingBoxIn, randomIn, 0.1f, 3, 2, 1, Blocks::torch->getDefaultState());
        randomlyPlaceBlock(worldIn, structureBoundingBoxIn, randomIn, 0.1f, 1, 2, 5, Blocks::torch->getDefaultState());
        randomlyPlaceBlock(worldIn, structureBoundingBoxIn, randomIn, 0.1f, 3, 2, 5, Blocks::torch->getDefaultState());
        if (expandsX)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 0, 1, 2, 0, 3, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        if (expandsZ)
        {
            fillWithBlocks(worldIn, structureBoundingBoxIn, 4, 1, 2, 4, 3, 4, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
        }
        return true;
    }
}

void Stronghold::placeDoor(World* worldIn, Random& p_74990_2_, const StructureBoundingBox& p_74990_3_, Door p_74990_4_, int p_74990_5_, int p_74990_6_, int p_74990_7_)
{
    switch (p_74990_4_)
    {
        case Door::OPENING:
        default:
            fillWithBlocks(worldIn, p_74990_3_, p_74990_5_, p_74990_6_, p_74990_7_, p_74990_5_ + 3 - 1, p_74990_6_ + 3 - 1, p_74990_7_, Blocks::air->getDefaultState(), Blocks::air->getDefaultState(), false);
            break;
        case Door::WOOD_DOOR:
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 1, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 2, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 2, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 2, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::oak_door->getDefaultState(), p_74990_5_ + 1, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::oak_door->getStateFromMeta(8), p_74990_5_ + 1, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            break;
        case Door::GRATES:
            setBlockState(worldIn, Blocks::air->getDefaultState(), p_74990_5_ + 1, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::air->getDefaultState(), p_74990_5_ + 1, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_ + 1, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_ + 2, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_ + 2, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_bars->getDefaultState(), p_74990_5_ + 2, p_74990_6_, p_74990_7_, p_74990_3_);
            break;
        case Door::IRON_DOOR:
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 1, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 2, p_74990_6_ + 2, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 2, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stonebrick->getDefaultState(), p_74990_5_ + 2, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_door->getDefaultState(), p_74990_5_ + 1, p_74990_6_, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::iron_door->getStateFromMeta(8), p_74990_5_ + 1, p_74990_6_ + 1, p_74990_7_, p_74990_3_);
            setBlockState(worldIn, Blocks::stone_button->getStateFromMeta(getMetadataWithOffset(Blocks::stone_button, 4)), p_74990_5_ + 2, p_74990_6_ + 1, p_74990_7_ + 1, p_74990_3_);
            setBlockState(worldIn, Blocks::stone_button->getStateFromMeta(getMetadataWithOffset(Blocks::stone_button, 3)), p_74990_5_ + 2, p_74990_6_ + 1, p_74990_7_ - 1, p_74990_3_);
    }
}

std::shared_ptr<StructureComponent> Stronghold::getNextComponentNormal(Stairs2* p_74986_1_, StructureComponentList& p_74986_2_, Random& p_74986_3_, int p_74986_4_, int p_74986_5_)
{
    if (coordBaseMode)
    {
        switch (*coordBaseMode)
        {
            case EnumFacing::NORTH:
                return func_175953_c(p_74986_1_, p_74986_2_, p_74986_3_, boundingBox.minX + p_74986_4_, boundingBox.minY + p_74986_5_, boundingBox.minZ - 1, *coordBaseMode, getComponentType());
            case EnumFacing::SOUTH:
                return func_175953_c(p_74986_1_, p_74986_2_, p_74986_3_, boundingBox.minX + p_74986_4_, boundingBox.minY + p_74986_5_, boundingBox.maxZ + 1, *coordBaseMode, getComponentType());
            case EnumFacing::WEST:
                return func_175953_c(p_74986_1_, p_74986_2_, p_74986_3_, boundingBox.minX - 1, boundingBox.minY + p_74986_5_, boundingBox.minZ + p_74986_4_, *coordBaseMode, getComponentType());
            case EnumFacing::EAST:
                return func_175953_c(p_74986_1_, p_74986_2_, p_74986_3_, boundingBox.maxX + 1, boundingBox.minY + p_74986_5_, boundingBox.minZ + p_74986_4_, *coordBaseMode, getComponentType());
        }
    }
    return nullptr;
}

std::shared_ptr<StructureComponent> Stronghold::getNextComponentX(Stairs2* p_74989_1_, StructureComponentList& p_74989_2_, Random& p_74989_3_, int p_74989_4_, int p_74989_5_)
{
    if (coordBaseMode)
    {
        switch (*coordBaseMode)
        {
            case EnumFacing::NORTH:
                return func_175953_c(p_74989_1_, p_74989_2_, p_74989_3_, boundingBox.minX - 1, boundingBox.minY + p_74989_4_, boundingBox.minZ + p_74989_5_, EnumFacing::WEST, getComponentType());
            case EnumFacing::SOUTH:
                return func_175953_c(p_74989_1_, p_74989_2_, p_74989_3_, boundingBox.minX - 1, boundingBox.minY + p_74989_4_, boundingBox.minZ + p_74989_5_, EnumFacing::WEST, getComponentType());
            case EnumFacing::WEST:
                return func_175953_c(p_74989_1_, p_74989_2_, p_74989_3_, boundingBox.minX + p_74989_5_, boundingBox.minY + p_74989_4_, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType());
            case EnumFacing::EAST:
                return func_175953_c(p_74989_1_, p_74989_2_, p_74989_3_, boundingBox.minX + p_74989_5_, boundingBox.minY + p_74989_4_, boundingBox.minZ - 1, EnumFacing::NORTH, getComponentType());
        }
    }
    return nullptr;
}

std::shared_ptr<StructureComponent> Stronghold::getNextComponentZ(Stairs2* p_74987_1_, StructureComponentList& p_74987_2_, Random& p_74987_3_, int p_74987_4_, int p_74987_5_)
{
    if (coordBaseMode)
    {
        switch (*coordBaseMode)
        {
            case EnumFacing::NORTH:
                return func_175953_c(p_74987_1_, p_74987_2_, p_74987_3_, boundingBox.maxX + 1, boundingBox.minY + p_74987_4_, boundingBox.minZ + p_74987_5_, EnumFacing::EAST, getComponentType());
            case EnumFacing::SOUTH:
                return func_175953_c(p_74987_1_, p_74987_2_, p_74987_3_, boundingBox.maxX + 1, boundingBox.minY + p_74987_4_, boundingBox.minZ + p_74987_5_, EnumFacing::EAST, getComponentType());
            case EnumFacing::WEST:
                return func_175953_c(p_74987_1_, p_74987_2_, p_74987_3_, boundingBox.minX + p_74987_5_, boundingBox.minY + p_74987_4_, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType());
            case EnumFacing::EAST:
                return func_175953_c(p_74987_1_, p_74987_2_, p_74987_3_, boundingBox.minX + p_74987_5_, boundingBox.minY + p_74987_4_, boundingBox.maxZ + 1, EnumFacing::SOUTH, getComponentType());
        }
    }
    return nullptr;
}

} // namespace StructureStrongholdPieces

// --- MapGenStronghold ---

MapGenStronghold::MapGenStronghold()
    : structureCoords(3)
{
    for (BiomeGenBase* biomegenbase : BiomeGenBase::getBiomeGenArray()) {
        if (biomegenbase && biomegenbase->minHeight > 0.0f)
            field_151546_e.push_back(biomegenbase);
    }
}

MapGenStronghold::MapGenStronghold(const FlatGeneratorInfo::Options& options)
    : MapGenStronghold()
{
    for (const auto& [key, value] : options) {
        if (key == "distance") {
            field_82671_h = MathHelper::parseDoubleWithDefaultAndMax(value, field_82671_h, 1.0);
        } else if (key == "count") {
            const int count = MathHelper::parseIntWithDefaultAndMax(value, static_cast<int>(structureCoords.size()), 1);
            structureCoords.assign(static_cast<size_t>(count), std::nullopt);
        } else if (key == "spread") {
            field_82672_i = MathHelper::parseIntWithDefaultAndMax(value, field_82672_i, 1);
        }
    }
}

bool MapGenStronghold::canSpawnStructureAtCoords(int chunkX, int chunkZ)
{
    if (!ranBiomeCheck) {
        Random random(worldObj->getSeed());
        double d0 = random.nextDouble() * 3.141592653589793 * 2.0;
        int i = 1;
        for (int j = 0; j < static_cast<int>(structureCoords.size()); ++j) {
            const double d1 = (1.25 * static_cast<double>(i) + random.nextDouble()) * field_82671_h * static_cast<double>(i);
            int k = static_cast<int>(java::round(java::mathCos(d0) * d1));
            int l = static_cast<int>(java::round(java::mathSin(d0) * d1));
            const auto blockpos = worldObj->getWorldChunkManager()->findBiomePosition((k << 4) + 8, (l << 4) + 8, 112, field_151546_e, random);
            if (blockpos) {
                k = blockpos->getX() >> 4;
                l = blockpos->getZ() >> 4;
            }
            structureCoords[static_cast<size_t>(j)] = ChunkCoordIntPair(k, l);
            d0 += (3.141592653589793 * 2.0) * static_cast<double>(i) / static_cast<double>(field_82672_i);
            if (j == field_82672_i) {
                i += 2 + random.nextInt(5);
                field_82672_i += 1 + random.nextInt(2);
            }
        }
        ranBiomeCheck = true;
    }
    for (const auto& chunkcoordintpair : structureCoords) {
        // Java would throw on a null entry; the loop above fills them all.
        if (chunkcoordintpair && chunkX == chunkcoordintpair->chunkXPos && chunkZ == chunkcoordintpair->chunkZPos)
            return true;
    }
    return false;
}

std::optional<std::vector<BlockPos>> MapGenStronghold::getCoordList()
{
    std::vector<BlockPos> list;
    for (const auto& chunkcoordintpair : structureCoords) {
        if (chunkcoordintpair)
            list.push_back(chunkcoordintpair->getCenterBlock(64));
    }
    return list;
}

std::shared_ptr<StructureStart> MapGenStronghold::getStructureStart(int chunkX, int chunkZ)
{
    std::shared_ptr<Start> start = std::make_shared<Start>(worldObj, rand, chunkX, chunkZ);
    while (start->getComponents().empty()
        || static_cast<StructureStrongholdPieces::Stairs2*>(start->getComponents()[0].get())->strongholdPortalRoom == nullptr)
        start = std::make_shared<Start>(worldObj, rand, chunkX, chunkZ);
    return start;
}

MapGenStronghold::Start::Start(World* worldIn, Random& rand, int chunkX, int chunkZ)
    : StructureStart(chunkX, chunkZ)
{
    using namespace StructureStrongholdPieces;
    prepareStructurePieces();
    auto stairs2 = std::make_shared<Stairs2>(0, rand, (chunkX << 4) + 2, (chunkZ << 4) + 2);
    components.push_back(stairs2);
    stairs2->buildComponent(stairs2.get(), components, rand);
    StructureComponentList& list = stairs2->field_75026_c;
    while (!list.empty()) {
        const int i = rand.nextInt(static_cast<int>(list.size()));
        const std::shared_ptr<StructureComponent> structurecomponent = list[static_cast<size_t>(i)];
        list.erase(list.begin() + i);
        structurecomponent->buildComponent(stairs2.get(), components, rand);
    }
    updateBoundingBox();
    markAvailableHeight(worldIn, rand, 10);
}

} // namespace mc
