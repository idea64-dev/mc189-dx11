// Original: Block.registerBlocks() from net/minecraft/block/Block.java -- every block with its id, name and
// the setter chain of the original.

#include "block/Block.h"
#include "block/BlockAir.h"
#include "block/BlockStone.h"

namespace mc {

void Block::registerBlocks()
{
    registerBlock(0, ResourceLocation("air"), create<BlockAir>()->setUnlocalizedName("air"));
    registerBlock(1, "stone", create<BlockStone>()->setHardness(1.5f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stone"));

    // Java: blockRegistry.validateKey()
    for (Block* block13 : blockRegistry.values()) {
        if (&block13->blockMaterial == &Material::air) {
            block13->useNeighborBrightness = false;
        } else {
            bool flag = false;
            const bool flag1 = false; // block13 instanceof BlockStairs
            const bool flag2 = false; // block13 instanceof BlockSlab
            const bool flag3 = false; // block13 == farmland
            const bool flag4 = block13->translucent;
            const bool flag5 = block13->lightOpacity == 0;
            if (flag1 || flag2 || flag3 || flag4 || flag5)
                flag = true;
            block13->useNeighborBrightness = flag;
        }
    }

    for (Block* block14 : blockRegistry.values()) {
        for (const IBlockState* state : block14->getBlockState().getValidStates()) {
            const int i = blockRegistry.getIDForObject(block14) << 4 | block14->getMetaFromState(state);
            BLOCK_STATE_IDS.put(state, i);
        }
    }
}

} // namespace mc
