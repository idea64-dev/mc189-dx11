#include "init/Bootstrap.h"

#include "block/Block.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/Item.h"

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
    // Java: StatList.init() and the dispenser behaviours -- with statistics and the dispenser (stages 9 and 10).
}

} // namespace mc
