#include "init/Bootstrap.h"

#include "block/Block.h"
#include "enchantment/Enchantment.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/Item.h"
#include "tileentity/TileEntity.h"
#include "world/biome/BiomeGenBase.h"

namespace mc {

void Bootstrap::registerAll()
{
    if (alreadyRegistered)
        return;
    alreadyRegistered = true;
    Block::registerBlocks();
    Blocks::init();
    // Java: BlockFire.init() -- with fire spreading (stage 9).
    Item::registerItems();
    Items::init();
    // Java: the static initializers of these classes, run when first used.
    BiomeGenBase::registerBiomes();
    TileEntity::registerTileEntities();
    Enchantment::registerEnchantments();
    // Java: StatList.init() and the dispenser behaviours -- with statistics and the dispenser (stages 12 and 9).
}

} // namespace mc
