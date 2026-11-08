#include "world/chunk/storage/ExtendedBlockStorage.h"

#include "block/Block.h"
#include "init/Blocks.h"

namespace mc {

ExtendedBlockStorage::ExtendedBlockStorage(int y, bool storeSkylight)
    : yBase(y)
    , blocklightArray(std::make_unique<NibbleArray>())
{
    if (storeSkylight)
        skylightArray = std::make_unique<NibbleArray>();
}

const IBlockState* ExtendedBlockStorage::get(int x, int y, int z) const
{
    const IBlockState* iblockstate = Block::BLOCK_STATE_IDS.getByValue(data[static_cast<size_t>(y << 8 | z << 4 | x)]);
    return iblockstate ? iblockstate : Blocks::air->getDefaultState();
}

void ExtendedBlockStorage::set(int x, int y, int z, const IBlockState* state)
{
    const Block* block = get(x, y, z)->getBlock();
    const Block* block1 = state->getBlock();
    if (block != Blocks::air) {
        --blockRefCount;
        if (block->getTickRandomly())
            --tickRefCount;
    }
    if (block1 != Blocks::air) {
        ++blockRefCount;
        if (block1->getTickRandomly())
            ++tickRefCount;
    }
    // Java: (char) of the id; an unregistered state (-1) becomes 65535, which reads back as air.
    data[static_cast<size_t>(y << 8 | z << 4 | x)] = static_cast<uint16_t>(Block::BLOCK_STATE_IDS.get(state));
}

Block* ExtendedBlockStorage::getBlockByExtId(int x, int y, int z) const
{
    return get(x, y, z)->getBlock();
}

int ExtendedBlockStorage::getExtBlockMetadata(int x, int y, int z) const
{
    const IBlockState* iblockstate = get(x, y, z);
    return iblockstate->getBlock()->getMetaFromState(iblockstate);
}

void ExtendedBlockStorage::removeInvalidBlocks()
{
    blockRefCount = 0;
    tickRefCount = 0;
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            for (int k = 0; k < 16; ++k) {
                const Block* block = getBlockByExtId(i, j, k);
                if (block != Blocks::air) {
                    ++blockRefCount;
                    if (block->getTickRandomly())
                        ++tickRefCount;
                }
            }
        }
    }
}

} // namespace mc
