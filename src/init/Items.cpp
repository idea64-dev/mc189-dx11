#include "init/Items.h"

#include "item/AllItems.h"

#include <stdexcept>
#include <string>

namespace mc::Items {

namespace {

template <class T>
T* getRegisteredItem(const char* name)
{
    // Java: itemRegistry.getObject, then a cast; an unknown name gives null.
    Item* item = Item::itemRegistry.getObject(ResourceLocation(name));
    auto* typed = dynamic_cast<T*>(item);
    if (typed == nullptr)
        throw std::logic_error(std::string("Invalid Item requested: ") + name);
    return typed;
}

} // namespace

void init()
{
    iron_shovel = getRegisteredItem<Item>("iron_shovel");
    iron_pickaxe = getRegisteredItem<Item>("iron_pickaxe");
    iron_axe = getRegisteredItem<Item>("iron_axe");
    flint_and_steel = getRegisteredItem<Item>("flint_and_steel");
    apple = getRegisteredItem<Item>("apple");
    bow = getRegisteredItem<ItemBow>("bow");
    arrow = getRegisteredItem<Item>("arrow");
    coal = getRegisteredItem<Item>("coal");
    diamond = getRegisteredItem<Item>("diamond");
    iron_ingot = getRegisteredItem<Item>("iron_ingot");
    gold_ingot = getRegisteredItem<Item>("gold_ingot");
    iron_sword = getRegisteredItem<Item>("iron_sword");
    wooden_sword = getRegisteredItem<Item>("wooden_sword");
    wooden_shovel = getRegisteredItem<Item>("wooden_shovel");
    wooden_pickaxe = getRegisteredItem<Item>("wooden_pickaxe");
    wooden_axe = getRegisteredItem<Item>("wooden_axe");
    stone_sword = getRegisteredItem<Item>("stone_sword");
    stone_shovel = getRegisteredItem<Item>("stone_shovel");
    stone_pickaxe = getRegisteredItem<Item>("stone_pickaxe");
    stone_axe = getRegisteredItem<Item>("stone_axe");
    diamond_sword = getRegisteredItem<Item>("diamond_sword");
    diamond_shovel = getRegisteredItem<Item>("diamond_shovel");
    diamond_pickaxe = getRegisteredItem<Item>("diamond_pickaxe");
    diamond_axe = getRegisteredItem<Item>("diamond_axe");
    stick = getRegisteredItem<Item>("stick");
    bowl = getRegisteredItem<Item>("bowl");
    mushroom_stew = getRegisteredItem<Item>("mushroom_stew");
    golden_sword = getRegisteredItem<Item>("golden_sword");
    golden_shovel = getRegisteredItem<Item>("golden_shovel");
    golden_pickaxe = getRegisteredItem<Item>("golden_pickaxe");
    golden_axe = getRegisteredItem<Item>("golden_axe");
    string = getRegisteredItem<Item>("string");
    feather = getRegisteredItem<Item>("feather");
    gunpowder = getRegisteredItem<Item>("gunpowder");
    wooden_hoe = getRegisteredItem<Item>("wooden_hoe");
    stone_hoe = getRegisteredItem<Item>("stone_hoe");
    iron_hoe = getRegisteredItem<Item>("iron_hoe");
    diamond_hoe = getRegisteredItem<Item>("diamond_hoe");
    golden_hoe = getRegisteredItem<Item>("golden_hoe");
    wheat_seeds = getRegisteredItem<Item>("wheat_seeds");
    wheat = getRegisteredItem<Item>("wheat");
    bread = getRegisteredItem<Item>("bread");
    leather_helmet = getRegisteredItem<ItemArmor>("leather_helmet");
    leather_chestplate = getRegisteredItem<ItemArmor>("leather_chestplate");
    leather_leggings = getRegisteredItem<ItemArmor>("leather_leggings");
    leather_boots = getRegisteredItem<ItemArmor>("leather_boots");
    chainmail_helmet = getRegisteredItem<ItemArmor>("chainmail_helmet");
    chainmail_chestplate = getRegisteredItem<ItemArmor>("chainmail_chestplate");
    chainmail_leggings = getRegisteredItem<ItemArmor>("chainmail_leggings");
    chainmail_boots = getRegisteredItem<ItemArmor>("chainmail_boots");
    iron_helmet = getRegisteredItem<ItemArmor>("iron_helmet");
    iron_chestplate = getRegisteredItem<ItemArmor>("iron_chestplate");
    iron_leggings = getRegisteredItem<ItemArmor>("iron_leggings");
    iron_boots = getRegisteredItem<ItemArmor>("iron_boots");
    diamond_helmet = getRegisteredItem<ItemArmor>("diamond_helmet");
    diamond_chestplate = getRegisteredItem<ItemArmor>("diamond_chestplate");
    diamond_leggings = getRegisteredItem<ItemArmor>("diamond_leggings");
    diamond_boots = getRegisteredItem<ItemArmor>("diamond_boots");
    golden_helmet = getRegisteredItem<ItemArmor>("golden_helmet");
    golden_chestplate = getRegisteredItem<ItemArmor>("golden_chestplate");
    golden_leggings = getRegisteredItem<ItemArmor>("golden_leggings");
    golden_boots = getRegisteredItem<ItemArmor>("golden_boots");
    flint = getRegisteredItem<Item>("flint");
    porkchop = getRegisteredItem<Item>("porkchop");
    cooked_porkchop = getRegisteredItem<Item>("cooked_porkchop");
    painting = getRegisteredItem<Item>("painting");
    golden_apple = getRegisteredItem<Item>("golden_apple");
    sign = getRegisteredItem<Item>("sign");
    oak_door = getRegisteredItem<Item>("wooden_door");
    spruce_door = getRegisteredItem<Item>("spruce_door");
    birch_door = getRegisteredItem<Item>("birch_door");
    jungle_door = getRegisteredItem<Item>("jungle_door");
    acacia_door = getRegisteredItem<Item>("acacia_door");
    dark_oak_door = getRegisteredItem<Item>("dark_oak_door");
    bucket = getRegisteredItem<Item>("bucket");
    water_bucket = getRegisteredItem<Item>("water_bucket");
    lava_bucket = getRegisteredItem<Item>("lava_bucket");
    minecart = getRegisteredItem<Item>("minecart");
    saddle = getRegisteredItem<Item>("saddle");
    iron_door = getRegisteredItem<Item>("iron_door");
    redstone = getRegisteredItem<Item>("redstone");
    snowball = getRegisteredItem<Item>("snowball");
    boat = getRegisteredItem<Item>("boat");
    leather = getRegisteredItem<Item>("leather");
    milk_bucket = getRegisteredItem<Item>("milk_bucket");
    brick = getRegisteredItem<Item>("brick");
    clay_ball = getRegisteredItem<Item>("clay_ball");
    reeds = getRegisteredItem<Item>("reeds");
    paper = getRegisteredItem<Item>("paper");
    book = getRegisteredItem<Item>("book");
    slime_ball = getRegisteredItem<Item>("slime_ball");
    chest_minecart = getRegisteredItem<Item>("chest_minecart");
    furnace_minecart = getRegisteredItem<Item>("furnace_minecart");
    egg = getRegisteredItem<Item>("egg");
    compass = getRegisteredItem<Item>("compass");
    fishing_rod = getRegisteredItem<ItemFishingRod>("fishing_rod");
    clock = getRegisteredItem<Item>("clock");
    glowstone_dust = getRegisteredItem<Item>("glowstone_dust");
    fish = getRegisteredItem<Item>("fish");
    cooked_fish = getRegisteredItem<Item>("cooked_fish");
    dye = getRegisteredItem<Item>("dye");
    bone = getRegisteredItem<Item>("bone");
    sugar = getRegisteredItem<Item>("sugar");
    cake = getRegisteredItem<Item>("cake");
    bed = getRegisteredItem<Item>("bed");
    repeater = getRegisteredItem<Item>("repeater");
    cookie = getRegisteredItem<Item>("cookie");
    filled_map = getRegisteredItem<ItemMap>("filled_map");
    shears = getRegisteredItem<ItemShears>("shears");
    melon = getRegisteredItem<Item>("melon");
    pumpkin_seeds = getRegisteredItem<Item>("pumpkin_seeds");
    melon_seeds = getRegisteredItem<Item>("melon_seeds");
    beef = getRegisteredItem<Item>("beef");
    cooked_beef = getRegisteredItem<Item>("cooked_beef");
    chicken = getRegisteredItem<Item>("chicken");
    cooked_chicken = getRegisteredItem<Item>("cooked_chicken");
    mutton = getRegisteredItem<Item>("mutton");
    cooked_mutton = getRegisteredItem<Item>("cooked_mutton");
    rabbit = getRegisteredItem<Item>("rabbit");
    cooked_rabbit = getRegisteredItem<Item>("cooked_rabbit");
    rabbit_stew = getRegisteredItem<Item>("rabbit_stew");
    rabbit_foot = getRegisteredItem<Item>("rabbit_foot");
    rabbit_hide = getRegisteredItem<Item>("rabbit_hide");
    rotten_flesh = getRegisteredItem<Item>("rotten_flesh");
    ender_pearl = getRegisteredItem<Item>("ender_pearl");
    blaze_rod = getRegisteredItem<Item>("blaze_rod");
    ghast_tear = getRegisteredItem<Item>("ghast_tear");
    gold_nugget = getRegisteredItem<Item>("gold_nugget");
    nether_wart = getRegisteredItem<Item>("nether_wart");
    potionitem = getRegisteredItem<ItemPotion>("potion");
    glass_bottle = getRegisteredItem<Item>("glass_bottle");
    spider_eye = getRegisteredItem<Item>("spider_eye");
    fermented_spider_eye = getRegisteredItem<Item>("fermented_spider_eye");
    blaze_powder = getRegisteredItem<Item>("blaze_powder");
    magma_cream = getRegisteredItem<Item>("magma_cream");
    brewing_stand = getRegisteredItem<Item>("brewing_stand");
    cauldron = getRegisteredItem<Item>("cauldron");
    ender_eye = getRegisteredItem<Item>("ender_eye");
    speckled_melon = getRegisteredItem<Item>("speckled_melon");
    spawn_egg = getRegisteredItem<Item>("spawn_egg");
    experience_bottle = getRegisteredItem<Item>("experience_bottle");
    fire_charge = getRegisteredItem<Item>("fire_charge");
    writable_book = getRegisteredItem<Item>("writable_book");
    written_book = getRegisteredItem<Item>("written_book");
    emerald = getRegisteredItem<Item>("emerald");
    item_frame = getRegisteredItem<Item>("item_frame");
    flower_pot = getRegisteredItem<Item>("flower_pot");
    carrot = getRegisteredItem<Item>("carrot");
    potato = getRegisteredItem<Item>("potato");
    baked_potato = getRegisteredItem<Item>("baked_potato");
    poisonous_potato = getRegisteredItem<Item>("poisonous_potato");
    map = getRegisteredItem<ItemEmptyMap>("map");
    golden_carrot = getRegisteredItem<Item>("golden_carrot");
    skull = getRegisteredItem<Item>("skull");
    carrot_on_a_stick = getRegisteredItem<Item>("carrot_on_a_stick");
    nether_star = getRegisteredItem<Item>("nether_star");
    pumpkin_pie = getRegisteredItem<Item>("pumpkin_pie");
    fireworks = getRegisteredItem<Item>("fireworks");
    firework_charge = getRegisteredItem<Item>("firework_charge");
    enchanted_book = getRegisteredItem<ItemEnchantedBook>("enchanted_book");
    comparator = getRegisteredItem<Item>("comparator");
    netherbrick = getRegisteredItem<Item>("netherbrick");
    quartz = getRegisteredItem<Item>("quartz");
    tnt_minecart = getRegisteredItem<Item>("tnt_minecart");
    hopper_minecart = getRegisteredItem<Item>("hopper_minecart");
    armor_stand = getRegisteredItem<ItemArmorStand>("armor_stand");
    iron_horse_armor = getRegisteredItem<Item>("iron_horse_armor");
    golden_horse_armor = getRegisteredItem<Item>("golden_horse_armor");
    diamond_horse_armor = getRegisteredItem<Item>("diamond_horse_armor");
    lead = getRegisteredItem<Item>("lead");
    name_tag = getRegisteredItem<Item>("name_tag");
    command_block_minecart = getRegisteredItem<Item>("command_block_minecart");
    record_13 = getRegisteredItem<Item>("record_13");
    record_cat = getRegisteredItem<Item>("record_cat");
    record_blocks = getRegisteredItem<Item>("record_blocks");
    record_chirp = getRegisteredItem<Item>("record_chirp");
    record_far = getRegisteredItem<Item>("record_far");
    record_mall = getRegisteredItem<Item>("record_mall");
    record_mellohi = getRegisteredItem<Item>("record_mellohi");
    record_stal = getRegisteredItem<Item>("record_stal");
    record_strad = getRegisteredItem<Item>("record_strad");
    record_ward = getRegisteredItem<Item>("record_ward");
    record_11 = getRegisteredItem<Item>("record_11");
    record_wait = getRegisteredItem<Item>("record_wait");
    prismarine_shard = getRegisteredItem<Item>("prismarine_shard");
    prismarine_crystals = getRegisteredItem<Item>("prismarine_crystals");
    banner = getRegisteredItem<Item>("banner");
}

} // namespace mc::Items
