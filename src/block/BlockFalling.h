#pragma once

// Original: net/minecraft/block/BlockFalling.java (the falling entity comes with stage 10)

#include "block/BlockBasics.h"

namespace mc {

class BlockFalling : public Block {
public:
    static inline bool fallInstantly = false;

    BlockFalling()
        : Block(Material::sand)
        , setsTab(true)
    {
    }
    explicit BlockFalling(const Material& materialIn)
        : Block(materialIn)
    {
    }

    void onBlockAdded(World* worldIn, const BlockPos& pos, const IBlockState* state) override;
    void onNeighborBlockChange(World* worldIn, const BlockPos& pos, const IBlockState* state, Block* neighborBlock) override;
    void updateTick(World* worldIn, const BlockPos& pos, const IBlockState* state, Random& rand) override;
    int tickRate(World*) override { return 2; }
    static bool canFallInto(World* worldIn, const BlockPos& pos);
    virtual void onEndFalling(World*, const BlockPos&) { }

protected:
    void construct() override
    {
        Block::construct();
        if (setsTab)
            setCreativeTab(&CreativeTabs::tabBlock);
    }

private:
    void checkFallable(World* worldIn, const BlockPos& pos);

    // Only the Java constructor without arguments sets the tab.
    bool setsTab = false;
};

} // namespace mc
