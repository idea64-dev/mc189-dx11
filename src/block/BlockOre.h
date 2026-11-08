#pragma once

// Original: net/minecraft/block/BlockOre.java and BlockRedstoneOre.java

#include "block/BlockBasics.h"
#include "init/Blocks.h"
#include "item/EnumDyeColor.h"
#include "util/Random.h"

namespace mc {

class BlockOre : public Block {
public:
    MC_BLOCK_CLASS(BlockOre)

    BlockOre()
        : BlockOre(Material::rock.getMaterialMapColor())
    {
    }
    explicit BlockOre(const MapColor& mapColorIn)
        : Block(Material::rock, mapColorIn)
    {
    }

    int quantityDropped(Random& random) const override { return this == Blocks::lapis_ore ? 4 + random.nextInt(5) : 1; }
    int damageDropped(const IBlockState*) const override { return this == Blocks::lapis_ore ? getDyeDamage(EnumDyeColor::BLUE) : 0; }
    // Java: getItemDropped (coal, diamond, lapis, emerald, quartz) and the fortune bonus need items (stage 4 items).

protected:
    void construct() override
    {
        Block::construct();
        setCreativeTab(&CreativeTabs::tabBlock);
    }
};

} // namespace mc
