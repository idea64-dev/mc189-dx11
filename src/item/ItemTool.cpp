#include "item/ItemTool.h"

#include "block/AllBlocks.h"
#include "creativetab/CreativeTabs.h"
#include "entity/EntityItemHooks.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "item/ItemStack.h"
#include "world/World.h"

namespace mc {

namespace {

// The EFFECTIVE_ON sets, built on first use like the Java class initializers (the first tool is made in
// registerItems, after the blocks).
const std::unordered_set<const Block*>& spadeEffectiveOn()
{
    static const std::unordered_set<const Block*> set { Blocks::clay, Blocks::dirt, Blocks::farmland, Blocks::grass, Blocks::gravel, Blocks::mycelium,
        Blocks::sand, Blocks::snow, Blocks::snow_layer, Blocks::soul_sand };
    return set;
}

const std::unordered_set<const Block*>& pickaxeEffectiveOn()
{
    static const std::unordered_set<const Block*> set { Blocks::activator_rail, Blocks::coal_ore, Blocks::cobblestone, Blocks::detector_rail,
        Blocks::diamond_block, Blocks::diamond_ore, Blocks::double_stone_slab, Blocks::golden_rail, Blocks::gold_block, Blocks::gold_ore, Blocks::ice,
        Blocks::iron_block, Blocks::iron_ore, Blocks::lapis_block, Blocks::lapis_ore, Blocks::lit_redstone_ore, Blocks::mossy_cobblestone,
        Blocks::netherrack, Blocks::packed_ice, Blocks::rail, Blocks::redstone_ore, Blocks::sandstone, Blocks::red_sandstone, Blocks::stone,
        Blocks::stone_slab };
    return set;
}

const std::unordered_set<const Block*>& axeEffectiveOn()
{
    static const std::unordered_set<const Block*> set { Blocks::planks, Blocks::bookshelf, Blocks::log, Blocks::log2, Blocks::chest, Blocks::pumpkin,
        Blocks::lit_pumpkin, Blocks::melon_block, Blocks::ladder };
    return set;
}

} // namespace

// --- ItemTool ---

ItemTool::ItemTool(float attackDamage, const ToolMaterial& material, const std::unordered_set<const Block*>& effectiveBlocksIn)
    : toolMaterial(material)
    , effectiveBlocks(effectiveBlocksIn)
    , damageVsEntity(attackDamage + material.getDamageVsEntity())
{
    maxStackSize = 1;
    setMaxDamage(material.getMaxUses());
    efficiencyOnProperMaterial = material.getEfficiencyOnProperMaterial();
    setCreativeTab(&CreativeTabs::tabTools);
}

float ItemTool::getStrVsBlock(const ItemStack&, const Block* block) const
{
    return effectiveBlocks.contains(block) ? efficiencyOnProperMaterial : 1.0f;
}

bool ItemTool::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const
{
    return toolMaterial.getRepairItem() == repair.getItem() ? true : Item::getIsRepairable(toRepair, repair);
}

// --- ItemSpade ---

ItemSpade::ItemSpade(const ToolMaterial& material)
    : ItemTool(1.0f, material, spadeEffectiveOn())
{
}

bool ItemSpade::canHarvestBlock(const Block* block) const
{
    return block == Blocks::snow_layer ? true : block == Blocks::snow;
}

// --- ItemPickaxe ---

ItemPickaxe::ItemPickaxe(const ToolMaterial& material)
    : ItemTool(2.0f, material, pickaxeEffectiveOn())
{
}

bool ItemPickaxe::canHarvestBlock(const Block* block) const
{
    if (block == Blocks::obsidian)
        return toolMaterial.getHarvestLevel() == 3;
    if (block == Blocks::diamond_block || block == Blocks::diamond_ore)
        return toolMaterial.getHarvestLevel() >= 2;
    if (block == Blocks::emerald_ore || block == Blocks::emerald_block)
        return toolMaterial.getHarvestLevel() >= 2;
    if (block == Blocks::gold_block || block == Blocks::gold_ore)
        return toolMaterial.getHarvestLevel() >= 2;
    if (block == Blocks::iron_block || block == Blocks::iron_ore)
        return toolMaterial.getHarvestLevel() >= 1;
    if (block == Blocks::lapis_block || block == Blocks::lapis_ore)
        return toolMaterial.getHarvestLevel() >= 1;
    if (block == Blocks::redstone_ore || block == Blocks::lit_redstone_ore)
        return toolMaterial.getHarvestLevel() >= 2;
    const Material& material = block->getMaterial();
    return &material == &Material::rock || &material == &Material::iron || &material == &Material::anvil;
}

float ItemPickaxe::getStrVsBlock(const ItemStack& stack, const Block* block) const
{
    const Material& material = block->getMaterial();
    return &material != &Material::iron && &material != &Material::anvil && &material != &Material::rock ? ItemTool::getStrVsBlock(stack, block)
                                                                                                         : efficiencyOnProperMaterial;
}

// --- ItemAxe ---

ItemAxe::ItemAxe(const ToolMaterial& material)
    : ItemTool(3.0f, material, axeEffectiveOn())
{
}

float ItemAxe::getStrVsBlock(const ItemStack& stack, const Block* block) const
{
    const Material& material = block->getMaterial();
    return &material != &Material::wood && &material != &Material::plants && &material != &Material::vine ? ItemTool::getStrVsBlock(stack, block)
                                                                                                         : efficiencyOnProperMaterial;
}

// --- ItemSword ---

ItemSword::ItemSword(const ToolMaterial& materialIn)
    : attackDamage(4.0f + materialIn.getDamageVsEntity())
    , material(materialIn)
{
    maxStackSize = 1;
    setMaxDamage(materialIn.getMaxUses());
    setCreativeTab(&CreativeTabs::tabCombat);
}

float ItemSword::getStrVsBlock(const ItemStack&, const Block* block) const
{
    if (block == Blocks::web)
        return 15.0f;
    const Material& m = block->getMaterial();
    return &m != &Material::plants && &m != &Material::vine && &m != &Material::coral && &m != &Material::leaves && &m != &Material::gourd ? 1.0f : 1.5f;
}

bool ItemSword::canHarvestBlock(const Block* block) const
{
    return block == Blocks::web;
}

bool ItemSword::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const
{
    return material.getRepairItem() == repair.getItem() ? true : Item::getIsRepairable(toRepair, repair);
}

// --- ItemHoe ---

ItemHoe::ItemHoe(const ToolMaterial& material)
    : theToolMaterial(material)
{
    maxStackSize = 1;
    setMaxDamage(material.getMaxUses());
    setCreativeTab(&CreativeTabs::tabTools);
}

// --- ItemShears ---

ItemShears::ItemShears()
{
    setMaxStackSize(1);
    setMaxDamage(238);
    setCreativeTab(&CreativeTabs::tabTools);
}

bool ItemShears::canHarvestBlock(const Block* block) const
{
    return block == Blocks::web || block == Blocks::redstone_wire || block == Blocks::tripwire;
}

float ItemShears::getStrVsBlock(const ItemStack& stack, const Block* block) const
{
    if (block == Blocks::web || &block->getMaterial() == &Material::leaves)
        return 15.0f;
    return block == Blocks::wool ? 5.0f : Item::getStrVsBlock(stack, block);
}

bool ItemTool::hitEntity(ItemStack& stack, EntityLivingBase*, EntityLivingBase* attacker)
{
    damageItem(stack, 2, attacker);
    return true;
}

bool ItemTool::onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn)
{
    if (static_cast<double>(blockIn->getBlockHardness(worldIn, pos)) != 0.0)
        damageItem(stack, 1, playerIn);
    return true;
}

bool ItemSword::hitEntity(ItemStack& stack, EntityLivingBase*, EntityLivingBase* attacker)
{
    damageItem(stack, 1, attacker);
    return true;
}

bool ItemSword::onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn)
{
    if (static_cast<double>(blockIn->getBlockHardness(worldIn, pos)) != 0.0)
        damageItem(stack, 2, playerIn);
    return true;
}

std::shared_ptr<ItemStack> ItemSword::onItemRightClick(const std::shared_ptr<ItemStack>& itemStackIn, World*, EntityPlayer* playerIn)
{
    playerIn->setItemInUse(itemStackIn, getMaxItemUseDuration(*itemStackIn));
    return itemStackIn;
}

bool ItemHoe::onItemUse(ItemStack& stack, EntityPlayer* playerIn, World* worldIn, const BlockPos& pos, EnumFacing side, float, float, float)
{
    if (!playerIn->canPlayerEdit(pos.offset(side), side, &stack))
        return false;

    const IBlockState* iblockstate = worldIn->getBlockState(pos);
    Block* block = iblockstate->getBlock();

    if (side != EnumFacing::DOWN && &worldIn->getBlockState(pos.up())->getBlock()->getMaterial() == &Material::air) {
        if (block == Blocks::grass)
            return useHoe(stack, playerIn, worldIn, pos, Blocks::farmland->getDefaultState());

        if (block == Blocks::dirt) {
            switch (iblockstate->getValue(BlockDirt::VARIANT)) {
            case BlockDirt::DirtType::DIRT:
                return useHoe(stack, playerIn, worldIn, pos, Blocks::farmland->getDefaultState());
            case BlockDirt::DirtType::COARSE_DIRT:
                return useHoe(stack, playerIn, worldIn, pos, Blocks::dirt->getDefaultState()->withProperty(BlockDirt::VARIANT, BlockDirt::DirtType::DIRT));
            default:
                break;
            }
        }
    }

    return false;
}

bool ItemHoe::useHoe(ItemStack& stack, EntityPlayer* player, World* worldIn, const BlockPos& target, const IBlockState* newState)
{
    const Block::SoundType& sound = *newState->getBlock()->stepSound;
    worldIn->playSoundEffect(static_cast<double>(static_cast<float>(target.getX()) + 0.5f), static_cast<double>(static_cast<float>(target.getY()) + 0.5f),
        static_cast<double>(static_cast<float>(target.getZ()) + 0.5f), sound.getStepSound(), (sound.getVolume() + 1.0f) / 2.0f, sound.getFrequency() * 0.8f);

    if (worldIn->isRemote)
        return true;

    worldIn->setBlockState(target, newState);
    damageItem(stack, 1, player);
    return true;
}

bool ItemShears::onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityLivingBase* playerIn)
{
    if (&blockIn->getMaterial() != &Material::leaves && blockIn != Blocks::web && blockIn != Blocks::tallgrass && blockIn != Blocks::vine
        && blockIn != Blocks::tripwire && blockIn != Blocks::wool)
        return Item::onBlockDestroyed(stack, worldIn, blockIn, pos, playerIn);

    damageItem(stack, 1, playerIn);
    return true;
}

} // namespace mc
