#pragma once

// Original: net/minecraft/block/BlockFire.java (spreading and the flammability table come with world ticks, stage 9)

#include "block/BlockBasics.h"

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
