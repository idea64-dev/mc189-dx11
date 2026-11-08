#pragma once

// Original: net/minecraft/world/gen/feature/*.java and world/gen/GeneratorBushFeature.java -- the decoration
// features: trees, plants, ores, lakes, dungeons and the rest.
// Java's argument and operand order is left to right and many features draw several random numbers in one
// expression (position.add(rand.nextInt(8) - rand.nextInt(8), ...)); C++ leaves that order unspecified, so
// the implementations draw into locals first.

#include "block/BlockDoublePlant.h"
#include "block/BlockFlower.h"
#include "block/BlockMisc.h"
#include "util/BlockPos.h"
#include "util/WeightedRandomChestContent.h"

#include <functional>
#include <memory>
#include <vector>

namespace mc {

class Block;
class BlockBush;
class IBlockState;
class PropertyBool;
class Random;
class World;

class WorldGenerator {
public:
    WorldGenerator()
        : WorldGenerator(false)
    {
    }
    explicit WorldGenerator(bool notify)
        : doBlockNotify(notify)
    {
    }
    virtual ~WorldGenerator() = default;

    virtual bool generate(World* worldIn, Random& rand, const BlockPos& position) = 0;
    virtual void func_175904_e() { }

protected:
    void setBlockAndNotifyAdequately(World* worldIn, const BlockPos& pos, const IBlockState* state);

private:
    bool doBlockNotify;
};

class WorldGenAbstractTree : public WorldGenerator {
public:
    using WorldGenerator::WorldGenerator;

    virtual void func_180711_a(World*, Random&, const BlockPos&) { }

protected:
    static bool func_150523_a(const Block* block);
    void func_175921_a(World* worldIn, const BlockPos& pos);
};

class WorldGenTrees : public WorldGenAbstractTree {
public:
    explicit WorldGenTrees(bool notify);
    WorldGenTrees(bool notify, int minTreeHeightIn, const IBlockState* woodIn, const IBlockState* leavesIn, bool vinesGrowIn);

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    void func_181652_a(World* worldIn, int age, const BlockPos& pos, EnumFacing facing);
    void func_181651_a(World* worldIn, const BlockPos& pos, const PropertyBool& side);
    void func_181650_b(World* worldIn, BlockPos pos, const PropertyBool& side);

    int minTreeHeight;
    bool vinesGrow;
    const IBlockState* metaWood;
    const IBlockState* metaLeaves;
};

class WorldGenShrub : public WorldGenTrees {
public:
    WorldGenShrub(const IBlockState* wood, const IBlockState* leaves);

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    const IBlockState* leavesMetadata;
    const IBlockState* woodMetadata;
};

class WorldGenBigTree : public WorldGenAbstractTree {
public:
    using WorldGenAbstractTree::WorldGenAbstractTree;

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
    void func_175904_e() override { leafDistanceLimit = 5; }

private:
    struct FoliageCoordinates {
        BlockPos pos;
        int field_178000_b;
    };

    void generateLeafNodeList();
    void func_181631_a(const BlockPos& pos, float size, const IBlockState* state);
    float layerSize(int y) const;
    float leafSize(int y) const;
    void generateLeafNode(const BlockPos& pos);
    void func_175937_a(const BlockPos& from, const BlockPos& to, const Block* block);
    static int getGreatestDistance(const BlockPos& posIn);
    static int func_175938_b(const BlockPos& base, const BlockPos& pos);
    void generateLeaves();
    bool leafNodeNeedsBase(int y) const { return static_cast<double>(y) >= static_cast<double>(heightLimit) * 0.2; }
    void generateTrunk();
    void generateLeafNodeBases();
    int checkBlockLine(const BlockPos& posOne, const BlockPos& posTwo);
    bool validTreeLocation();

    std::unique_ptr<Random> rand;
    World* world = nullptr;
    BlockPos basePos = BlockPos::ORIGIN;
    int heightLimit = 0;
    int height = 0;
    double heightAttenuation = 0.618;
    double branchSlope = 0.381;
    double scaleWidth = 1.0;
    double leafDensity = 1.0;
    int trunkSize = 1;
    int heightLimitLimit = 12;
    int leafDistanceLimit = 4;
    std::vector<FoliageCoordinates> field_175948_j;
};

class WorldGenSwamp : public WorldGenAbstractTree {
public:
    WorldGenSwamp()
        : WorldGenAbstractTree(false)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    void func_181647_a(World* worldIn, BlockPos pos, const PropertyBool& side);
};

class WorldGenForest : public WorldGenAbstractTree {
public:
    WorldGenForest(bool notify, bool extraHeight)
        : WorldGenAbstractTree(notify)
        , useExtraRandomHeight(extraHeight)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    bool useExtraRandomHeight;
};

class WorldGenTaiga1 : public WorldGenAbstractTree {
public:
    WorldGenTaiga1()
        : WorldGenAbstractTree(false)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenTaiga2 : public WorldGenAbstractTree {
public:
    using WorldGenAbstractTree::WorldGenAbstractTree;

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenHugeTrees : public WorldGenAbstractTree {
public:
    WorldGenHugeTrees(bool notify, int baseHeightIn, int extraRandomHeightIn, const IBlockState* wood, const IBlockState* leaves)
        : WorldGenAbstractTree(notify)
        , baseHeight(baseHeightIn)
        , woodMetadata(wood)
        , leavesMetadata(leaves)
        , extraRandomHeight(extraRandomHeightIn)
    {
    }

protected:
    int func_150533_a(Random& rand);
    bool func_175929_a(World* worldIn, Random& rand, const BlockPos& pos, int height);
    void func_175925_a(World* worldIn, const BlockPos& pos, int radius);
    void func_175928_b(World* worldIn, const BlockPos& pos, int radius);

    const int baseHeight;
    const IBlockState* const woodMetadata;
    const IBlockState* const leavesMetadata;
    int extraRandomHeight;

private:
    bool func_175926_c(World* worldIn, const BlockPos& pos, int height);
    bool func_175927_a(const BlockPos& pos, World* worldIn);
};

class WorldGenMegaPineTree : public WorldGenHugeTrees {
public:
    WorldGenMegaPineTree(bool notify, bool useBaseHeightIn);

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
    void func_180711_a(World* worldIn, Random& rand, const BlockPos& pos) override;

private:
    void func_150541_c(World* worldIn, int x, int z, int y, int radius, Random& rand);
    void func_175933_b(World* worldIn, const BlockPos& pos);
    void func_175934_c(World* worldIn, const BlockPos& pos);

    bool useBaseHeight;
};

class WorldGenMegaJungle : public WorldGenHugeTrees {
public:
    using WorldGenHugeTrees::WorldGenHugeTrees;

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    void func_181632_a(World* worldIn, Random& rand, const BlockPos& pos, const PropertyBool& side);
    void func_175930_c(World* worldIn, const BlockPos& pos, int radius);
};

class WorldGenCanopyTree : public WorldGenAbstractTree {
public:
    using WorldGenAbstractTree::WorldGenAbstractTree;

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    bool func_181638_a(World* worldIn, const BlockPos& pos, int height);
    void func_181639_b(World* worldIn, const BlockPos& pos);
    void func_150526_a(World* worldIn, int x, int y, int z);
};

class WorldGenSavannaTree : public WorldGenAbstractTree {
public:
    using WorldGenAbstractTree::WorldGenAbstractTree;

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    void func_181642_b(World* worldIn, const BlockPos& pos);
    void func_175924_b(World* worldIn, const BlockPos& pos);
};

class WorldGenBigMushroom : public WorldGenerator {
public:
    explicit WorldGenBigMushroom(Block* type)
        : WorldGenerator(true)
        , mushroomType(type)
    {
    }
    WorldGenBigMushroom()
        : WorldGenerator(false)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* mushroomType = nullptr;
};

class WorldGenTallGrass : public WorldGenerator {
public:
    explicit WorldGenTallGrass(BlockTallGrass::EnumType type);

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    const IBlockState* tallGrassState;
};

class WorldGenDoublePlant : public WorldGenerator {
public:
    void setPlantType(BlockDoublePlant::EnumPlantType type) { field_150549_a = type; }
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    BlockDoublePlant::EnumPlantType field_150549_a = BlockDoublePlant::EnumPlantType::SUNFLOWER;
};

class WorldGenFlowers : public WorldGenerator {
public:
    WorldGenFlowers(BlockFlower* flowerIn, BlockFlower::EnumFlowerType type) { setGeneratedBlock(flowerIn, type); }

    void setGeneratedBlock(BlockFlower* flowerIn, BlockFlower::EnumFlowerType type);
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    BlockFlower* flower = nullptr;
    const IBlockState* field_175915_b = nullptr;
};

// Java: world/gen/GeneratorBushFeature.java
class GeneratorBushFeature : public WorldGenerator {
public:
    explicit GeneratorBushFeature(BlockBush* bush)
        : field_175908_a(bush)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    BlockBush* field_175908_a;
};

class WorldGenDeadBush : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenCactus : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenReed : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenWaterlily : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenPumpkin : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenMelon : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenVines : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenFire : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenGlowStone1 : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenGlowStone2 : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

// WorldGenClay, WorldGenSand and WorldGenIcePath: discs of a block replacing others around a position.
class WorldGenClay : public WorldGenerator {
public:
    explicit WorldGenClay(int numberOfBlocksIn)
        : numberOfBlocks(numberOfBlocksIn)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    int numberOfBlocks;
};

class WorldGenSand : public WorldGenerator {
public:
    WorldGenSand(Block* blockIn, int radiusIn)
        : block(blockIn)
        , radius(radiusIn)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* block;
    int radius;
};

class WorldGenIcePath : public WorldGenerator {
public:
    explicit WorldGenIcePath(int width)
        : basePathWidth(width)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    int basePathWidth;
};

class WorldGenIceSpike : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenBlockBlob : public WorldGenerator {
public:
    WorldGenBlockBlob(Block* blockIn, int size)
        : WorldGenerator(false)
        , field_150545_a(blockIn)
        , field_150544_b(size)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* field_150545_a;
    int field_150544_b;
};

class WorldGenDesertWells : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;
};

class WorldGenLiquids : public WorldGenerator {
public:
    explicit WorldGenLiquids(Block* blockIn)
        : block(blockIn)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* block;
};

class WorldGenHellLava : public WorldGenerator {
public:
    WorldGenHellLava(Block* blockIn, bool insideRock)
        : field_150553_a(blockIn)
        , field_94524_b(insideRock)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* field_150553_a;
    bool field_94524_b;
};

class WorldGenMinable : public WorldGenerator {
public:
    using Predicate = std::function<bool(const IBlockState*)>;

    // Replacing stone, as Java's BlockHelper.forBlock(Blocks.stone).
    WorldGenMinable(const IBlockState* state, int blockCount);
    WorldGenMinable(const IBlockState* state, int blockCount, Predicate predicateIn)
        : oreBlock(state)
        , numberOfBlocks(blockCount)
        , predicate(std::move(predicateIn))
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    const IBlockState* oreBlock;
    int numberOfBlocks;
    Predicate predicate;
};

class WorldGenLakes : public WorldGenerator {
public:
    explicit WorldGenLakes(Block* blockIn)
        : block(blockIn)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* block;
};

class WorldGenDungeons : public WorldGenerator {
public:
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    static std::string pickMobSpawner(Random& rand);
};

// Original: net/minecraft/world/gen/feature/WorldGeneratorBonusChest.java
class WorldGeneratorBonusChest : public WorldGenerator {
public:
    WorldGeneratorBonusChest(std::vector<WeightedRandomChestContent> chestItemsIn, int itemsToGenerate)
        : chestItems(std::move(chestItemsIn))
        , itemsToGenerateInBonusChest(itemsToGenerate)
    {
    }
    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    std::vector<WeightedRandomChestContent> chestItems;
    int itemsToGenerateInBonusChest;
};

class WorldGenSpikes : public WorldGenerator {
public:
    explicit WorldGenSpikes(Block* baseBlock)
        : baseBlockRequired(baseBlock)
    {
    }

    bool generate(World* worldIn, Random& rand, const BlockPos& position) override;

private:
    Block* baseBlockRequired;
};

} // namespace mc
