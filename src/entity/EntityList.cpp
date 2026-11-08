#include "entity/EntityList.h"

#include "entity/Entity.h"

namespace mc {

namespace {

struct Mapping {
    const char* name;
    int id;
    bool egg;
    int baseColor;
    int spotColor;
};

// The addMapping calls of the static initializer, in order.
constexpr Mapping kMappings[] {
    { "Item", 1, false, 0, 0 },
    { "XPOrb", 2, false, 0, 0 },
    { "ThrownEgg", 7, false, 0, 0 },
    { "LeashKnot", 8, false, 0, 0 },
    { "Painting", 9, false, 0, 0 },
    { "Arrow", 10, false, 0, 0 },
    { "Snowball", 11, false, 0, 0 },
    { "Fireball", 12, false, 0, 0 },
    { "SmallFireball", 13, false, 0, 0 },
    { "ThrownEnderpearl", 14, false, 0, 0 },
    { "EyeOfEnderSignal", 15, false, 0, 0 },
    { "ThrownPotion", 16, false, 0, 0 },
    { "ThrownExpBottle", 17, false, 0, 0 },
    { "ItemFrame", 18, false, 0, 0 },
    { "WitherSkull", 19, false, 0, 0 },
    { "PrimedTnt", 20, false, 0, 0 },
    { "FallingSand", 21, false, 0, 0 },
    { "FireworksRocketEntity", 22, false, 0, 0 },
    { "ArmorStand", 30, false, 0, 0 },
    { "Boat", 41, false, 0, 0 },
    { "MinecartRideable", 42, false, 0, 0 },
    { "MinecartChest", 43, false, 0, 0 },
    { "MinecartFurnace", 44, false, 0, 0 },
    { "MinecartTNT", 45, false, 0, 0 },
    { "MinecartHopper", 46, false, 0, 0 },
    { "MinecartSpawner", 47, false, 0, 0 },
    { "MinecartCommandBlock", 40, false, 0, 0 },
    { "Mob", 48, false, 0, 0 },
    { "Monster", 49, false, 0, 0 },
    { "Creeper", 50, true, 894731, 0 },
    { "Skeleton", 51, true, 12698049, 4802889 },
    { "Spider", 52, true, 3419431, 11013646 },
    { "Giant", 53, false, 0, 0 },
    { "Zombie", 54, true, 44975, 7969893 },
    { "Slime", 55, true, 5349438, 8306542 },
    { "Ghast", 56, true, 16382457, 12369084 },
    { "PigZombie", 57, true, 15373203, 5009705 },
    { "Enderman", 58, true, 1447446, 0 },
    { "CaveSpider", 59, true, 803406, 11013646 },
    { "Silverfish", 60, true, 7237230, 3158064 },
    { "Blaze", 61, true, 16167425, 16775294 },
    { "LavaSlime", 62, true, 3407872, 16579584 },
    { "EnderDragon", 63, false, 0, 0 },
    { "WitherBoss", 64, false, 0, 0 },
    { "Bat", 65, true, 4996656, 986895 },
    { "Witch", 66, true, 3407872, 5349438 },
    { "Endermite", 67, true, 1447446, 7237230 },
    { "Guardian", 68, true, 5931634, 15826224 },
    { "Pig", 90, true, 15771042, 14377823 },
    { "Sheep", 91, true, 15198183, 16758197 },
    { "Cow", 92, true, 4470310, 10592673 },
    { "Chicken", 93, true, 10592673, 16711680 },
    { "Squid", 94, true, 2243405, 7375001 },
    { "Wolf", 95, true, 14144467, 13545366 },
    { "MushroomCow", 96, true, 10489616, 12040119 },
    { "SnowMan", 97, false, 0, 0 },
    { "Ozelot", 98, true, 15720061, 5653556 },
    { "VillagerGolem", 99, false, 0, 0 },
    { "EntityHorse", 100, true, 12623485, 15656192 },
    { "Rabbit", 101, true, 10051392, 7555121 },
    { "Villager", 120, true, 5651507, 12422002 },
    { "EnderCrystal", 200, false, 0, 0 },
};

} // namespace

std::optional<std::string> EntityList::getStringFromID(int id)
{
    for (const Mapping& mapping : kMappings) {
        if (mapping.id == id)
            return std::string(mapping.name);
    }
    return std::nullopt;
}

int EntityList::getIDFromString(std::string_view name)
{
    for (const Mapping& mapping : kMappings) {
        if (mapping.name == name)
            return mapping.id;
    }
    return -1;
}

const std::vector<EntityList::EntityEggInfo>& EntityList::entityEggs()
{
    static const std::vector<EntityEggInfo> eggs = [] {
        std::vector<EntityEggInfo> result;
        for (const Mapping& mapping : kMappings) {
            if (mapping.egg)
                result.push_back({ mapping.id, mapping.baseColor, mapping.spotColor });
        }
        return result;
    }();
    return eggs;
}

const EntityList::EntityEggInfo* EntityList::getEggInfo(int id)
{
    for (const EntityEggInfo& egg : entityEggs()) {
        if (egg.spawnedID == id)
            return &egg;
    }
    return nullptr;
}

std::optional<std::string> EntityList::getEntityString(std::string_view javaClassName)
{
    struct ClassMapping {
        std::string_view className;
        const char* name;
    };
    static constexpr ClassMapping kClasses[] { { "EntityItem", "Item" }, { "EntityXPOrb", "XPOrb" }, { "EntityEgg", "ThrownEgg" }, { "EntityLeashKnot", "LeashKnot" }, { "EntityPainting", "Painting" }, { "EntityArrow", "Arrow" }, { "EntitySnowball", "Snowball" }, { "EntityLargeFireball", "Fireball" }, { "EntitySmallFireball", "SmallFireball" }, { "EntityEnderPearl", "ThrownEnderpearl" }, { "EntityEnderEye", "EyeOfEnderSignal" }, { "EntityPotion", "ThrownPotion" }, { "EntityExpBottle", "ThrownExpBottle" }, { "EntityItemFrame", "ItemFrame" }, { "EntityWitherSkull", "WitherSkull" }, { "EntityTNTPrimed", "PrimedTnt" }, { "EntityFallingBlock", "FallingSand" }, { "EntityFireworkRocket", "FireworksRocketEntity" }, { "EntityArmorStand", "ArmorStand" }, { "EntityBoat", "Boat" }, { "EntityMinecartEmpty", "MinecartRideable" }, { "EntityMinecartChest", "MinecartChest" }, { "EntityMinecartFurnace", "MinecartFurnace" }, { "EntityMinecartTNT", "MinecartTNT" }, { "EntityMinecartHopper", "MinecartHopper" }, { "EntityMinecartMobSpawner", "MinecartSpawner" }, { "EntityMinecartCommandBlock", "MinecartCommandBlock" }, { "EntityLiving", "Mob" }, { "EntityMob", "Monster" }, { "EntityCreeper", "Creeper" }, { "EntitySkeleton", "Skeleton" }, { "EntitySpider", "Spider" }, { "EntityGiantZombie", "Giant" }, { "EntityZombie", "Zombie" }, { "EntitySlime", "Slime" }, { "EntityGhast", "Ghast" }, { "EntityPigZombie", "PigZombie" }, { "EntityEnderman", "Enderman" }, { "EntityCaveSpider", "CaveSpider" }, { "EntitySilverfish", "Silverfish" }, { "EntityBlaze", "Blaze" }, { "EntityMagmaCube", "LavaSlime" }, { "EntityDragon", "EnderDragon" }, { "EntityWither", "WitherBoss" }, { "EntityBat", "Bat" }, { "EntityWitch", "Witch" }, { "EntityEndermite", "Endermite" }, { "EntityGuardian", "Guardian" }, { "EntityPig", "Pig" }, { "EntitySheep", "Sheep" }, { "EntityCow", "Cow" }, { "EntityChicken", "Chicken" }, { "EntitySquid", "Squid" }, { "EntityWolf", "Wolf" }, { "EntityMooshroom", "MushroomCow" }, { "EntitySnowman", "SnowMan" }, { "EntityOcelot", "Ozelot" }, { "EntityIronGolem", "VillagerGolem" }, { "EntityHorse", "EntityHorse" }, { "EntityRabbit", "Rabbit" }, { "EntityVillager", "Villager" }, { "EntityEnderCrystal", "EnderCrystal" }, };
    for (const ClassMapping& mapping : kClasses) {
        if (mapping.className == javaClassName)
            return std::string(mapping.name);
    }
    return std::nullopt;
}

std::optional<std::string> EntityList::getEntityString(const Entity* entity)
{
    return getEntityString(entity->getClassName());
}

int EntityList::getEntityID(const Entity* entity)
{
    const std::optional<std::string> name = getEntityString(entity->getClassName());
    if (!name)
        return 0;
    const int id = getIDFromString(*name);
    return id < 0 ? 0 : id;
}

} // namespace mc
