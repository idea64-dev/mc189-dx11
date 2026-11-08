#include "util/WeightedRandomChestContent.h"

#include "item/ItemStack.h"
#include "tileentity/TileEntities.h"

namespace mc {

WeightedRandomChestContent::WeightedRandomChestContent(Item* item, int meta, int minimumChance, int maximumChance, int itemWeightIn)
    : itemWeight(itemWeightIn)
    , theItemId(std::make_shared<ItemStack>(item, 1, meta))
    , minStackSize(minimumChance)
    , maxStackSize(maximumChance)
{
}

WeightedRandomChestContent::WeightedRandomChestContent(std::shared_ptr<ItemStack> stack, int minimumChance, int maximumChance, int itemWeightIn)
    : itemWeight(itemWeightIn)
    , theItemId(std::move(stack))
    , minStackSize(minimumChance)
    , maxStackSize(maximumChance)
{
}

namespace {

template <class Inventory>
void fill(Random& random, const std::vector<WeightedRandomChestContent>& listIn, Inventory& inv, int max)
{
    for (int i = 0; i < max; ++i) {
        const WeightedRandomChestContent* content = WeightedRandom::getRandomItem(random, listIn);
        const int j = content->minStackSize + random.nextInt(content->maxStackSize - content->minStackSize + 1);
        if (content->theItemId->getMaxStackSize() >= j) {
            auto itemstack1 = std::make_shared<ItemStack>(content->theItemId->copy());
            itemstack1->stackSize = j;
            inv.setInventorySlotContents(random.nextInt(inv.getSizeInventory()), itemstack1);
        } else {
            for (int k = 0; k < j; ++k) {
                auto itemstack = std::make_shared<ItemStack>(content->theItemId->copy());
                itemstack->stackSize = 1;
                inv.setInventorySlotContents(random.nextInt(inv.getSizeInventory()), itemstack);
            }
        }
    }
}

} // namespace

void WeightedRandomChestContent::generateChestContents(Random& random, const std::vector<WeightedRandomChestContent>& listIn, IInventory& inv, int max)
{
    fill(random, listIn, inv, max);
}

void WeightedRandomChestContent::generateDispenserContents(Random& random, const std::vector<WeightedRandomChestContent>& listIn,
    TileEntityDispenser& dispenser, int max)
{
    fill(random, listIn, dispenser, max);
}

std::vector<WeightedRandomChestContent> WeightedRandomChestContent::func_177629_a(std::vector<WeightedRandomChestContent> list,
    std::initializer_list<WeightedRandomChestContent> extra)
{
    list.insert(list.end(), extra.begin(), extra.end());
    return list;
}

} // namespace mc
