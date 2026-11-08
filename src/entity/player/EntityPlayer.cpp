#include "entity/player/EntityPlayer.h"

#include "block/AllBlocks.h"
#include "block/BlockWorldHelpers.h"
#include "block/material/Material.h"
#include "core/Java.h"
#include "enchantment/EnchantmentHelper.h"
#include "entity/EntityItemHooks.h"
#include "entity/SharedMonsterAttributes.h"
#include "entity/player/EntityPlayerMP.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "inventory/ContainerPlayer.h"
#include "inventory/CopyItemStack.h"
#include "inventory/InventoryBasic.h"
#include "item/ItemArmor.h"
#include "item/ItemBlock.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"
#include "network/NetHandlerPlayServer.h"
#include "network/play/ServerPlayPackets.h"
#include "util/ChatComponent.h"
#include "util/DamageSource.h"
#include "util/EnumParticleTypes.h"
#include "util/MathHelper.h"
#include "world/EnumDifficulty.h"
#include "world/GameRules.h"
#include "world/World.h"
#include "world/WorldProvider.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace mc {

namespace {

constexpr float kPiF = 3.1415927f;

// Java: EntityLiving.getArmorPosition -- 4 for heads, 3..1 for chest, legs and feet, 0 for the hand.
int getArmorPosition(const ItemStack& stack)
{
    if (stack.getItem() != Item::getItemFromBlock(Blocks::pumpkin) && stack.getItem() != Items::skull) {
        if (const auto* armor = dynamic_cast<const ItemArmor*>(stack.getItem())) {
            switch (armor->armorType) {
            case 0: return 4;
            case 1: return 3;
            case 2: return 2;
            case 3: return 1;
            default: break;
            }
        }
        return 0;
    }
    return 4;
}

} // namespace

EntityPlayer::EntityPlayer(World* worldIn, GameProfile gameProfileIn)
    : EntityLivingBase(worldIn)
    , theInventoryEnderChest(std::make_shared<InventoryEnderChest>())
    , gameProfile(std::move(gameProfileIn))
{
}

EntityPlayer::~EntityPlayer() = default;

void EntityPlayer::construct()
{
    EntityLivingBase::construct();
    entityUniqueID = getUUID(gameProfile);
    inventoryContainer = std::make_shared<ContainerPlayer>(inventory, !worldObj->isRemote, this);
    setOpenContainer(inventoryContainer);
    const BlockPos blockpos = worldObj->getSpawnPoint();
    setLocationAndAngles(static_cast<double>(blockpos.getX()) + 0.5, static_cast<double>(blockpos.getY() + 1), static_cast<double>(blockpos.getZ()) + 0.5,
        0.0f, 0.0f);
    unused180 = 180.0f;
    fireResistance = 20;
}

void EntityPlayer::applyEntityAttributes()
{
    EntityLivingBase::applyEntityAttributes();
    getAttributeMap().registerAttribute(SharedMonsterAttributes::attackDamage()).setBaseValue(1.0);
    getEntityAttribute(SharedMonsterAttributes::movementSpeed())->setBaseValue(0.10000000149011612);
}

void EntityPlayer::entityInit()
{
    EntityLivingBase::entityInit();
    dataWatcher.addObject(16, int8_t(0));
    dataWatcher.addObject(17, 0.0f);
    dataWatcher.addObject(18, int32_t(0));
    dataWatcher.addObject(10, int8_t(0));
}

int EntityPlayer::getItemInUseDuration() const
{
    return isUsingItem() ? itemInUse->getMaxItemUseDuration() - itemInUseCount : 0;
}

void EntityPlayer::stopUsingItem()
{
    // Java: itemInUse.onPlayerStoppedUsing(world, this, itemInUseCount) -- bows; with the item hooks.
    clearItemInUse();
}

void EntityPlayer::clearItemInUse()
{
    itemInUse = nullptr;
    itemInUseCount = 0;
    if (!worldObj->isRemote)
        setEating(false);
}

bool EntityPlayer::isBlocking() const
{
    return isUsingItem() && itemInUse->getItem()->getItemUseAction(*itemInUse) == EnumAction::BLOCK;
}

void EntityPlayer::onUpdate()
{
    noClip = isSpectator();
    if (isSpectator())
        onGround = false;
    if (itemInUse) {
        const std::shared_ptr<ItemStack> itemstack = inventory.getCurrentItem();
        if (itemstack == itemInUse) {
            if (itemInUseCount <= 25 && itemInUseCount % 4 == 0)
                updateItemUse(*itemstack, 5);
            if (--itemInUseCount == 0 && !worldObj->isRemote)
                onItemUseFinish();
        } else {
            clearItemInUse();
        }
    }
    if (xpCooldown > 0)
        --xpCooldown;
    if (isPlayerSleeping()) {
        ++sleepTimer;
        if (sleepTimer > 100)
            sleepTimer = 100;
        if (!worldObj->isRemote) {
            if (!isInBed())
                wakeUpPlayer(true, true, false);
            else if (worldObj->isDaytime())
                wakeUpPlayer(false, true, true);
        }
    } else if (sleepTimer > 0) {
        ++sleepTimer;
        if (sleepTimer >= 110)
            sleepTimer = 0;
    }
    EntityLivingBase::onUpdate();
    if (!worldObj->isRemote && openContainer && !openContainer->canInteractWith(this)) {
        closeScreen();
        setOpenContainer(inventoryContainer);
    }
    if (isBurning() && capabilities.disableDamage)
        extinguish();
    prevChasingPosX = chasingPosX;
    prevChasingPosY = chasingPosY;
    prevChasingPosZ = chasingPosZ;
    const double d5 = posX - chasingPosX;
    const double d0 = posY - chasingPosY;
    const double d1 = posZ - chasingPosZ;
    const double d2 = 10.0;
    if (d5 > d2)
        prevChasingPosX = chasingPosX = posX;
    if (d1 > d2)
        prevChasingPosZ = chasingPosZ = posZ;
    if (d0 > d2)
        prevChasingPosY = chasingPosY = posY;
    if (d5 < -d2)
        prevChasingPosX = chasingPosX = posX;
    if (d1 < -d2)
        prevChasingPosZ = chasingPosZ = posZ;
    if (d0 < -d2)
        prevChasingPosY = chasingPosY = posY;
    chasingPosX += d5 * 0.25;
    chasingPosZ += d1 * 0.25;
    chasingPosY += d0 * 0.25;
    if (!ridingEntity)
        startMinecartRidingCoordinate.reset();
    if (!worldObj->isRemote) {
        foodStats.onUpdate(this);
        // Java: triggerAchievement(minutesPlayedStat) and, while alive, timeSinceDeathStat -- statistics.
    }
    const double d3 = MathHelper::clamp_double(posX, -2.9999999E7, 2.9999999E7);
    const double d4 = MathHelper::clamp_double(posZ, -2.9999999E7, 2.9999999E7);
    if (d3 != posX || d4 != posZ)
        setPosition(d3, posY, d4);
}

void EntityPlayer::playSound(const std::string& name, float volume, float pitch)
{
    worldObj->playSoundToNearExcept(this, name, volume, pitch);
}

void EntityPlayer::updateItemUse(const ItemStack& itemStackIn, int p_71010_2_)
{
    if (itemStackIn.getItemUseAction() == EnumAction::DRINK)
        playSound("random.drink", 0.5f, worldObj->rand.nextFloat() * 0.1f + 0.9f);
    if (itemStackIn.getItemUseAction() == EnumAction::EAT) {
        for (int i = 0; i < p_71010_2_; ++i) {
            const double vx = (static_cast<double>(rand.nextFloat()) - 0.5) * 0.1;
            Vec3 vec3(vx, mathRandom() * 0.1 + 0.1, 0.0);
            vec3 = vec3.rotatePitch(-rotationPitch * kPiF / 180.0f);
            vec3 = vec3.rotateYaw(-rotationYaw * kPiF / 180.0f);
            const double d0 = static_cast<double>(-rand.nextFloat()) * 0.6 - 0.3;
            Vec3 vec31((static_cast<double>(rand.nextFloat()) - 0.5) * 0.3, d0, 0.6);
            vec31 = vec31.rotatePitch(-rotationPitch * kPiF / 180.0f);
            vec31 = vec31.rotateYaw(-rotationYaw * kPiF / 180.0f);
            vec31 = vec31.addVector(posX, posY + static_cast<double>(getEyeHeight()), posZ);
            if (itemStackIn.getHasSubtypes()) {
                spawnParticle(worldObj, EnumParticleTypes::ITEM_CRACK, vec31.xCoord, vec31.yCoord, vec31.zCoord, vec3.xCoord, vec3.yCoord + 0.05, vec3.zCoord,
                    { Item::getIdFromItem(itemStackIn.getItem()), itemStackIn.getMetadata() });
            } else {
                spawnParticle(worldObj, EnumParticleTypes::ITEM_CRACK, vec31.xCoord, vec31.yCoord, vec31.zCoord, vec3.xCoord, vec3.yCoord + 0.05, vec3.zCoord,
                    { Item::getIdFromItem(itemStackIn.getItem()) });
            }
        }
        const float volume = 0.5f + 0.5f * static_cast<float>(rand.nextInt(2));
        const float a = rand.nextFloat();
        const float pitch = (a - rand.nextFloat()) * 0.2f + 1.0f;
        playSound("random.eat", volume, pitch);
    }
}

void EntityPlayer::onItemUseFinish()
{
    if (!itemInUse)
        return;
    updateItemUse(*itemInUse, 16);
    const int i = itemInUse->stackSize;
    // Java: itemInUse.onItemUseFinish(world, this) -- Item.onItemUseFinish (food, potions, milk) comes with the
    // item hooks; until then the stack comes back unchanged.
    const std::shared_ptr<ItemStack> itemstack = itemInUse;
    if (itemstack != itemInUse || (itemstack && itemstack->stackSize != i)) {
        inventory.mainInventory[static_cast<size_t>(inventory.currentItem)] = itemstack;
        if (itemstack->stackSize == 0)
            inventory.mainInventory[static_cast<size_t>(inventory.currentItem)] = nullptr;
    }
    clearItemInUse();
}

void EntityPlayer::handleStatusUpdate(int8_t id)
{
    if (id == 9)
        onItemUseFinish();
    else if (id == 23)
        reducedDebug = false;
    else if (id == 22)
        reducedDebug = true;
    else
        EntityLivingBase::handleStatusUpdate(id);
}

void EntityPlayer::closeScreen()
{
    setOpenContainer(inventoryContainer);
}

void EntityPlayer::updateRidden()
{
    if (!worldObj->isRemote && isSneaking()) {
        mountEntity(nullptr);
        setSneaking(false);
    } else {
        const double d0 = posX;
        const double d1 = posY;
        const double d2 = posZ;
        const float f = rotationYaw;
        const float f1 = rotationPitch;
        EntityLivingBase::updateRidden();
        prevCameraYaw = cameraYaw;
        cameraYaw = 0.0f;
        addMountedMovementStat(posX - d0, posY - d1, posZ - d2);
        if (ridingEntity && ridingEntity->getClassName() == "EntityPig") {
            rotationPitch = f1;
            rotationYaw = f;
            renderYawOffset = static_cast<EntityLivingBase*>(ridingEntity.get())->renderYawOffset;
        }
    }
}

void EntityPlayer::preparePlayerToSpawn()
{
    setSize(0.6f, 1.8f);
    EntityLivingBase::preparePlayerToSpawn();
    setHealth(getMaxHealth());
    deathTime = 0;
}

void EntityPlayer::updateEntityActionState()
{
    EntityLivingBase::updateEntityActionState();
    updateArmSwingProgress();
    rotationYawHead = rotationYaw;
}

void EntityPlayer::onLivingUpdate()
{
    if (flyToggleTimer > 0)
        --flyToggleTimer;
    if (worldObj->getDifficulty() == EnumDifficulty::PEACEFUL && worldObj->getGameRules().getBoolean("naturalRegeneration")) {
        if (getHealth() < getMaxHealth() && ticksExisted % 20 == 0)
            heal(1.0f);
        if (foodStats.needFood() && ticksExisted % 10 == 0)
            foodStats.setFoodLevel(foodStats.getFoodLevel() + 1);
    }
    inventory.decrementAnimations();
    prevCameraYaw = cameraYaw;
    EntityLivingBase::onLivingUpdate();
    IAttributeInstance* iattributeinstance = getEntityAttribute(SharedMonsterAttributes::movementSpeed());
    if (!worldObj->isRemote)
        iattributeinstance->setBaseValue(static_cast<double>(capabilities.getWalkSpeed()));
    jumpMovementFactor = speedInAir;
    if (isSprinting())
        jumpMovementFactor = static_cast<float>(static_cast<double>(jumpMovementFactor) + static_cast<double>(speedInAir) * 0.3);
    setAIMoveSpeed(static_cast<float>(iattributeinstance->getAttributeValue()));
    float f = MathHelper::sqrt_double(motionX * motionX + motionZ * motionZ);
    // Java: Math.atan (StrictMath's fdlibm); std::atan is correctly rounded for these small arguments.
    float f1 = static_cast<float>(std::atan(-motionY * 0.20000000298023224) * 15.0);
    if (f > 0.1f)
        f = 0.1f;
    if (!onGround || getHealth() <= 0.0f)
        f = 0.0f;
    if (onGround || getHealth() <= 0.0f)
        f1 = 0.0f;
    cameraYaw += (f - cameraYaw) * 0.4f;
    cameraPitch += (f1 - cameraPitch) * 0.8f;
    if (getHealth() > 0.0f && !isSpectator()) {
        AxisAlignedBB axisalignedbb = ridingEntity && !ridingEntity->isDead
            ? getEntityBoundingBox().unionWith(ridingEntity->getEntityBoundingBox()).expand(1.0, 0.0, 1.0)
            : getEntityBoundingBox().expand(1.0, 0.5, 1.0);
        const std::vector<Entity*> list = worldObj->getEntitiesWithinAABBExcludingEntity(this, axisalignedbb);
        for (Entity* entity : list) {
            if (!entity->isDead)
                collideWithPlayer(entity);
        }
    }
}

void EntityPlayer::addScore(int p_85039_1_)
{
    const int i = getScore();
    dataWatcher.updateObject(18, static_cast<int32_t>(i + p_85039_1_));
}

void EntityPlayer::onDeath(const DamageSource& cause)
{
    EntityLivingBase::onDeath(cause);
    setSize(0.2f, 0.2f);
    setPosition(posX, posY, posZ);
    motionY = 0.10000000149011612;
    if (getName() == "Notch")
        dropItem(std::make_shared<ItemStack>(Items::apple, 1), true, false);
    if (!worldObj->getGameRules().getBoolean("keepInventory"))
        inventory.dropAllItems();
    motionX = static_cast<double>(-MathHelper::cos((attackedAtYaw + rotationYaw) * kPiF / 180.0f) * 0.1f);
    motionZ = static_cast<double>(-MathHelper::sin((attackedAtYaw + rotationYaw) * kPiF / 180.0f) * 0.1f);
    // Java: triggerAchievement(deathsStat), func_175145_a(timeSinceDeathStat) -- statistics.
}

void EntityPlayer::addToPlayerScore(Entity*, int amount)
{
    addScore(amount);
    // Java: the kill statistics and the totalKillCount / playerKillCount / team-kill objectives -- statistics
    // and the scoreboard.
}

std::shared_ptr<EntityItem> EntityPlayer::dropOneItem(bool dropAll)
{
    const std::shared_ptr<ItemStack> current = inventory.getCurrentItem();
    return dropItem(inventory.decrStackSize(inventory.currentItem, dropAll && current ? current->stackSize : 1), false, true);
}

std::shared_ptr<EntityItem> EntityPlayer::dropPlayerItemWithRandomChoice(const std::shared_ptr<ItemStack>& itemStackIn, bool)
{
    return dropItem(itemStackIn, false, false);
}

std::shared_ptr<EntityItem> EntityPlayer::dropItem(const std::shared_ptr<ItemStack>& droppedItem, bool dropAround, bool traceItem)
{
    if (!droppedItem || droppedItem->stackSize == 0)
        return nullptr;
    // Java: spawns an EntityItem at the eyes (posY - 0.3 + eye height) with a pickup delay of 40, thrown
    // around (dropAround) or ahead with a random spread, through joinEntityItemWithWorld; traced drops record
    // the thrower and the drop statistic. Item entities come with EntityItem: the stack is lost until then.
    (void)dropAround;
    (void)traceItem;
    return nullptr;
}

void EntityPlayer::joinEntityItemWithWorld(const std::shared_ptr<EntityItem>& itemIn)
{
    // Java: worldObj.spawnEntityInWorld(itemIn) -- with EntityItem.
    (void)itemIn;
}

float EntityPlayer::getToolDigEfficiency(const Block* p_180471_1_)
{
    float f = inventory.getStrVsBlock(p_180471_1_);
    if (f > 1.0f) {
        const int i = EnchantmentHelper::getEfficiencyModifier(this);
        const std::shared_ptr<ItemStack> itemstack = inventory.getCurrentItem();
        if (i > 0 && itemstack)
            f += static_cast<float>(i * i + 1);
    }
    if (const PotionEffect* haste = getActivePotionEffect(Potion::digSpeed))
        f *= 1.0f + static_cast<float>(haste->getAmplifier() + 1) * 0.2f;
    if (const PotionEffect* fatigue = getActivePotionEffect(Potion::digSlowdown)) {
        float f1 = 1.0f;
        switch (fatigue->getAmplifier()) {
        case 0:
            f1 = 0.3f;
            break;
        case 1:
            f1 = 0.09f;
            break;
        case 2:
            f1 = 0.0027f;
            break;
        case 3:
        default:
            f1 = 8.1E-4f;
        }
        f *= f1;
    }
    if (isInsideOfMaterial(Material::water) && !EnchantmentHelper::getAquaAffinityModifier(this))
        f /= 5.0f;
    if (!onGround)
        f /= 5.0f;
    return f;
}

void EntityPlayer::readEntityFromNBT(const NBTTagCompound& tagCompund)
{
    EntityLivingBase::readEntityFromNBT(tagCompund);
    entityUniqueID = getUUID(gameProfile);
    const std::shared_ptr<NBTTagList> nbttaglist = tagCompund.getTagList("Inventory", 10);
    inventory.readFromNBT(*nbttaglist);
    inventory.currentItem = tagCompund.getInteger("SelectedItemSlot");
    sleeping = tagCompund.getBoolean("Sleeping");
    sleepTimer = tagCompund.getShort("SleepTimer");
    experience = tagCompund.getFloat("XpP");
    experienceLevel = tagCompund.getInteger("XpLevel");
    experienceTotal = tagCompund.getInteger("XpTotal");
    xpSeed = tagCompund.getInteger("XpSeed");
    if (xpSeed == 0)
        xpSeed = rand.nextInt();
    setScore(tagCompund.getInteger("Score"));
    if (sleeping) {
        playerLocation = BlockPos(posX, posY, posZ);
        wakeUpPlayer(true, true, false);
    }
    if (tagCompund.hasKey("SpawnX", 99) && tagCompund.hasKey("SpawnY", 99) && tagCompund.hasKey("SpawnZ", 99)) {
        spawnChunk = BlockPos(tagCompund.getInteger("SpawnX"), tagCompund.getInteger("SpawnY"), tagCompund.getInteger("SpawnZ"));
        spawnForced = tagCompund.getBoolean("SpawnForced");
    }
    foodStats.readNBT(tagCompund);
    capabilities.readCapabilitiesFromNBT(tagCompund);
    if (tagCompund.hasKey("EnderItems", 9)) {
        const std::shared_ptr<NBTTagList> nbttaglist1 = tagCompund.getTagList("EnderItems", 10);
        theInventoryEnderChest->loadInventoryFromNBT(*nbttaglist1);
    }
}

void EntityPlayer::writeEntityToNBT(NBTTagCompound& tagCompound)
{
    EntityLivingBase::writeEntityToNBT(tagCompound);
    auto inventoryList = std::make_shared<NBTTagList>();
    inventory.writeToNBT(*inventoryList);
    tagCompound.setTag("Inventory", inventoryList);
    tagCompound.setInteger("SelectedItemSlot", inventory.currentItem);
    tagCompound.setBoolean("Sleeping", sleeping);
    tagCompound.setShort("SleepTimer", static_cast<int16_t>(sleepTimer));
    tagCompound.setFloat("XpP", experience);
    tagCompound.setInteger("XpLevel", experienceLevel);
    tagCompound.setInteger("XpTotal", experienceTotal);
    tagCompound.setInteger("XpSeed", xpSeed);
    tagCompound.setInteger("Score", getScore());
    if (spawnChunk) {
        tagCompound.setInteger("SpawnX", spawnChunk->getX());
        tagCompound.setInteger("SpawnY", spawnChunk->getY());
        tagCompound.setInteger("SpawnZ", spawnChunk->getZ());
        tagCompound.setBoolean("SpawnForced", spawnForced);
    }
    foodStats.writeNBT(tagCompound);
    capabilities.writeCapabilitiesToNBT(tagCompound);
    tagCompound.setTag("EnderItems", theInventoryEnderChest->saveInventoryToNBT());
    const std::shared_ptr<ItemStack> itemstack = inventory.getCurrentItem();
    if (itemstack && itemstack->getItem()) {
        auto selected = std::make_shared<NBTTagCompound>();
        itemstack->writeToNBT(*selected);
        tagCompound.setTag("SelectedItem", selected);
    }
}

bool EntityPlayer::attackEntityFrom(const DamageSource& source, float amount)
{
    if (isEntityInvulnerable(source))
        return false;
    if (capabilities.disableDamage && !source.canHarmInCreative())
        return false;
    entityAge = 0;
    if (getHealth() <= 0.0f)
        return false;
    if (isPlayerSleeping() && !worldObj->isRemote)
        wakeUpPlayer(true, true, false);
    if (source.isDifficultyScaled()) {
        if (worldObj->getDifficulty() == EnumDifficulty::PEACEFUL)
            amount = 0.0f;
        if (worldObj->getDifficulty() == EnumDifficulty::EASY)
            amount = amount / 2.0f + 1.0f;
        if (worldObj->getDifficulty() == EnumDifficulty::HARD)
            amount = amount * 3.0f / 2.0f;
    }
    if (amount == 0.0f)
        return false;
    return EntityLivingBase::attackEntityFrom(source, amount);
}

bool EntityPlayer::canAttackPlayer(EntityPlayer*)
{
    // Java: true without a team, else the team's friendly-fire rule -- teams come with the scoreboard.
    return true;
}

float EntityPlayer::getArmorVisibility() const
{
    int i = 0;
    for (const auto& itemstack : inventory.armorInventory) {
        if (itemstack)
            ++i;
    }
    return static_cast<float>(i) / static_cast<float>(inventory.armorInventory.size());
}

void EntityPlayer::damageEntity(const DamageSource& damageSrc, float damageAmount)
{
    if (isEntityInvulnerable(damageSrc))
        return;
    if (!damageSrc.isUnblockable() && isBlocking() && damageAmount > 0.0f)
        damageAmount = (1.0f + damageAmount) * 0.5f;
    damageAmount = applyArmorCalculations(damageSrc, damageAmount);
    damageAmount = applyPotionDamageCalculations(damageSrc, damageAmount);
    const float f = damageAmount;
    damageAmount = std::max(damageAmount - getAbsorptionAmount(), 0.0f);
    setAbsorptionAmount(getAbsorptionAmount() - (f - damageAmount));
    if (damageAmount != 0.0f) {
        addExhaustion(damageSrc.getHungerDamage());
        const float f1 = getHealth();
        setHealth(getHealth() - damageAmount);
        getCombatTracker().trackDamage(damageSrc, f1, damageAmount);
        // Java: addStat(damageTakenStat, round(damage * 10)) below 3.4028235E37 -- statistics.
    }
}

bool EntityPlayer::interactWith(Entity* targetEntity)
{
    if (isSpectator()) {
        if (auto* inventoryIn = dynamic_cast<IInventory*>(targetEntity))
            displayGUIChest(inventoryIn);
        return false;
    }
    std::shared_ptr<ItemStack> itemstack = getCurrentEquippedItem();
    const std::shared_ptr<ItemStack> itemstack1 = copyItemStack(itemstack);
    if (!targetEntity->interactFirst(this)) {
        if (itemstack && dynamic_cast<EntityLivingBase*>(targetEntity)) {
            if (capabilities.isCreativeMode)
                itemstack = itemstack1;
            // Java: itemstack.interactWithEntity(this, target) -- Item.itemInteractionForEntity (shears,
            // saddles, name tags, dyes) comes with the item hooks: nothing interacts yet.
        }
        return false;
    }
    if (itemstack && itemstack == getCurrentEquippedItem()) {
        if (itemstack->stackSize <= 0 && !capabilities.isCreativeMode)
            destroyCurrentEquippedItem();
        else if (itemstack->stackSize < itemstack1->stackSize && capabilities.isCreativeMode)
            itemstack->stackSize = itemstack1->stackSize;
    }
    return true;
}

void EntityPlayer::destroyCurrentEquippedItem()
{
    inventory.setInventorySlotContents(inventory.currentItem, nullptr);
}

void EntityPlayer::attackTargetEntityWithCurrentItem(Entity* targetEntity)
{
    if (!targetEntity->canAttackWithItem())
        return;
    if (targetEntity->hitByEntity(this))
        return;
    float f = static_cast<float>(getEntityAttribute(SharedMonsterAttributes::attackDamage())->getAttributeValue());
    int i = 0;
    float f1 = 0.0f;
    auto* targetLiving = dynamic_cast<EntityLivingBase*>(targetEntity);
    if (targetLiving)
        f1 = EnchantmentHelper::getModifierForCreature(getHeldItem().get(), targetLiving->getCreatureAttribute());
    else
        f1 = EnchantmentHelper::getModifierForCreature(getHeldItem().get(), EnumCreatureAttribute::UNDEFINED);
    i = i + EnchantmentHelper::getKnockbackModifier(this);
    if (isSprinting())
        ++i;
    if (f > 0.0f || f1 > 0.0f) {
        const bool flag = fallDistance > 0.0f && !onGround && !isOnLadder() && !isInWater() && !isPotionActive(Potion::blindness) && !ridingEntity
            && targetLiving;
        if (flag && f > 0.0f)
            f *= 1.5f;
        f = f + f1;
        bool flag1 = false;
        const int j = EnchantmentHelper::getFireAspectModifier(this);
        if (targetLiving && j > 0 && !targetEntity->isBurning()) {
            flag1 = true;
            targetEntity->setFire(1);
        }
        const double d0 = targetEntity->motionX;
        const double d1 = targetEntity->motionY;
        const double d2 = targetEntity->motionZ;
        const std::shared_ptr<DamageSource> source = DamageSource::causePlayerDamage(this);
        const bool flag2 = targetEntity->attackEntityFrom(*source, f);
        if (flag2) {
            if (i > 0) {
                targetEntity->addVelocity(static_cast<double>(-MathHelper::sin(rotationYaw * kPiF / 180.0f) * static_cast<float>(i) * 0.5f), 0.1,
                    static_cast<double>(MathHelper::cos(rotationYaw * kPiF / 180.0f) * static_cast<float>(i) * 0.5f));
                motionX *= 0.6;
                motionZ *= 0.6;
                setSprinting(false);
            }
            if (auto* targetPlayer = dynamic_cast<EntityPlayerMP*>(targetEntity); targetPlayer && targetEntity->velocityChanged) {
                targetPlayer->playerNetServerHandler->sendPacket(std::make_shared<S12PacketEntityVelocity>(targetEntity));
                targetEntity->velocityChanged = false;
                targetEntity->motionX = d0;
                targetEntity->motionY = d1;
                targetEntity->motionZ = d2;
            }
            if (flag)
                onCriticalHit(targetEntity);
            if (f1 > 0.0f)
                onEnchantmentCritical(targetEntity);
            // Java: f >= 18 triggers AchievementList.overkill -- achievements.
            setLastAttacker(targetEntity);
            if (targetLiving)
                EnchantmentHelper::applyThornEnchantments(targetLiving, this);
            EnchantmentHelper::applyArthropodEnchantments(this, targetEntity);
            const std::shared_ptr<ItemStack> itemstack = getCurrentEquippedItem();
            // Java: an EntityDragonPart target stands for its dragon -- with the dragon.
            Entity* entity = targetEntity;
            if (itemstack) {
                if (auto* entityLiving = dynamic_cast<EntityLivingBase*>(entity)) {
                    hitEntity(*itemstack, entityLiving, this);
                    if (itemstack->stackSize <= 0)
                        destroyCurrentEquippedItem();
                }
            }
            if (targetLiving) {
                // Java: addStat(damageDealtStat, round(f * 10)) -- statistics.
                if (j > 0)
                    targetEntity->setFire(j * 4);
            }
            addExhaustion(0.3f);
        } else if (flag1) {
            targetEntity->extinguish();
        }
    }
}

void EntityPlayer::setDead()
{
    EntityLivingBase::setDead();
    inventoryContainer->onContainerClosed(this);
    if (openContainer)
        openContainer->onContainerClosed(this);
}

EntityPlayer::EnumStatus EntityPlayer::trySleep(const BlockPos& bedLocation)
{
    if (!worldObj->isRemote) {
        if (isPlayerSleeping() || !isEntityAlive())
            return EnumStatus::OTHER_PROBLEM;
        if (!worldObj->provider->isSurfaceWorld())
            return EnumStatus::NOT_POSSIBLE_HERE;
        if (worldObj->isDaytime())
            return EnumStatus::NOT_POSSIBLE_NOW;
        if (std::abs(posX - static_cast<double>(bedLocation.getX())) > 3.0 || std::abs(posY - static_cast<double>(bedLocation.getY())) > 2.0
            || std::abs(posZ - static_cast<double>(bedLocation.getZ())) > 3.0)
            return EnumStatus::TOO_FAR_AWAY;
        // Java: NOT_SAFE when an EntityMob is within 8 x 5 x 8 of the bed -- mobs come with their stage.
    }
    if (isRiding())
        mountEntity(nullptr);
    setSize(0.2f, 0.2f);
    if (worldObj->isBlockLoaded(bedLocation)) {
        const EnumFacing enumfacing = worldObj->getBlockState(bedLocation)->getValue(BlockDirectional::FACING);
        float f = 0.5f;
        float f1 = 0.5f;
        switch (enumfacing) {
        case EnumFacing::SOUTH:
            f1 = 0.9f;
            break;
        case EnumFacing::NORTH:
            f1 = 0.1f;
            break;
        case EnumFacing::WEST:
            f = 0.1f;
            break;
        case EnumFacing::EAST:
            f = 0.9f;
            break;
        default:
            break;
        }
        func_175139_a(enumfacing);
        setPosition(static_cast<double>(static_cast<float>(bedLocation.getX()) + f), static_cast<double>(static_cast<float>(bedLocation.getY()) + 0.6875f),
            static_cast<double>(static_cast<float>(bedLocation.getZ()) + f1));
    } else {
        setPosition(static_cast<double>(static_cast<float>(bedLocation.getX()) + 0.5f), static_cast<double>(static_cast<float>(bedLocation.getY()) + 0.6875f),
            static_cast<double>(static_cast<float>(bedLocation.getZ()) + 0.5f));
    }
    sleeping = true;
    sleepTimer = 0;
    playerLocation = bedLocation;
    motionX = motionZ = motionY = 0.0;
    if (!worldObj->isRemote)
        worldObj->updateAllPlayersSleepingFlag();
    return EnumStatus::OK;
}

void EntityPlayer::func_175139_a(EnumFacing p_175139_1_)
{
    renderOffsetX = 0.0f;
    renderOffsetZ = 0.0f;
    switch (p_175139_1_) {
    case EnumFacing::SOUTH:
        renderOffsetZ = -1.8f;
        break;
    case EnumFacing::NORTH:
        renderOffsetZ = 1.8f;
        break;
    case EnumFacing::WEST:
        renderOffsetX = 1.8f;
        break;
    case EnumFacing::EAST:
        renderOffsetX = -1.8f;
        break;
    default:
        break;
    }
}

void EntityPlayer::wakeUpPlayer(bool immediately, bool updateWorldFlag, bool setSpawn)
{
    setSize(0.6f, 1.8f);
    if (playerLocation) {
        const IBlockState* iblockstate = worldObj->getBlockState(*playerLocation);
        if (iblockstate->getBlock() == Blocks::bed) {
            worldObj->setBlockState(*playerLocation, iblockstate->withProperty(BlockBed::OCCUPIED, false), 4);
            std::optional<BlockPos> blockpos = BlockBed::getSafeExitLocation(worldObj, *playerLocation, 0);
            if (!blockpos)
                blockpos = playerLocation->up();
            setPosition(static_cast<double>(static_cast<float>(blockpos->getX()) + 0.5f), static_cast<double>(static_cast<float>(blockpos->getY()) + 0.1f),
                static_cast<double>(static_cast<float>(blockpos->getZ()) + 0.5f));
        }
    }
    sleeping = false;
    if (!worldObj->isRemote && updateWorldFlag)
        worldObj->updateAllPlayersSleepingFlag();
    sleepTimer = immediately ? 0 : 100;
    if (setSpawn)
        setSpawnPoint(playerLocation, false);
}

bool EntityPlayer::isInBed()
{
    return playerLocation && worldObj->getBlockState(*playerLocation)->getBlock() == Blocks::bed;
}

std::optional<BlockPos> EntityPlayer::getBedSpawnLocation(World* worldIn, const BlockPos& bedLocation, bool forceSpawn)
{
    Block* block = worldIn->getBlockState(bedLocation)->getBlock();
    if (block != Blocks::bed) {
        if (!forceSpawn)
            return std::nullopt;
        const bool flag = block->canSpawnInBlock();
        const bool flag1 = worldIn->getBlockState(bedLocation.up())->getBlock()->canSpawnInBlock();
        return flag && flag1 ? std::optional<BlockPos>(bedLocation) : std::nullopt;
    }
    return BlockBed::getSafeExitLocation(worldIn, bedLocation, 0);
}

float EntityPlayer::getBedOrientationInDegrees()
{
    if (playerLocation) {
        switch (worldObj->getBlockState(*playerLocation)->getValue(BlockDirectional::FACING)) {
        case EnumFacing::SOUTH: return 90.0f;
        case EnumFacing::NORTH: return 270.0f;
        case EnumFacing::WEST: return 0.0f;
        case EnumFacing::EAST: return 180.0f;
        default: break;
        }
    }
    return 0.0f;
}

void EntityPlayer::setSpawnPoint(const std::optional<BlockPos>& pos, bool forced)
{
    if (pos) {
        spawnChunk = pos;
        spawnForced = forced;
    } else {
        spawnChunk.reset();
        spawnForced = false;
    }
}

void EntityPlayer::jump()
{
    EntityLivingBase::jump();
    // Java: triggerAchievement(jumpStat) -- statistics.
    if (isSprinting())
        addExhaustion(0.8f);
    else
        addExhaustion(0.2f);
}

void EntityPlayer::moveEntityWithHeading(float strafe, float forward)
{
    const double d0 = posX;
    const double d1 = posY;
    const double d2 = posZ;
    if (capabilities.isFlying && !ridingEntity) {
        const double d3 = motionY;
        const float f = jumpMovementFactor;
        jumpMovementFactor = capabilities.getFlySpeed() * static_cast<float>(isSprinting() ? 2 : 1);
        EntityLivingBase::moveEntityWithHeading(strafe, forward);
        motionY = d3 * 0.6;
        jumpMovementFactor = f;
    } else {
        EntityLivingBase::moveEntityWithHeading(strafe, forward);
    }
    addMovementStat(posX - d0, posY - d1, posZ - d2);
}

float EntityPlayer::getAIMoveSpeed()
{
    return static_cast<float>(getEntityAttribute(SharedMonsterAttributes::movementSpeed())->getAttributeValue());
}

void EntityPlayer::addMovementStat(double p_71000_1_, double p_71000_3_, double p_71000_5_)
{
    if (ridingEntity)
        return;
    // The statistics come with their stage; the exhaustion is applied as in Java.
    if (isInsideOfMaterial(Material::water)) {
        const int i = java::round(MathHelper::sqrt_double(p_71000_1_ * p_71000_1_ + p_71000_3_ * p_71000_3_ + p_71000_5_ * p_71000_5_) * 100.0f);
        if (i > 0)
            addExhaustion(0.015f * static_cast<float>(i) * 0.01f);
    } else if (isInWater()) {
        const int j = java::round(MathHelper::sqrt_double(p_71000_1_ * p_71000_1_ + p_71000_5_ * p_71000_5_) * 100.0f);
        if (j > 0)
            addExhaustion(0.015f * static_cast<float>(j) * 0.01f);
    } else if (isOnLadder()) {
        // Java: distanceClimbedStat when moving up.
    } else if (onGround) {
        const int k = java::round(MathHelper::sqrt_double(p_71000_1_ * p_71000_1_ + p_71000_5_ * p_71000_5_) * 100.0f);
        if (k > 0) {
            if (isSprinting())
                addExhaustion(0.099999994f * static_cast<float>(k) * 0.01f);
            else
                addExhaustion(0.01f * static_cast<float>(k) * 0.01f);
        }
    }
}

void EntityPlayer::addMountedMovementStat(double, double, double)
{
    // Java: the riding statistics (minecart, boat, pig, horse) and the onARail achievement -- statistics.
}

void EntityPlayer::fall(float distance, float damageMultiplier)
{
    if (!capabilities.allowFlying) {
        // Java: distanceFallenStat for falls of 2 or more -- statistics.
        EntityLivingBase::fall(distance, damageMultiplier);
    }
}

void EntityPlayer::resetHeight()
{
    if (!isSpectator())
        EntityLivingBase::resetHeight();
}

void EntityPlayer::onKillEntity(EntityLivingBase*)
{
    // Java: the killEnemy achievement for IMobs and the egg's kill statistic -- statistics.
}

void EntityPlayer::setInWeb()
{
    if (!capabilities.isFlying)
        Entity::setInWeb();
}

void EntityPlayer::addExperience(int amount)
{
    addScore(amount);
    const int i = INT_MAX - experienceTotal;
    if (amount > i)
        amount = i;
    experience += static_cast<float>(amount) / static_cast<float>(xpBarCap());
    for (experienceTotal += amount; experience >= 1.0f; experience /= static_cast<float>(xpBarCap())) {
        experience = (experience - 1.0f) * static_cast<float>(xpBarCap());
        addExperienceLevel(1);
    }
}

void EntityPlayer::removeExperienceLevel(int levels)
{
    experienceLevel -= levels;
    if (experienceLevel < 0) {
        experienceLevel = 0;
        experience = 0.0f;
        experienceTotal = 0;
    }
    xpSeed = rand.nextInt();
}

void EntityPlayer::addExperienceLevel(int levels)
{
    experienceLevel += levels;
    if (experienceLevel < 0) {
        experienceLevel = 0;
        experience = 0.0f;
        experienceTotal = 0;
    }
    if (levels > 0 && experienceLevel % 5 == 0 && static_cast<float>(lastXPSound) < static_cast<float>(ticksExisted) - 100.0f) {
        const float f = experienceLevel > 30 ? 1.0f : static_cast<float>(experienceLevel) / 30.0f;
        worldObj->playSoundAtEntity(this, "random.levelup", f * 0.75f, 1.0f);
        lastXPSound = ticksExisted;
    }
}

int EntityPlayer::xpBarCap() const
{
    return experienceLevel >= 30 ? 112 + (experienceLevel - 30) * 9 : (experienceLevel >= 15 ? 37 + (experienceLevel - 15) * 5 : 7 + experienceLevel * 2);
}

void EntityPlayer::addExhaustion(float p_71020_1_)
{
    if (!capabilities.disableDamage && !worldObj->isRemote)
        foodStats.addExhaustion(p_71020_1_);
}

void EntityPlayer::setItemInUse(const std::shared_ptr<ItemStack>& stack, int duration)
{
    if (stack != itemInUse) {
        itemInUse = stack;
        itemInUseCount = duration;
        if (!worldObj->isRemote)
            setEating(true);
    }
}

bool EntityPlayer::canPlayerEdit(const BlockPos& p_175151_1_, EnumFacing p_175151_2_, ItemStack* p_175151_3_)
{
    if (capabilities.allowEdit)
        return true;
    if (!p_175151_3_)
        return false;
    const BlockPos blockpos = p_175151_1_.offset(getOpposite(p_175151_2_));
    const Block* block = worldObj->getBlockState(blockpos)->getBlock();
    return p_175151_3_->canPlaceOn(block) || p_175151_3_->canEditBlocks();
}

int EntityPlayer::getExperiencePoints(EntityPlayer*)
{
    if (worldObj->getGameRules().getBoolean("keepInventory"))
        return 0;
    const int i = experienceLevel * 7;
    return i > 100 ? 100 : i;
}

void EntityPlayer::clonePlayer(EntityPlayer* oldPlayer, bool respawnFromEnd)
{
    if (respawnFromEnd) {
        inventory.copyInventory(oldPlayer->inventory);
        setHealth(oldPlayer->getHealth());
        foodStats = oldPlayer->foodStats;
        experienceLevel = oldPlayer->experienceLevel;
        experienceTotal = oldPlayer->experienceTotal;
        experience = oldPlayer->experience;
        setScore(oldPlayer->getScore());
        lastPortalPos = oldPlayer->lastPortalPos;
        lastPortalVec = oldPlayer->lastPortalVec;
        teleportDirection = oldPlayer->teleportDirection;
    } else if (worldObj->getGameRules().getBoolean("keepInventory")) {
        inventory.copyInventory(oldPlayer->inventory);
        experienceLevel = oldPlayer->experienceLevel;
        experienceTotal = oldPlayer->experienceTotal;
        experience = oldPlayer->experience;
        setScore(oldPlayer->getScore());
    }
    xpSeed = oldPlayer->xpSeed;
    theInventoryEnderChest = oldPlayer->theInventoryEnderChest;
    getDataWatcher().updateObject(10, oldPlayer->getDataWatcher().getWatchableObjectByte(10));
}

std::shared_ptr<ItemStack> EntityPlayer::getEquipmentInSlot(int slotIn)
{
    return slotIn == 0 ? inventory.getCurrentItem() : inventory.armorInventory.at(static_cast<size_t>(slotIn - 1));
}

void EntityPlayer::setCurrentItemOrArmor(int slotIn, std::shared_ptr<ItemStack> stack)
{
    inventory.armorInventory.at(static_cast<size_t>(slotIn)) = std::move(stack);
}

bool EntityPlayer::isInvisibleToPlayer(EntityPlayer* player)
{
    if (!isInvisible())
        return false;
    if (player->isSpectator())
        return false;
    // Java: true unless both share a team that sees friendly invisibles -- teams come with the scoreboard.
    return true;
}

std::vector<std::shared_ptr<ItemStack>> EntityPlayer::getInventory()
{
    return std::vector<std::shared_ptr<ItemStack>>(inventory.armorInventory.begin(), inventory.armorInventory.end());
}

std::shared_ptr<IChatComponent> EntityPlayer::getDisplayName()
{
    // Java: ScorePlayerTeam.formatPlayerName(getTeam(), name) -- the name itself without a team.
    auto ichatcomponent = std::make_shared<ChatComponentText>(getName());
    // Java: the click event suggests "/msg <name> " and the hover event shows the entity -- with the chat events.
    ichatcomponent->getChatStyle()->setInsertion(getName());
    return ichatcomponent;
}

float EntityPlayer::getEyeHeight() const
{
    float f = 1.62f;
    if (sleeping)
        f = 0.2f;
    if (isSneaking())
        f -= 0.08f;
    return f;
}

void EntityPlayer::setAbsorptionAmount(float amount)
{
    if (amount < 0.0f)
        amount = 0.0f;
    dataWatcher.updateObject(17, amount);
}

UUID EntityPlayer::getUUID(const GameProfile& profile)
{
    if (profile.getId())
        return *profile.getId();
    return getOfflineUUID(profile.getName());
}

UUID EntityPlayer::getOfflineUUID(const std::string& username)
{
    return UUID::nameUUIDFromBytes("OfflinePlayer:" + username);
}

bool EntityPlayer::canOpen(const std::string& lockCode)
{
    if (lockCode.empty())
        return true;
    const std::shared_ptr<ItemStack> itemstack = getCurrentEquippedItem();
    return itemstack && itemstack->hasDisplayName() ? itemstack->getDisplayName() == lockCode : false;
}

bool EntityPlayer::isWearing(EnumPlayerModelParts p_175148_1_) const
{
    return (dataWatcher.getWatchableObjectByte(10) & getPartMask(p_175148_1_)) == getPartMask(p_175148_1_);
}

bool EntityPlayer::replaceItemInInventory(int inventorySlot, std::shared_ptr<ItemStack> itemStackIn)
{
    if (inventorySlot >= 0 && inventorySlot < static_cast<int>(inventory.mainInventory.size())) {
        inventory.setInventorySlotContents(inventorySlot, std::move(itemStackIn));
        return true;
    }
    const int i = inventorySlot - 100;
    if (i >= 0 && i < static_cast<int>(inventory.armorInventory.size())) {
        const int k = i + 1;
        if (itemStackIn && itemStackIn->getItem()) {
            if (dynamic_cast<ItemArmor*>(itemStackIn->getItem())) {
                if (getArmorPosition(*itemStackIn) != k)
                    return false;
            } else if (k != 4 || (itemStackIn->getItem() != Items::skull && !dynamic_cast<ItemBlock*>(itemStackIn->getItem()))) {
                return false;
            }
        }
        inventory.setInventorySlotContents(i + static_cast<int>(inventory.mainInventory.size()), std::move(itemStackIn));
        return true;
    }
    const int j = inventorySlot - 200;
    if (j >= 0 && j < theInventoryEnderChest->getSizeInventory()) {
        theInventoryEnderChest->setInventorySlotContents(j, std::move(itemStackIn));
        return true;
    }
    return false;
}

} // namespace mc
