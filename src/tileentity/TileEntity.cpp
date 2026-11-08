#include "tileentity/TileEntity.h"

#include "block/AllBlocks.h"
#include "core/Log.h"
#include "init/Blocks.h"
#include "init/Items.h"
#include "item/ItemBlock.h"
#include "item/ItemStack.h"
#include "item/ItemTool.h"
#include "nbt/NBTTagCompound.h"
#include "network/play/ServerPlayPackets.h"
#include "tileentity/TileEntities.h"
#include "tileentity/TileEntityBanner.h"
#include "tileentity/TileEntityBeacon.h"
#include "tileentity/TileEntityBrewingStand.h"
#include "tileentity/TileEntityHopper.h"
#include "tileentity/TileEntityJukebox.h"
#include "tileentity/TileEntityPiston.h"
#include "tileentity/TileEntitySign.h"
#include "tileentity/TileEntitySkull.h"
#include "util/MathHelper.h"
#include "world/World.h"

#include <map>

namespace mc {

namespace {

using Factory = std::shared_ptr<TileEntity> (*)(const std::string& id);

std::map<std::string, Factory>& nameToClassMap()
{
    static std::map<std::string, Factory> map;
    return map;
}

template <class T>
std::shared_ptr<TileEntity> make(const std::string&)
{
    return std::make_shared<T>();
}

std::shared_ptr<TileEntity> makeUnported(const std::string& id)
{
    return std::make_shared<TileEntityUnported>(id);
}

void addMapping(Factory factory, const std::string& id)
{
    if (!nameToClassMap().emplace(id, factory).second)
        throw std::invalid_argument("Duplicate id: " + id);
}

} // namespace

void TileEntity::registerTileEntities()
{
    if (!nameToClassMap().empty())
        return;
    addMapping(make<TileEntityFurnace>, "Furnace");
    addMapping(make<TileEntityChest>, "Chest");
    addMapping(make<TileEntityEnderChest>, "EnderChest");
    addMapping(make<TileEntityJukebox>, "RecordPlayer");
    addMapping(make<TileEntityDispenser>, "Trap");
    addMapping(make<TileEntityDropper>, "Dropper");
    addMapping(make<TileEntitySign>, "Sign");
    addMapping(make<TileEntityMobSpawner>, "MobSpawner");
    addMapping(make<TileEntityNote>, "Music");
    addMapping(make<TileEntityPiston>, "Piston");
    addMapping(make<TileEntityBrewingStand>, "Cauldron");
    addMapping(make<TileEntityEnchantmentTable>, "EnchantTable");
    addMapping(make<TileEntityEndPortal>, "Airportal");
    addMapping(makeUnported, "Control");
    addMapping(make<TileEntityBeacon>, "Beacon");
    addMapping(make<TileEntitySkull>, "Skull");
    addMapping(make<TileEntityDaylightDetector>, "DLDetector");
    addMapping(make<TileEntityHopper>, "Hopper");
    addMapping(make<TileEntityComparator>, "Comparator");
    addMapping(make<TileEntityFlowerPot>, "FlowerPot");
    addMapping(make<TileEntityBanner>, "Banner");
}

std::shared_ptr<TileEntity> TileEntity::createAndLoadEntity(const NBTTagCompound& nbt)
{
    const std::string id = nbt.getString("id");
    const auto it = nameToClassMap().find(id);
    if (it == nameToClassMap().end()) {
        Log::warn("Skipping BlockEntity with id {}", id);
        return nullptr;
    }
    std::shared_ptr<TileEntity> tileentity = it->second(id);
    tileentity->readFromNBT(nbt);
    return tileentity;
}

void TileEntity::readFromNBT(const NBTTagCompound& compound)
{
    pos = BlockPos(compound.getInteger("x"), compound.getInteger("y"), compound.getInteger("z"));
}

void TileEntity::writeToNBT(NBTTagCompound& compound) const
{
    compound.setString("id", std::string(getMappingName()));
    compound.setInteger("x", pos.getX());
    compound.setInteger("y", pos.getY());
    compound.setInteger("z", pos.getZ());
}

int TileEntity::getBlockMetadata()
{
    if (blockMetadata == -1) {
        const IBlockState* iblockstate = worldObj->getBlockState(pos);
        blockMetadata = iblockstate->getBlock()->getMetaFromState(iblockstate);
    }
    return blockMetadata;
}

void TileEntity::markDirty()
{
    if (worldObj) {
        const IBlockState* iblockstate = worldObj->getBlockState(pos);
        blockMetadata = iblockstate->getBlock()->getMetaFromState(iblockstate);
        worldObj->markChunkDirty(pos, this);
        if (getBlockType() != Blocks::air)
            worldObj->updateComparatorOutputLevel(pos, getBlockType());
    }
}

double TileEntity::getDistanceSq(double x, double y, double z) const
{
    const double d0 = static_cast<double>(pos.getX()) + 0.5 - x;
    const double d1 = static_cast<double>(pos.getY()) + 0.5 - y;
    const double d2 = static_cast<double>(pos.getZ()) + 0.5 - z;
    return d0 * d0 + d1 * d1 + d2 * d2;
}

Block* TileEntity::getBlockType()
{
    if (!blockType)
        blockType = worldObj->getBlockState(pos)->getBlock();
    return blockType;
}

bool TileEntity::receiveClientEvent(int, int)
{
    return false;
}

void TileEntity::updateContainingBlockInfo()
{
    blockType = nullptr;
    blockMetadata = -1;
}

// --- TileEntityUnported ---

void TileEntityUnported::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    data = std::static_pointer_cast<NBTTagCompound>(compound.copy());
}

void TileEntityUnported::writeToNBT(NBTTagCompound& compound) const
{
    if (data)
        compound.merge(*data);
    TileEntity::writeToNBT(compound);
}

// --- TileEntityLockable ---

void TileEntityLockable::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    code = compound.hasKey("Lock", 8) ? compound.getString("Lock") : std::string();
}

void TileEntityLockable::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    compound.setString("Lock", code);
}

// --- InventorySlots ---

template <size_t N>
std::shared_ptr<ItemStack> InventorySlots<N>::decrStackSize(int index, int count, bool& changed)
{
    std::shared_ptr<ItemStack>& slot = at(index);
    changed = false;
    if (!slot)
        return nullptr;
    changed = true;
    if (slot->stackSize <= count) {
        std::shared_ptr<ItemStack> itemstack1 = std::move(slot);
        slot = nullptr;
        return itemstack1;
    }
    auto itemstack = std::make_shared<ItemStack>(slot->splitStack(count));
    if (slot->stackSize == 0)
        slot = nullptr;
    return itemstack;
}

template <size_t N>
std::shared_ptr<ItemStack> InventorySlots<N>::removeStackFromSlot(int index)
{
    std::shared_ptr<ItemStack>& slot = at(index);
    std::shared_ptr<ItemStack> itemstack = std::move(slot);
    slot = nullptr;
    return itemstack;
}

template <size_t N>
void InventorySlots<N>::readItems(const NBTTagCompound& compound, bool maskSlot)
{
    const auto nbttaglist = compound.getTagList("Items", 10);
    clear();
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        const auto nbttagcompound = nbttaglist->getCompoundTagAt(i);
        const int j = maskSlot ? nbttagcompound->getByte("Slot") & 255 : nbttagcompound->getByte("Slot");
        if (j >= 0 && j < static_cast<int>(N)) {
            auto loaded = ItemStack::loadItemStackFromNBT(*nbttagcompound);
            stacks[static_cast<size_t>(j)] = loaded ? std::shared_ptr<ItemStack>(std::move(loaded)) : nullptr;
        }
    }
}

template <size_t N>
void InventorySlots<N>::writeItems(NBTTagCompound& compound) const
{
    auto nbttaglist = std::make_shared<NBTTagList>();
    for (size_t i = 0; i < N; ++i) {
        if (stacks[i]) {
            auto nbttagcompound = std::make_shared<NBTTagCompound>();
            nbttagcompound->setByte("Slot", static_cast<int8_t>(i));
            stacks[i]->writeToNBT(*nbttagcompound);
            nbttaglist->appendTag(nbttagcompound);
        }
    }
    compound.setTag("Items", nbttaglist);
}

template class InventorySlots<27>;
template class InventorySlots<9>;
template class InventorySlots<3>;

// --- TileEntityChest ---

std::shared_ptr<ItemStack> TileEntityChest::decrStackSize(int index, int count)
{
    bool changed = false;
    auto stack = chestContents.decrStackSize(index, count, changed);
    if (changed)
        markDirty();
    return stack;
}

void TileEntityChest::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (stack && stack->stackSize > getInventoryStackLimit())
        stack->stackSize = getInventoryStackLimit();
    chestContents.at(index) = std::move(stack);
    markDirty();
}

void TileEntityChest::readFromNBT(const NBTTagCompound& compound)
{
    TileEntityLockable::readFromNBT(compound);
    if (compound.hasKey("CustomName", 8))
        customName = compound.getString("CustomName");
    chestContents.readItems(compound, true);
}

void TileEntityChest::writeToNBT(NBTTagCompound& compound) const
{
    TileEntityLockable::writeToNBT(compound);
    chestContents.writeItems(compound);
    if (hasCustomName())
        compound.setString("CustomName", customName);
}

bool TileEntityChest::isUseableByPlayer(EntityPlayer*)
{
    // Java: and the player within 8 blocks (stage 10).
    return worldObj->getTileEntity(pos) == this;
}

void TileEntityChest::updateContainingBlockInfo()
{
    TileEntity::updateContainingBlockInfo();
    adjacentChestChecked = false;
}

void TileEntityChest::func_174910_a(TileEntityChest* chestTe, EnumFacing side)
{
    if (chestTe->isInvalid()) {
        adjacentChestChecked = false;
    } else if (adjacentChestChecked) {
        switch (side) {
        case EnumFacing::NORTH:
            if (adjacentChestZNeg != chestTe)
                adjacentChestChecked = false;
            break;
        case EnumFacing::SOUTH:
            if (adjacentChestZPos != chestTe)
                adjacentChestChecked = false;
            break;
        case EnumFacing::EAST:
            if (adjacentChestXPos != chestTe)
                adjacentChestChecked = false;
            break;
        case EnumFacing::WEST:
            if (adjacentChestXNeg != chestTe)
                adjacentChestChecked = false;
            break;
        default:
            break;
        }
    }
}

void TileEntityChest::checkForAdjacentChests()
{
    if (!adjacentChestChecked) {
        adjacentChestChecked = true;
        adjacentChestXNeg = getAdjacentChest(EnumFacing::WEST);
        adjacentChestXPos = getAdjacentChest(EnumFacing::EAST);
        adjacentChestZNeg = getAdjacentChest(EnumFacing::NORTH);
        adjacentChestZPos = getAdjacentChest(EnumFacing::SOUTH);
    }
}

TileEntityChest* TileEntityChest::getAdjacentChest(EnumFacing side)
{
    const BlockPos blockpos = pos.offset(side);
    if (isChestAt(blockpos)) {
        if (auto* tileentitychest = dynamic_cast<TileEntityChest*>(worldObj->getTileEntity(blockpos))) {
            tileentitychest->func_174910_a(this, getOpposite(side));
            return tileentitychest;
        }
    }
    return nullptr;
}

bool TileEntityChest::isChestAt(const BlockPos& posIn)
{
    if (!worldObj)
        return false;
    const auto* block = dynamic_cast<const BlockChest*>(worldObj->getBlockState(posIn)->getBlock());
    return block && block->chestType == getChestType();
}

void TileEntityChest::update()
{
    checkForAdjacentChests();
    const int i = pos.getX();
    const int j = pos.getY();
    const int k = pos.getZ();
    ++ticksSinceSync;
    // Java: every 200 ticks the server recounts the players viewing the chest -- stage 10.
    (void)i;
    (void)j;
    (void)k;
    prevLidAngle = lidAngle;
    const float f1 = 0.1f;
    if (numPlayersUsing > 0 && lidAngle == 0.0f && !adjacentChestZNeg && !adjacentChestXNeg) {
        double d1 = static_cast<double>(i) + 0.5;
        double d2 = static_cast<double>(k) + 0.5;
        if (adjacentChestZPos)
            d2 += 0.5;
        if (adjacentChestXPos)
            d1 += 0.5;
        worldObj->playSoundEffect(d1, static_cast<double>(j) + 0.5, d2, "random.chestopen", 0.5f, worldObj->rand.nextFloat() * 0.1f + 0.9f);
    }
    if ((numPlayersUsing == 0 && lidAngle > 0.0f) || (numPlayersUsing > 0 && lidAngle < 1.0f)) {
        const float f2 = lidAngle;
        if (numPlayersUsing > 0)
            lidAngle += f1;
        else
            lidAngle -= f1;
        if (lidAngle > 1.0f)
            lidAngle = 1.0f;
        const float f3 = 0.5f;
        if (lidAngle < f3 && f2 >= f3 && !adjacentChestZNeg && !adjacentChestXNeg) {
            double d3 = static_cast<double>(i) + 0.5;
            double d0 = static_cast<double>(k) + 0.5;
            if (adjacentChestZPos)
                d0 += 0.5;
            if (adjacentChestXPos)
                d3 += 0.5;
            worldObj->playSoundEffect(d3, static_cast<double>(j) + 0.5, d0, "random.chestclosed", 0.5f, worldObj->rand.nextFloat() * 0.1f + 0.9f);
        }
        if (lidAngle < 0.0f)
            lidAngle = 0.0f;
    }
}

bool TileEntityChest::receiveClientEvent(int id, int type)
{
    if (id == 1) {
        numPlayersUsing = type;
        return true;
    }
    return TileEntity::receiveClientEvent(id, type);
}

void TileEntityChest::openInventory(EntityPlayer*)
{
    // Java: skips spectators (stage 10).
    if (numPlayersUsing < 0)
        numPlayersUsing = 0;
    ++numPlayersUsing;
    worldObj->addBlockEvent(pos, getBlockType(), 1, numPlayersUsing);
    worldObj->notifyNeighborsOfStateChange(pos, getBlockType());
    worldObj->notifyNeighborsOfStateChange(pos.down(), getBlockType());
}

void TileEntityChest::closeInventory(EntityPlayer*)
{
    if (dynamic_cast<BlockChest*>(getBlockType())) {
        --numPlayersUsing;
        worldObj->addBlockEvent(pos, getBlockType(), 1, numPlayersUsing);
        worldObj->notifyNeighborsOfStateChange(pos, getBlockType());
        worldObj->notifyNeighborsOfStateChange(pos.down(), getBlockType());
    }
}

void TileEntityChest::invalidate()
{
    TileEntity::invalidate();
    updateContainingBlockInfo();
    checkForAdjacentChests();
}

int TileEntityChest::getChestType()
{
    if (cachedChestType == -1) {
        if (!worldObj)
            return 0;
        const auto* chest = dynamic_cast<const BlockChest*>(getBlockType());
        if (!chest)
            return 0;
        cachedChestType = chest->chestType;
    }
    return cachedChestType;
}

// --- TileEntityDispenser ---

namespace {

Random& dispenserRNG()
{
    static Random rng;
    return rng;
}

} // namespace

std::shared_ptr<ItemStack> TileEntityDispenser::decrStackSize(int index, int count)
{
    bool changed = false;
    auto stack = stacks.decrStackSize(index, count, changed);
    if (changed)
        markDirty();
    return stack;
}

int TileEntityDispenser::getDispenseSlot()
{
    int i = -1;
    int j = 1;
    for (int k = 0; k < stacks.size(); ++k) {
        if (stacks.get(k) && dispenserRNG().nextInt(j++) == 0)
            i = k;
    }
    return i;
}

void TileEntityDispenser::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    if (stack && stack->stackSize > getInventoryStackLimit())
        stack->stackSize = getInventoryStackLimit();
    stacks.at(index) = std::move(stack);
    markDirty();
}

int TileEntityDispenser::addItemStack(std::shared_ptr<ItemStack> stack)
{
    for (int i = 0; i < stacks.size(); ++i) {
        if (!stacks.get(i) || !stacks.get(i)->getItem()) {
            setInventorySlotContents(i, std::move(stack));
            return i;
        }
    }
    return -1;
}

void TileEntityDispenser::readFromNBT(const NBTTagCompound& compound)
{
    TileEntityLockable::readFromNBT(compound);
    stacks.readItems(compound, true);
    if (compound.hasKey("CustomName", 8))
        customName = compound.getString("CustomName");
}

void TileEntityDispenser::writeToNBT(NBTTagCompound& compound) const
{
    TileEntityLockable::writeToNBT(compound);
    stacks.writeItems(compound);
    if (hasCustomName())
        compound.setString("CustomName", *customName);
}

bool TileEntityDispenser::isUseableByPlayer(EntityPlayer*)
{
    return worldObj->getTileEntity(pos) == this;
}

// --- TileEntityFurnace ---

std::shared_ptr<ItemStack> TileEntityFurnace::decrStackSize(int index, int count)
{
    bool changed = false;
    return furnaceItemStacks.decrStackSize(index, count, changed);
}

void TileEntityFurnace::setInventorySlotContents(int index, std::shared_ptr<ItemStack> stack)
{
    const ItemStack* current = furnaceItemStacks.get(index).get();
    const bool flag = stack && stack->isItemEqual(current) && ItemStack::areItemStackTagsEqual(stack.get(), current);
    if (stack && stack->stackSize > getInventoryStackLimit())
        stack->stackSize = getInventoryStackLimit();
    const bool hasStack = stack != nullptr;
    furnaceItemStacks.at(index) = std::move(stack);
    if (index == 0 && !flag) {
        // Java: getCookTime(stack), always 200.
        (void)hasStack;
        totalCookTime = 200;
        cookTime = 0;
        markDirty();
    }
}

void TileEntityFurnace::readFromNBT(const NBTTagCompound& compound)
{
    TileEntityLockable::readFromNBT(compound);
    furnaceItemStacks.readItems(compound, false);
    furnaceBurnTime = compound.getShort("BurnTime");
    cookTime = compound.getShort("CookTime");
    totalCookTime = compound.getShort("CookTimeTotal");
    currentItemBurnTime = getItemBurnTime(furnaceItemStacks.get(1).get());
    if (compound.hasKey("CustomName", 8))
        furnaceCustomName = compound.getString("CustomName");
}

void TileEntityFurnace::writeToNBT(NBTTagCompound& compound) const
{
    TileEntityLockable::writeToNBT(compound);
    compound.setShort("BurnTime", static_cast<int16_t>(furnaceBurnTime));
    compound.setShort("CookTime", static_cast<int16_t>(cookTime));
    compound.setShort("CookTimeTotal", static_cast<int16_t>(totalCookTime));
    furnaceItemStacks.writeItems(compound);
    if (hasCustomName())
        compound.setString("CustomName", furnaceCustomName);
}

int TileEntityFurnace::getItemBurnTime(const ItemStack* stack)
{
    if (!stack)
        return 0;
    Item* item = stack->getItem();
    if (const auto* itemBlock = dynamic_cast<const ItemBlock*>(item); itemBlock && itemBlock->getBlock() != Blocks::air) {
        const Block* block = itemBlock->getBlock();
        if (block == Blocks::wooden_slab)
            return 150;
        if (&block->getMaterial() == &Material::wood)
            return 300;
        if (block == Blocks::coal_block)
            return 16000;
    }
    if (const auto* tool = dynamic_cast<const ItemTool*>(item); tool && tool->getToolMaterialName() == "WOOD")
        return 200;
    if (const auto* sword = dynamic_cast<const ItemSword*>(item); sword && sword->getToolMaterialName() == "WOOD")
        return 200;
    if (const auto* hoe = dynamic_cast<const ItemHoe*>(item); hoe && hoe->getMaterialName() == "WOOD")
        return 200;
    if (item == Items::stick)
        return 100;
    if (item == Items::coal)
        return 1600;
    if (item == Items::lava_bucket)
        return 20000;
    if (item == Item::getItemFromBlock(Blocks::sapling))
        return 100;
    if (item == Items::blaze_rod)
        return 2400;
    return 0;
}

bool TileEntityFurnace::isUseableByPlayer(EntityPlayer*)
{
    return worldObj->getTileEntity(pos) == this;
}

bool TileEntityFurnace::isItemValidForSlot(int index, const ItemStack& stack) const
{
    return index == 2 ? false : (index != 1 ? true : isItemFuel(&stack) || (stack.getItem() && stack.getItem() == Items::bucket));
}

int TileEntityFurnace::getField(int id) const
{
    switch (id) {
    case 0: return furnaceBurnTime;
    case 1: return currentItemBurnTime;
    case 2: return cookTime;
    case 3: return totalCookTime;
    default: return 0;
    }
}

void TileEntityFurnace::setField(int id, int value)
{
    switch (id) {
    case 0: furnaceBurnTime = value; break;
    case 1: currentItemBurnTime = value; break;
    case 2: cookTime = value; break;
    case 3: totalCookTime = value; break;
    default: break;
    }
}

// --- MobSpawnerBaseLogic ---

MobSpawnerBaseLogic::WeightedRandomMinecart::WeightedRandomMinecart(const NBTTagCompound& tagCompound)
    : WeightedRandomMinecart(tagCompound.getCompoundTag("Properties"), tagCompound.getString("Type"), tagCompound.getInteger("Weight"))
{
}

MobSpawnerBaseLogic::WeightedRandomMinecart::WeightedRandomMinecart(std::shared_ptr<NBTTagCompound> tagCompound, std::string type, int weight)
    : itemWeight(weight)
    , nbtData(std::move(tagCompound))
    , entityType(std::move(type))
{
    if (entityType == "Minecart") {
        if (nbtData) {
            // Java: EntityMinecart.EnumMinecartType.byNetworkID(Type).getName()
            static const char* const names[7] { "MinecartRideable", "MinecartChest", "MinecartFurnace", "MinecartTNT", "MinecartSpawner",
                "MinecartHopper", "MinecartCommandBlock" };
            const int id = nbtData->getInteger("Type");
            entityType = names[id >= 0 && id < 7 ? id : 0];
        } else {
            entityType = "MinecartRideable";
        }
    }
}

std::shared_ptr<NBTTagCompound> MobSpawnerBaseLogic::WeightedRandomMinecart::toNBT() const
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    nbttagcompound->setTag("Properties", nbtData);
    nbttagcompound->setString("Type", entityType);
    nbttagcompound->setInteger("Weight", itemWeight);
    return nbttagcompound;
}

World* MobSpawnerBaseLogic::getSpawnerWorld() const
{
    return owner.worldObj;
}

BlockPos MobSpawnerBaseLogic::getSpawnerPosition() const
{
    return owner.pos;
}

std::string MobSpawnerBaseLogic::getEntityNameToSpawn() const
{
    if (!randomEntity) {
        if (mobID == "Minecart")
            const_cast<MobSpawnerBaseLogic*>(this)->mobID = "MinecartRideable";
        return mobID;
    }
    return randomEntity->entityType;
}

void MobSpawnerBaseLogic::updateSpawner()
{
    // Java: when a player is within activatingRangeFromPlayer, the client spins the mob and makes particles and
    // the server spawns entities -- entities and players come with stage 10.
}

void MobSpawnerBaseLogic::readFromNBT(const NBTTagCompound& nbt)
{
    mobID = nbt.getString("EntityId");
    spawnDelay = nbt.getShort("Delay");
    minecartToSpawn.clear();
    if (nbt.hasKey("SpawnPotentials", 9)) {
        const auto nbttaglist = nbt.getTagList("SpawnPotentials", 10);
        for (int i = 0; i < nbttaglist->tagCount(); ++i)
            minecartToSpawn.emplace_back(*nbttaglist->getCompoundTagAt(i));
    }
    if (nbt.hasKey("SpawnData", 10))
        setRandomEntity(WeightedRandomMinecart(nbt.getCompoundTag("SpawnData"), mobID));
    else
        setRandomEntity(std::nullopt);
    if (nbt.hasKey("MinSpawnDelay", 99)) {
        minSpawnDelay = nbt.getShort("MinSpawnDelay");
        maxSpawnDelay = nbt.getShort("MaxSpawnDelay");
        spawnCount = nbt.getShort("SpawnCount");
    }
    if (nbt.hasKey("MaxNearbyEntities", 99)) {
        maxNearbyEntities = nbt.getShort("MaxNearbyEntities");
        activatingRangeFromPlayer = nbt.getShort("RequiredPlayerRange");
    }
    if (nbt.hasKey("SpawnRange", 99))
        spawnRange = nbt.getShort("SpawnRange");
}

void MobSpawnerBaseLogic::writeToNBT(NBTTagCompound& nbt) const
{
    const std::string s = getEntityNameToSpawn();
    if (!s.empty()) {
        nbt.setString("EntityId", s);
        nbt.setShort("Delay", static_cast<int16_t>(spawnDelay));
        nbt.setShort("MinSpawnDelay", static_cast<int16_t>(minSpawnDelay));
        nbt.setShort("MaxSpawnDelay", static_cast<int16_t>(maxSpawnDelay));
        nbt.setShort("SpawnCount", static_cast<int16_t>(spawnCount));
        nbt.setShort("MaxNearbyEntities", static_cast<int16_t>(maxNearbyEntities));
        nbt.setShort("RequiredPlayerRange", static_cast<int16_t>(activatingRangeFromPlayer));
        nbt.setShort("SpawnRange", static_cast<int16_t>(spawnRange));
        if (randomEntity)
            nbt.setTag("SpawnData", randomEntity->nbtData->copy());
        if (randomEntity || !minecartToSpawn.empty()) {
            auto nbttaglist = std::make_shared<NBTTagList>();
            if (!minecartToSpawn.empty()) {
                for (const WeightedRandomMinecart& minecart : minecartToSpawn)
                    nbttaglist->appendTag(minecart.toNBT());
            } else {
                nbttaglist->appendTag(randomEntity->toNBT());
            }
            nbt.setTag("SpawnPotentials", nbttaglist);
        }
    }
}

bool MobSpawnerBaseLogic::setDelayToMin(int delay)
{
    if (delay == 1 && getSpawnerWorld()->isRemote) {
        spawnDelay = minSpawnDelay;
        return true;
    }
    return false;
}

void MobSpawnerBaseLogic::setRandomEntity(std::optional<WeightedRandomMinecart> entity)
{
    randomEntity = std::move(entity);
    if (getSpawnerWorld())
        getSpawnerWorld()->markBlockForUpdate(owner.pos);
}

// --- TileEntityMobSpawner ---

void TileEntityMobSpawner::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    spawnerLogic.readFromNBT(compound);
}

void TileEntityMobSpawner::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    spawnerLogic.writeToNBT(compound);
}

bool TileEntityMobSpawner::receiveClientEvent(int id, int type)
{
    return spawnerLogic.setDelayToMin(id) ? true : TileEntity::receiveClientEvent(id, type);
}

// --- TileEntityFlowerPot ---

void TileEntityFlowerPot::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    const auto resourcelocation = flowerPotItem ? Item::getNameForItem(flowerPotItem) : std::nullopt;
    compound.setString("Item", resourcelocation ? resourcelocation->toString() : "");
    compound.setInteger("Data", flowerPotData);
}

void TileEntityFlowerPot::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    if (compound.hasKey("Item", 8))
        flowerPotItem = Item::getByNameOrId(compound.getString("Item"));
    else
        flowerPotItem = Item::getItemById(compound.getInteger("Item"));
    flowerPotData = compound.getInteger("Data");
}

// --- the small ones ---

void TileEntityEnchantmentTable::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    if (compound.hasKey("CustomName", 8))
        customName = compound.getString("CustomName");
}

void TileEntityEnchantmentTable::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    if (customName && !customName->empty())
        compound.setString("CustomName", *customName);
}

void TileEntityNote::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    note = compound.getByte("note");
    note = static_cast<int8_t>(MathHelper::clamp_int(note, 0, 24));
}

void TileEntityNote::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    compound.setByte("note", note);
}

void TileEntityComparator::readFromNBT(const NBTTagCompound& compound)
{
    TileEntity::readFromNBT(compound);
    outputSignal = compound.getInteger("OutputSignal");
}

void TileEntityComparator::writeToNBT(NBTTagCompound& compound) const
{
    TileEntity::writeToNBT(compound);
    compound.setInteger("OutputSignal", outputSignal);
}

std::shared_ptr<Packet> TileEntityMobSpawner::getDescriptionPacket()
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    writeToNBT(*nbttagcompound);
    nbttagcompound->removeTag("SpawnPotentials");
    return std::make_shared<S35PacketUpdateTileEntity>(pos, 1, nbttagcompound);
}

std::shared_ptr<Packet> TileEntityFlowerPot::getDescriptionPacket()
{
    auto nbttagcompound = std::make_shared<NBTTagCompound>();
    writeToNBT(*nbttagcompound);
    nbttagcompound->removeTag("Item");
    nbttagcompound->setInteger("Item", Item::getIdFromItem(flowerPotItem));
    return std::make_shared<S35PacketUpdateTileEntity>(pos, 5, nbttagcompound);
}

} // namespace mc
