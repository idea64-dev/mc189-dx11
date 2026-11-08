#pragma once

// Original: net/minecraft/block/BlockFalling.java (falling behaviour comes with entities, stage 9)

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

protected:
    void construct() override
    {
        Block::construct();
        if (setsTab)
            setCreativeTab(&CreativeTabs::tabBlock);
    }

private:
    // Only the Java constructor without arguments sets the tab.
    bool setsTab = false;
};

} // namespace mc
