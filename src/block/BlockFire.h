#pragma once

// Original: net/minecraft/block/BlockFire.java

#include "block/BlockBasics.h"

#include <unordered_map>

namespace mc {

class BlockFire : public Block {
public:
    MC_BLOCK_CLASS(BlockFire)

    static const PropertyInteger AGE;
    static const PropertyBool FLIP;
    static const PropertyBool ALT;
    static const PropertyBool NORTH;
    static const PropertyBool EAST;
    static const PropertyBool SOUTH;
    static const PropertyBool WEST;
    static const PropertyInteger UPPER;

    BlockFire()
        : Block(Material::fire)
    {
    }

    bool isOpaqueCube() const override { return false; }
    bool isFullCube() const override { return false; }
    int quantityDropped(Random&) const override { return 0; }
    bool requiresUpdates() const override { return false; }
    bool isCollidable() const override { return false; }
    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::tntColor; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    const IBlockState* getStateFromMeta(int meta) const override { return getDefaultState()->withProperty(AGE, meta); }
    int getMetaFromState(const IBlockState* state) const override { return state->getValue(AGE); }
    const IBlockState* getActualState(const IBlockState* state, IBlockAccess* worldIn, const BlockPos& pos) override;
    // Java: the flammability table, filled from Bootstrap after the blocks are registered.
    static void init();
    void setFireInfo(const Block* blockIn, int encouragement, int flammability);
    std::optional<AxisAlignedBB> getCollisionBoundingBox(World*, const BlockPos&, const IBlockState*) override { return std::nullopt; }
    int tickRate(World*) override { return 30; }
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    bool canCatchFire(IBlockAccess* worldIn, const BlockPos& pos);
    bool canPlaceBlockAt(World* worldIn, const BlockPos& pos) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void randomDisplayTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;

protected:
    bool canDie(World* worldIn, const BlockPos& pos);

private:
    int getFlammability(const Block* blockIn) const;
    int getEncouragement(const Block* blockIn) const;
    void catchOnFire(World* worldIn, const BlockPos& pos, int chance, Random& random, int age);
    bool canNeighborCatchFire(World* worldIn, const BlockPos& pos);
    int getNeighborEncouragement(World* worldIn, const BlockPos& pos);

    std::unordered_map<const Block*, int> encouragements;
    std::unordered_map<const Block*, int> flammabilities;

public:

protected:
    void construct() override
    {
        Block::construct();
        setDefaultState(blockState->getBaseState()->withProperty(AGE, 0)->withProperty(FLIP, false)->withProperty(ALT, false)->withProperty(NORTH, false)->withProperty(EAST, false)->withProperty(SOUTH, false)->withProperty(WEST, false)->withProperty(UPPER, 0));
        setTickRandomly(true);
    }
    std::unique_ptr<BlockState> createBlockState() override { return makeBlockState({ &AGE, &NORTH, &EAST, &SOUTH, &WEST, &UPPER, &FLIP, &ALT }); }
};

inline const PropertyInteger BlockFire::AGE("age", 0, 15);
inline const PropertyBool BlockFire::FLIP("flip");
inline const PropertyBool BlockFire::ALT("alt");
inline const PropertyBool BlockFire::NORTH("north");
inline const PropertyBool BlockFire::EAST("east");
inline const PropertyBool BlockFire::SOUTH("south");
inline const PropertyBool BlockFire::WEST("west");
inline const PropertyInteger BlockFire::UPPER("upper", 0, 2);

} // namespace mc
