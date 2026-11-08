#include "entity/EntityItemHooks.h"

#include "block/AllBlocks.h"
#include "block/material/Material.h"
#include "enchantment/Enchantment.h"
#include "enchantment/EnchantmentHelper.h"
#include "entity/EntityLivingBase.h"
#include "entity/SharedMonsterAttributes.h"
#include "entity/player/EntityPlayer.h"
#include "init/Blocks.h"
#include "item/ItemArmor.h"
#include "item/ItemStack.h"
#include "item/ItemTool.h"
#include "nbt/NBTTagCompound.h"
#include "potion/Potion.h"
#include "util/DamageSource.h"
#include "util/MathHelper.h"
#include "util/Random.h"

namespace mc {

namespace {

// Java: EnchantmentHelper.applyEnchantmentModifier -- calls `modifier` with each (enchantment, level) of the stack.
template <class Modifier>
void applyEnchantmentModifier(Modifier&& modifier, const ItemStack* stack)
{
    if (!stack)
        return;
    const std::shared_ptr<NBTTagList> nbttaglist = stack->getEnchantmentTagList();
    if (!nbttaglist)
        return;
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        const int j = nbttaglist->getCompoundTagAt(i)->getShort("id");
        const int k = nbttaglist->getCompoundTagAt(i)->getShort("lvl");
        if (Enchantment* enchantment = Enchantment::getEnchantmentById(j))
            modifier(*enchantment, k);
    }
}

template <class Modifier>
void applyEnchantmentModifierArray(Modifier&& modifier, const std::vector<std::shared_ptr<ItemStack>>& stacks)
{
    for (const auto& itemstack : stacks)
        applyEnchantmentModifier(modifier, itemstack.get());
}

// Java: Enchantment.calcModifierDamage (EnchantmentProtection's override; 0 for the others).
int calcModifierDamage(const Enchantment& enchantment, int level, const DamageSource& source)
{
    const auto* protection = dynamic_cast<const EnchantmentProtection*>(&enchantment);
    if (!protection)
        return 0;
    if (source.canHarmInCreative())
        return 0;
    const float f = static_cast<float>(6 + level * level) / 3.0f;
    const int type = protection->protectionType;
    if (type == 0)
        return MathHelper::floor_float(f * 0.75f);
    if (type == 1 && source.isFireDamage())
        return MathHelper::floor_float(f * 1.25f);
    if (type == 2 && &source == &DamageSource::fall)
        return MathHelper::floor_float(f * 2.5f);
    if (type == 3 && source.isExplosion())
        return MathHelper::floor_float(f * 1.5f);
    if (type == 4 && source.isProjectile())
        return MathHelper::floor_float(f * 1.5f);
    return 0;
}

// Java: Enchantment.calcDamageByCreature (EnchantmentDamage's override; 0 for the others).
float calcDamageByCreature(const Enchantment& enchantment, int level, EnumCreatureAttribute creatureType)
{
    const auto* damage = dynamic_cast<const EnchantmentDamage*>(&enchantment);
    if (!damage)
        return 0.0f;
    if (damage->damageType == 0)
        return static_cast<float>(level) * 1.25f;
    if (damage->damageType == 1 && creatureType == EnumCreatureAttribute::UNDEAD)
        return static_cast<float>(level) * 2.5f;
    if (damage->damageType == 2 && creatureType == EnumCreatureAttribute::ARTHROPOD)
        return static_cast<float>(level) * 2.5f;
    return 0.0f;
}

// Java: EnchantmentThorns.func_92094_a / func_92095_b
bool thornsShouldHit(int level, Random& rand)
{
    return level <= 0 ? false : rand.nextFloat() < 0.15f * static_cast<float>(level);
}

int thornsDamage(int level, Random& rand)
{
    return level > 10 ? level - 10 : 1 + rand.nextInt(4);
}

// Java: Enchantment.onUserHurt (EnchantmentThorns' override; empty for the others).
void onUserHurt(const Enchantment& enchantment, EntityLivingBase* user, Entity* attacker, int level)
{
    if (&enchantment != Enchantment::thorns)
        return;
    Random& random = user->getRNG();
    const std::shared_ptr<ItemStack> itemstack = EnchantmentHelper::getEnchantedItem(Enchantment::thorns, user);
    if (thornsShouldHit(level, random)) {
        if (attacker) {
            const std::shared_ptr<DamageSource> source = DamageSource::causeThornsDamage(user);
            const float amount = static_cast<float>(thornsDamage(level, random));
            attacker->attackEntityFrom(*source, amount);
            attacker->playSound("damage.thorns", 0.5f, 1.0f);
        }
        if (itemstack)
            damageItem(*itemstack, 3, user);
    } else if (itemstack) {
        damageItem(*itemstack, 1, user);
    }
}

// Java: Enchantment.onEntityDamaged (EnchantmentDamage's override; empty for the others).
void onEntityDamaged(const Enchantment& enchantment, EntityLivingBase* user, Entity* target, int level)
{
    const auto* damage = dynamic_cast<const EnchantmentDamage*>(&enchantment);
    if (!damage)
        return;
    if (auto* entitylivingbase = dynamic_cast<EntityLivingBase*>(target)) {
        if (damage->damageType == 2 && entitylivingbase->getCreatureAttribute() == EnumCreatureAttribute::ARTHROPOD) {
            const int i = 20 + user->getRNG().nextInt(10 * level);
            entitylivingbase->addPotionEffect(PotionEffect(Potion::moveSlowdown->id, i, 3));
        }
    }
}

Random& enchantmentRand()
{
    static Random rand;
    return rand;
}

} // namespace

const UUID& itemModifierUUID()
{
    static const UUID uuid = *UUID::fromString("CB3F55D3-645C-4F38-A497-9C13A33DB5CF");
    return uuid;
}

bool attemptDamageItem(ItemStack& stack, int amount, Random& rand)
{
    if (!stack.isItemStackDamageable())
        return false;
    if (amount > 0) {
        const int i = EnchantmentHelper::getEnchantmentLevel(Enchantment::unbreaking->effectId, &stack);
        int j = 0;
        for (int k = 0; i > 0 && k < amount; ++k) {
            // Java: EnchantmentDurability.negateDamage
            const bool negate = dynamic_cast<ItemArmor*>(stack.getItem()) && rand.nextFloat() < 0.6f ? false : rand.nextInt(i + 1) > 0;
            if (negate)
                ++j;
        }
        amount -= j;
        if (amount <= 0)
            return false;
    }
    stack.setItemDamage(stack.getItemDamage() + amount);
    return stack.getItemDamage() > stack.getMaxDamage();
}

void damageItem(ItemStack& stack, int amount, EntityLivingBase* entityIn)
{
    auto* entityplayer = dynamic_cast<EntityPlayer*>(entityIn);
    if (entityplayer && entityplayer->capabilities.isCreativeMode)
        return;
    if (!stack.isItemStackDamageable())
        return;
    if (attemptDamageItem(stack, amount, entityIn->getRNG())) {
        entityIn->renderBrokenItemStack(stack);
        --stack.stackSize;
        if (entityplayer) {
            // Java: triggerAchievement(StatList.objectBreakStats[id]) -- statistics come with the stats stage.
            if (stack.stackSize == 0 && stack.getItem()->getClassName() == "ItemBow")
                entityplayer->destroyCurrentEquippedItem();
        }
        if (stack.stackSize < 0)
            stack.stackSize = 0;
        stack.setItemDamage(0);
    }
}

void hitEntity(ItemStack& stack, EntityLivingBase*, EntityPlayer* playerIn)
{
    const Item* item = stack.getItem();
    if (dynamic_cast<const ItemSword*>(item))
        damageItem(stack, 1, playerIn);
    else if (dynamic_cast<const ItemTool*>(item))
        damageItem(stack, 2, playerIn);
}

void onBlockDestroyed(ItemStack& stack, World* worldIn, Block* blockIn, const BlockPos& pos, EntityPlayer* playerIn)
{
    const Item* item = stack.getItem();
    if (dynamic_cast<const ItemShears*>(item)) {
        if (&blockIn->getMaterial() != &Material::leaves && blockIn != Blocks::web && blockIn != Blocks::tallgrass && blockIn != Blocks::vine
            && blockIn != Blocks::tripwire && blockIn != Blocks::wool)
            return;
        damageItem(stack, 1, playerIn);
    } else if (dynamic_cast<const ItemSword*>(item)) {
        if (static_cast<double>(blockIn->getBlockHardness(worldIn, pos)) != 0.0)
            damageItem(stack, 2, playerIn);
    } else if (dynamic_cast<const ItemTool*>(item)) {
        if (static_cast<double>(blockIn->getBlockHardness(worldIn, pos)) != 0.0)
            damageItem(stack, 1, playerIn);
    }
}

AttributeModifierMultimap getAttributeModifiers(const ItemStack& stack)
{
    AttributeModifierMultimap multimap;
    if (stack.hasTagCompound() && stack.getTagCompound()->hasKey("AttributeModifiers", 9)) {
        const std::shared_ptr<NBTTagList> nbttaglist = stack.getTagCompound()->getTagList("AttributeModifiers", 10);
        for (int i = 0; i < nbttaglist->tagCount(); ++i) {
            const std::shared_ptr<NBTTagCompound> nbttagcompound = nbttaglist->getCompoundTagAt(i);
            const std::optional<AttributeModifier> attributemodifier = SharedMonsterAttributes::readAttributeModifierFromNBT(*nbttagcompound);
            if (attributemodifier && attributemodifier->getID().getLeastSignificantBits() != 0 && attributemodifier->getID().getMostSignificantBits() != 0)
                multimap.emplace_back(nbttagcompound->getString("AttributeName"), *attributemodifier);
        }
        return multimap;
    }
    // Java: Item.getItemAttributeModifiers -- empty but for swords and tools.
    const Item* item = stack.getItem();
    if (const auto* sword = dynamic_cast<const ItemSword*>(item)) {
        multimap.emplace_back(SharedMonsterAttributes::attackDamage().getAttributeUnlocalizedName(),
            AttributeModifier(itemModifierUUID(), "Weapon modifier", static_cast<double>(4.0f + sword->getDamageVsEntity()), 0));
    } else if (const auto* tool = dynamic_cast<const ItemTool*>(item)) {
        multimap.emplace_back(SharedMonsterAttributes::attackDamage().getAttributeUnlocalizedName(),
            AttributeModifier(itemModifierUUID(), "Tool modifier", static_cast<double>(tool->getDamageVsEntityValue()), 0));
    }
    return multimap;
}

namespace EnchantmentHelper {

int getEnchantmentModifierDamage(const std::vector<std::shared_ptr<ItemStack>>& stacks, const DamageSource& source)
{
    int damageModifier = 0;
    applyEnchantmentModifierArray([&](const Enchantment& enchantment, int level) { damageModifier += calcModifierDamage(enchantment, level, source); }, stacks);
    if (damageModifier > 25)
        damageModifier = 25;
    else if (damageModifier < 0)
        damageModifier = 0;
    return (damageModifier + 1 >> 1) + enchantmentRand().nextInt((damageModifier >> 1) + 1);
}

float getModifierForCreature(const ItemStack* stack, EnumCreatureAttribute creatureAttribute)
{
    float livingModifier = 0.0f;
    applyEnchantmentModifier([&](const Enchantment& enchantment, int level) { livingModifier += calcDamageByCreature(enchantment, level, creatureAttribute); }, stack);
    return livingModifier;
}

void applyThornEnchantments(EntityLivingBase* user, Entity* attacker)
{
    const auto modifier = [&](const Enchantment& enchantment, int level) { onUserHurt(enchantment, user, attacker, level); };
    if (user)
        applyEnchantmentModifierArray(modifier, user->getInventory());
    if (dynamic_cast<EntityPlayer*>(attacker))
        applyEnchantmentModifier(modifier, user->getHeldItem().get());
}

void applyArthropodEnchantments(EntityLivingBase* user, Entity* target)
{
    const auto modifier = [&](const Enchantment& enchantment, int level) { onEntityDamaged(enchantment, user, target, level); };
    if (user)
        applyEnchantmentModifierArray(modifier, user->getInventory());
    if (dynamic_cast<EntityPlayer*>(user))
        applyEnchantmentModifier(modifier, user->getHeldItem().get());
}

int getMaxEnchantmentLevel(int enchID, const std::vector<std::shared_ptr<ItemStack>>& stacks)
{
    int i = 0;
    for (const auto& itemstack : stacks) {
        const int j = getEnchantmentLevel(enchID, itemstack.get());
        if (j > i)
            i = j;
    }
    return i;
}

int getKnockbackModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::knockback->effectId, player->getHeldItem().get());
}

int getFireAspectModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::fireAspect->effectId, player->getHeldItem().get());
}

int getRespiration(Entity* player)
{
    return getMaxEnchantmentLevel(Enchantment::respiration->effectId, player->getInventory());
}

int getDepthStriderModifier(Entity* player)
{
    return getMaxEnchantmentLevel(Enchantment::depthStrider->effectId, player->getInventory());
}

int getEfficiencyModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::efficiency->effectId, player->getHeldItem().get());
}

bool getSilkTouchModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::silkTouch->effectId, player->getHeldItem().get()) > 0;
}

int getFortuneModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::fortune->effectId, player->getHeldItem().get());
}

int getLuckOfSeaModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::luckOfTheSea->effectId, player->getHeldItem().get());
}

int getLureModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::lure->effectId, player->getHeldItem().get());
}

int getLootingModifier(EntityLivingBase* player)
{
    return getEnchantmentLevel(Enchantment::looting->effectId, player->getHeldItem().get());
}

bool getAquaAffinityModifier(EntityLivingBase* player)
{
    return getMaxEnchantmentLevel(Enchantment::aquaAffinity->effectId, player->getInventory()) > 0;
}

std::shared_ptr<ItemStack> getEnchantedItem(const Enchantment* enchantment, EntityLivingBase* entity)
{
    for (const auto& itemstack : entity->getInventory()) {
        if (itemstack && getEnchantmentLevel(enchantment->effectId, itemstack.get()) > 0)
            return itemstack;
    }
    return nullptr;
}

} // namespace EnchantmentHelper

int getFireTimeForEntity(Entity* entity, int fireTime)
{
    const int i = EnchantmentHelper::getMaxEnchantmentLevel(Enchantment::fireProtection->effectId, entity->getInventory());
    if (i > 0)
        fireTime -= MathHelper::floor_float(static_cast<float>(fireTime) * static_cast<float>(i) * 0.15f);
    return fireTime;
}

double func_92092_a(Entity* entity, double strength)
{
    const int i = EnchantmentHelper::getMaxEnchantmentLevel(Enchantment::blastProtection->effectId, entity->getInventory());
    if (i > 0)
        strength -= static_cast<double>(MathHelper::floor_double(strength * static_cast<double>(static_cast<float>(i) * 0.15f)));
    return strength;
}

} // namespace mc
