#include "item/ItemMisc.h"

#include "client/resources/Locale.h"
#include "core/JavaString.h"
#include "creativetab/CreativeTabs.h"
#include "entity/EntityList.h"
#include "item/EnumDyeColor.h"
#include "item/ItemStack.h"
#include "nbt/NBTTagCompound.h"

#include <unordered_map>

namespace mc {

ItemBow::ItemBow()
{
    maxStackSize = 1;
    setMaxDamage(384);
    setCreativeTab(&CreativeTabs::tabCombat);
}

ItemFishingRod::ItemFishingRod()
{
    setMaxDamage(64);
    setMaxStackSize(1);
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemFlintAndSteel::ItemFlintAndSteel()
{
    maxStackSize = 1;
    setMaxDamage(64);
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemCarrotOnAStick::ItemCarrotOnAStick()
{
    setCreativeTab(&CreativeTabs::tabTransport);
    setMaxStackSize(1);
    setMaxDamage(25);
}

ItemDoor::ItemDoor(Block* blockIn)
    : block(blockIn)
{
    setCreativeTab(&CreativeTabs::tabRedstone);
}

ItemMinecart::ItemMinecart(EnumMinecartType type)
    : minecartType(type)
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabTransport);
}

ItemBucket::ItemBucket(Block* containedBlock)
    : isFull(containedBlock)
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemBucketMilk::ItemBucketMilk()
{
    setMaxStackSize(1);
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemHangingEntity::ItemHangingEntity(HangingType type)
    : hangingEntityClass(type)
{
    setCreativeTab(&CreativeTabs::tabDecorations);
}

ItemSign::ItemSign()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabDecorations);
}

ItemBed::ItemBed()
{
    setCreativeTab(&CreativeTabs::tabDecorations);
}

ItemSaddle::ItemSaddle()
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabTransport);
}

ItemRedstone::ItemRedstone()
{
    setCreativeTab(&CreativeTabs::tabRedstone);
}

ItemSnowball::ItemSnowball()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemBoat::ItemBoat()
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabTransport);
}

bool ItemBook::isItemTool(const ItemStack& stack) const
{
    return stack.stackSize == 1;
}

ItemEgg::ItemEgg()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabMaterials);
}

ItemEmptyMap::ItemEmptyMap()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemEnderPearl::ItemEnderPearl()
{
    maxStackSize = 16;
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemEnderEye::ItemEnderEye()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemGlassBottle::ItemGlassBottle()
{
    setCreativeTab(&CreativeTabs::tabBrewing);
}

ItemFireball::ItemFireball()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

ItemLead::ItemLead()
{
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemNameTag::ItemNameTag()
{
    setCreativeTab(&CreativeTabs::tabTools);
}

ItemArmorStand::ItemArmorStand()
{
    setCreativeTab(&CreativeTabs::tabDecorations);
}

// --- ItemMonsterPlacer ---

ItemMonsterPlacer::ItemMonsterPlacer()
{
    setHasSubtypes(true);
    setCreativeTab(&CreativeTabs::tabMisc);
}

std::string ItemMonsterPlacer::getItemStackDisplayName(const ItemStack& stack) const
{
    std::string s = java::trim(StatCollector::translateToLocal(getUnlocalizedName() + ".name"));
    if (const auto s1 = EntityList::getStringFromID(stack.getMetadata()))
        s += " " + StatCollector::translateToLocal("entity." + *s1 + ".name");
    return s;
}

int ItemMonsterPlacer::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    const EntityList::EntityEggInfo* egg = EntityList::getEggInfo(stack.getMetadata());
    return egg ? (renderPass == 0 ? egg->primaryColor : egg->secondaryColor) : 16777215;
}

void ItemMonsterPlacer::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (const EntityList::EntityEggInfo& egg : EntityList::entityEggs())
        subItems.emplace_back(item, 1, egg.spawnedID);
}

// --- ItemSkull ---

ItemSkull::ItemSkull()
{
    setCreativeTab(&CreativeTabs::tabDecorations);
    setMaxDamage(0);
    setHasSubtypes(true);
}

void ItemSkull::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (int i = 0; i < 5; ++i)
        subItems.emplace_back(item, 1, i);
}

std::string ItemSkull::getUnlocalizedName(const ItemStack& stack) const
{
    int i = stack.getMetadata();
    if (i < 0 || i >= 5)
        i = 0;
    return Item::getUnlocalizedName() + "." + skullTypes[i];
}

std::string ItemSkull::getItemStackDisplayName(const ItemStack& stack) const
{
    if (stack.getMetadata() == 3 && stack.hasTagCompound()) {
        const auto& tag = stack.getTagCompound();
        if (tag->hasKey("SkullOwner", 8))
            return StatCollector::translateToLocalFormatted("item.skull.player.name", tag->getString("SkullOwner"));
        if (tag->hasKey("SkullOwner", 10)) {
            const auto nbttagcompound = tag->getCompoundTag("SkullOwner");
            if (nbttagcompound->hasKey("Name", 8))
                return StatCollector::translateToLocalFormatted("item.skull.player.name", nbttagcompound->getString("Name"));
        }
    }
    return Item::getItemStackDisplayName(stack);
}

bool ItemSkull::updateItemStackNBT(NBTTagCompound& nbt) const
{
    if (nbt.hasKey("SkullOwner", 8) && !nbt.getString("SkullOwner").empty()) {
        // TileEntitySkull.updateGameprofile without a running server returns the profile unchanged (no id, the
        // name only), and NBTUtil.writeGameProfile then writes just the name. With the integrated server
        // (stage 9) the profile cache fills in id and textures.
        auto profile = std::make_shared<NBTTagCompound>();
        profile->setString("Name", nbt.getString("SkullOwner"));
        nbt.setTag("SkullOwner", profile);
        return true;
    }
    return false;
}

// --- ItemFireworkCharge ---

int ItemFireworkCharge::getColorFromItemStack(const ItemStack& stack, int renderPass) const
{
    if (renderPass != 1)
        return Item::getColorFromItemStack(stack, renderPass);
    const auto nbtbase = std::dynamic_pointer_cast<NBTTagIntArray>(getExplosionTag(stack, "Colors"));
    if (!nbtbase)
        return 9079434;
    const std::vector<int32_t>& aint = nbtbase->getIntArray();
    if (aint.size() == 1)
        return aint[0];
    // Java divides by the length -- an empty array throws ArithmeticException there.
    int i = 0;
    int j = 0;
    int k = 0;
    for (const int l : aint) {
        i += (l & 16711680) >> 16;
        j += (l & 65280) >> 8;
        k += (l & 255) >> 0;
    }
    const int n = static_cast<int>(aint.size());
    i = i / n;
    j = j / n;
    k = k / n;
    return i << 16 | j << 8 | k;
}

std::shared_ptr<NBTBase> ItemFireworkCharge::getExplosionTag(const ItemStack& stack, const std::string& key)
{
    if (stack.hasTagCompound())
        return stack.getTagCompound()->getCompoundTag("Explosion")->getTag(key);
    return nullptr;
}

// --- ItemEnchantedBook ---

EnumRarity ItemEnchantedBook::getRarity(const ItemStack& stack) const
{
    return getEnchantments(stack)->tagCount() > 0 ? EnumRarity::UNCOMMON : Item::getRarity(stack);
}

std::shared_ptr<NBTTagList> ItemEnchantedBook::getEnchantments(const ItemStack& stack) const
{
    const auto& nbttagcompound = stack.getTagCompound();
    if (nbttagcompound && nbttagcompound->hasKey("StoredEnchantments", 9))
        return std::static_pointer_cast<NBTTagList>(nbttagcompound->getTag("StoredEnchantments"));
    return std::make_shared<NBTTagList>();
}

// --- ItemDye ---

ItemDye::ItemDye()
{
    setHasSubtypes(true);
    setMaxDamage(0);
    setCreativeTab(&CreativeTabs::tabMaterials);
}

std::string ItemDye::getUnlocalizedName(const ItemStack& stack) const
{
    return Item::getUnlocalizedName() + "." + std::string(info(dyeColorByDyeDamage(stack.getMetadata())).unlocalizedName);
}

void ItemDye::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    for (int i = 0; i < 16; ++i)
        subItems.emplace_back(item, 1, i);
}

// --- ItemCoal ---

ItemCoal::ItemCoal()
{
    setHasSubtypes(true);
    setMaxDamage(0);
    setCreativeTab(&CreativeTabs::tabMaterials);
}

std::string ItemCoal::getUnlocalizedName(const ItemStack& stack) const
{
    return stack.getMetadata() == 1 ? "item.charcoal" : "item.coal";
}

void ItemCoal::getSubItems(Item* item, const CreativeTabs*, std::vector<ItemStack>& subItems) const
{
    subItems.emplace_back(item, 1, 0);
    subItems.emplace_back(item, 1, 1);
}

// --- ItemRecord ---

namespace {

std::unordered_map<std::string, ItemRecord*>& records()
{
    static std::unordered_map<std::string, ItemRecord*> map;
    return map;
}

} // namespace

ItemRecord::ItemRecord(std::string name)
    : recordName(std::move(name))
{
    maxStackSize = 1;
    setCreativeTab(&CreativeTabs::tabMisc);
    records()["records." + recordName] = this;
}

std::string ItemRecord::getRecordNameLocal() const
{
    return StatCollector::translateToLocal("item.record." + recordName + ".desc");
}

ItemRecord* ItemRecord::getRecord(const std::string& name)
{
    const auto it = records().find(name);
    return it == records().end() ? nullptr : it->second;
}

// --- ItemExpBottle ---

ItemExpBottle::ItemExpBottle()
{
    setCreativeTab(&CreativeTabs::tabMisc);
}

// --- Books ---

bool ItemWritableBook::isNBTValid(const NBTTagCompound* nbt)
{
    if (!nbt || !nbt->hasKey("pages", 9))
        return false;
    const auto nbttaglist = nbt->getTagList("pages", 8);
    for (int i = 0; i < nbttaglist->tagCount(); ++i) {
        if (java::utf16Length(nbttaglist->getStringTagAt(i)) > 32767)
            return false;
    }
    return true;
}

bool ItemEditableBook::validBookTagContents(const NBTTagCompound* nbt)
{
    if (!ItemWritableBook::isNBTValid(nbt) || !nbt->hasKey("title", 8))
        return false;
    const std::string s = nbt->getString("title");
    return java::utf16Length(s) <= 32 ? nbt->hasKey("author", 8) : false;
}

int ItemEditableBook::getGeneration(const ItemStack& book)
{
    return book.getTagCompound()->getInteger("generation");
}

std::string ItemEditableBook::getItemStackDisplayName(const ItemStack& stack) const
{
    if (stack.hasTagCompound()) {
        const std::string s = stack.getTagCompound()->getString("title");
        if (!s.empty())
            return s;
    }
    return Item::getItemStackDisplayName(stack);
}

} // namespace mc
