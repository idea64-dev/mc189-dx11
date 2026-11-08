#include "init/Blocks.h"

#include "block/Block.h"

#include <stdexcept>

namespace mc::Blocks {

namespace {

Block* getRegisteredBlock(const char* name)
{
    Block* block = Block::blockRegistry.getObject(ResourceLocation(name));
    if (block == nullptr)
        throw std::logic_error(std::string("Invalid Block requested: ") + name);
    return block;
}

} // namespace

void init()
{
    air = getRegisteredBlock("air");
    stone = getRegisteredBlock("stone");
}

} // namespace mc::Blocks
