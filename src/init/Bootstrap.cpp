#include "init/Bootstrap.h"

#include "block/Block.h"
#include "init/Blocks.h"

namespace mc {

void Bootstrap::registerAll()
{
    if (alreadyRegistered)
        return;
    alreadyRegistered = true;
    Block::registerBlocks();
    Blocks::init();
    // Java: BlockFire.init(), Item.registerItems(), StatList.init(), dispenser behaviours.
}

} // namespace mc
