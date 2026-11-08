#pragma once

// Simple blocks that only set a material, tab, map color or drop count:
// net/minecraft/block/BlockGlowstone.java, BlockGravel.java, BlockHardenedClay.java, BlockMelon.java,
// BlockNetherBrick.java, BlockNetherrack.java, BlockObsidian.java, BlockPackedIce.java, BlockSoulSand.java,
// BlockSeaLantern.java, BlockSnowBlock.java, BlockWorkbench.java, BlockRedstoneLight.java and BlockNote.java.

#include "block/BlockContainer.h"
#include "block/BlockFalling.h"
#include "util/MathHelper.h"
#include "util/Random.h"

#include <algorithm>

namespace mc {

class BlockGlowstone : public Block {
public:
    MC_BLOCK_CLASS(BlockGlowstone)

    explicit BlockGlowstone(const Material& materialIn)
        : Block(materialIn)
    {
    }

    int quantityDroppedWithBonus(int fortune, Random& random) const override
    {
        // Java evaluates left to right; C++ leaves the order of + unspecified.
        const int base = quantityDropped(random);
        return MathHelper::clamp_int(base + random.nextInt(fortune + 1), 1, 4);
    }
    int quantityDropped(Random& random) const override { return 2 + random.nextInt(3); }
    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::sandColor; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::glowstone_dust; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockGravel : public BlockFalling {
public:
    MC_BLOCK_CLASS(BlockGravel)

    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::stoneColor; }
    Item* getItemDropped(const IBlockState*, Random& rand, int fortune) const override;
};

class BlockHardenedClay : public Block {
public:
    MC_BLOCK_CLASS(BlockHardenedClay)

    BlockHardenedClay()
        : Block(Material::rock)
    {
    }

    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::adobeColor; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockMelon : public Block {
public:
    MC_BLOCK_CLASS(BlockMelon)

    BlockMelon()
        : Block(Material::gourd, MapColor::limeColor)
    {
    }

    int quantityDropped(Random& random) const override { return 3 + random.nextInt(5); }
    int quantityDroppedWithBonus(int fortune, Random& random) const override
    {
        const int base = quantityDropped(random);
        return std::min(9, base + random.nextInt(1 + fortune));
    }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Items::melon; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockNetherBrick : public Block {
public:
    MC_BLOCK_CLASS(BlockNetherBrick)

    BlockNetherBrick()
        : Block(Material::rock)
    {
    }

    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::netherrackColor; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockNetherrack : public Block {
public:
    MC_BLOCK_CLASS(BlockNetherrack)

    BlockNetherrack()
        : Block(Material::rock)
    {
    }

    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::netherrackColor; }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockObsidian : public Block {
public:
    MC_BLOCK_CLASS(BlockObsidian)

    BlockObsidian()
        : Block(Material::rock)
    {
    }

    const MapColor& getMapColor(const IBlockState*) const override { return MapColor::blackColor; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return Item::getItemFromBlock(Blocks::obsidian); }

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockPackedIce : public Block {
public:
    MC_BLOCK_CLASS(BlockPackedIce)

    BlockPackedIce()
        : Block(Material::packedIce)
    {
    }

    int quantityDropped(Random&) const override { return 0; }

protected:
    void construct() override
    {
        Block::construct();
        slipperiness = 0.98f;
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

class BlockNote : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockNote)

    BlockNote()
        : BlockContainer(Material::wood)
    {
    }

    int getRenderType() const override { return 3; }

protected:
    void construct() override
    {
        BlockContainer::construct();
        setCreativeTab(&CreativeTabs::tabRedstone);
    }
};

class BlockMobSpawner : public BlockContainer {
public:
    MC_BLOCK_CLASS(BlockMobSpawner)

    BlockMobSpawner()
        : BlockContainer(Material::rock)
    {
    }

    int quantityDropped(Random&) const override { return 0; }
    bool isOpaqueCube() const override { return false; }
    int getRenderType() const override { return 3; }
    EnumWorldBlockLayer getBlockLayer() const override { return EnumWorldBlockLayer::CUTOUT; }
    Item* getItemDropped(const IBlockState*, Random&, int) const override { return nullptr; }
};

} // namespace mc
