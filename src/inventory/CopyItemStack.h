#pragma once

// Java: ItemStack.copyItemStack -- a deep copy, null stays null.

#include "item/ItemStack.h"

#include <memory>

namespace mc {

inline std::shared_ptr<ItemStack> copyItemStack(const ItemStack* stack)
{
    return stack ? std::make_shared<ItemStack>(stack->copy()) : nullptr;
}

inline std::shared_ptr<ItemStack> copyItemStack(const std::shared_ptr<ItemStack>& stack)
{
    return copyItemStack(stack.get());
}

} // namespace mc
