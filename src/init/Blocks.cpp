#include "init/Blocks.h"

#include "block/AllBlocks.h"

#include <stdexcept>

namespace mc::Blocks {

namespace {

template <class T>
T* getRegisteredBlock(const char* name)
{
    // Java: blockRegistry.getObject -- an unknown name would give air.
    Block* block = Block::blockRegistry.getObject(ResourceLocation(name));
    auto* typed = dynamic_cast<T*>(block);
    if (typed == nullptr)
        throw std::logic_error(std::string("Invalid Block requested: ") + name);
    return typed;
}

} // namespace

void init()
{
    air = getRegisteredBlock<Block>("air");
    stone = getRegisteredBlock<Block>("stone");
    grass = getRegisteredBlock<BlockGrass>("grass");
    dirt = getRegisteredBlock<Block>("dirt");
    cobblestone = getRegisteredBlock<Block>("cobblestone");
    planks = getRegisteredBlock<Block>("planks");
    sapling = getRegisteredBlock<Block>("sapling");
    bedrock = getRegisteredBlock<Block>("bedrock");
    flowing_water = getRegisteredBlock<BlockDynamicLiquid>("flowing_water");
    water = getRegisteredBlock<BlockStaticLiquid>("water");
    flowing_lava = getRegisteredBlock<BlockDynamicLiquid>("flowing_lava");
    lava = getRegisteredBlock<BlockStaticLiquid>("lava");
    sand = getRegisteredBlock<BlockSand>("sand");
    gravel = getRegisteredBlock<Block>("gravel");
    gold_ore = getRegisteredBlock<Block>("gold_ore");
    iron_ore = getRegisteredBlock<Block>("iron_ore");
    coal_ore = getRegisteredBlock<Block>("coal_ore");
    log = getRegisteredBlock<Block>("log");
    log2 = getRegisteredBlock<Block>("log2");
    leaves = getRegisteredBlock<BlockLeaves>("leaves");
    leaves2 = getRegisteredBlock<BlockLeaves>("leaves2");
    sponge = getRegisteredBlock<Block>("sponge");
    glass = getRegisteredBlock<Block>("glass");
    lapis_ore = getRegisteredBlock<Block>("lapis_ore");
    lapis_block = getRegisteredBlock<Block>("lapis_block");
    dispenser = getRegisteredBlock<Block>("dispenser");
    sandstone = getRegisteredBlock<Block>("sandstone");
    noteblock = getRegisteredBlock<Block>("noteblock");
    bed = getRegisteredBlock<Block>("bed");
    golden_rail = getRegisteredBlock<Block>("golden_rail");
    detector_rail = getRegisteredBlock<Block>("detector_rail");
    sticky_piston = getRegisteredBlock<BlockPistonBase>("sticky_piston");
    web = getRegisteredBlock<Block>("web");
    tallgrass = getRegisteredBlock<BlockTallGrass>("tallgrass");
    deadbush = getRegisteredBlock<BlockDeadBush>("deadbush");
    piston = getRegisteredBlock<BlockPistonBase>("piston");
    piston_head = getRegisteredBlock<BlockPistonExtension>("piston_head");
    wool = getRegisteredBlock<Block>("wool");
    piston_extension = getRegisteredBlock<BlockPistonMoving>("piston_extension");
    yellow_flower = getRegisteredBlock<BlockFlower>("yellow_flower");
    red_flower = getRegisteredBlock<BlockFlower>("red_flower");
    brown_mushroom = getRegisteredBlock<BlockBush>("brown_mushroom");
    red_mushroom = getRegisteredBlock<BlockBush>("red_mushroom");
    gold_block = getRegisteredBlock<Block>("gold_block");
    iron_block = getRegisteredBlock<Block>("iron_block");
    double_stone_slab = getRegisteredBlock<BlockSlab>("double_stone_slab");
    stone_slab = getRegisteredBlock<BlockSlab>("stone_slab");
    brick_block = getRegisteredBlock<Block>("brick_block");
    tnt = getRegisteredBlock<Block>("tnt");
    bookshelf = getRegisteredBlock<Block>("bookshelf");
    mossy_cobblestone = getRegisteredBlock<Block>("mossy_cobblestone");
    obsidian = getRegisteredBlock<Block>("obsidian");
    torch = getRegisteredBlock<Block>("torch");
    fire = getRegisteredBlock<BlockFire>("fire");
    mob_spawner = getRegisteredBlock<Block>("mob_spawner");
    oak_stairs = getRegisteredBlock<Block>("oak_stairs");
    chest = getRegisteredBlock<BlockChest>("chest");
    redstone_wire = getRegisteredBlock<BlockRedstoneWire>("redstone_wire");
    diamond_ore = getRegisteredBlock<Block>("diamond_ore");
    diamond_block = getRegisteredBlock<Block>("diamond_block");
    crafting_table = getRegisteredBlock<Block>("crafting_table");
    wheat = getRegisteredBlock<Block>("wheat");
    farmland = getRegisteredBlock<Block>("farmland");
    furnace = getRegisteredBlock<Block>("furnace");
    lit_furnace = getRegisteredBlock<Block>("lit_furnace");
    standing_sign = getRegisteredBlock<Block>("standing_sign");
    oak_door = getRegisteredBlock<Block>("wooden_door");
    spruce_door = getRegisteredBlock<Block>("spruce_door");
    birch_door = getRegisteredBlock<Block>("birch_door");
    jungle_door = getRegisteredBlock<Block>("jungle_door");
    acacia_door = getRegisteredBlock<Block>("acacia_door");
    dark_oak_door = getRegisteredBlock<Block>("dark_oak_door");
    ladder = getRegisteredBlock<Block>("ladder");
    rail = getRegisteredBlock<Block>("rail");
    stone_stairs = getRegisteredBlock<Block>("stone_stairs");
    wall_sign = getRegisteredBlock<Block>("wall_sign");
    lever = getRegisteredBlock<Block>("lever");
    stone_pressure_plate = getRegisteredBlock<Block>("stone_pressure_plate");
    iron_door = getRegisteredBlock<Block>("iron_door");
    wooden_pressure_plate = getRegisteredBlock<Block>("wooden_pressure_plate");
    redstone_ore = getRegisteredBlock<Block>("redstone_ore");
    lit_redstone_ore = getRegisteredBlock<Block>("lit_redstone_ore");
    unlit_redstone_torch = getRegisteredBlock<Block>("unlit_redstone_torch");
    redstone_torch = getRegisteredBlock<Block>("redstone_torch");
    stone_button = getRegisteredBlock<Block>("stone_button");
    snow_layer = getRegisteredBlock<Block>("snow_layer");
    ice = getRegisteredBlock<Block>("ice");
    snow = getRegisteredBlock<Block>("snow");
    cactus = getRegisteredBlock<BlockCactus>("cactus");
    clay = getRegisteredBlock<Block>("clay");
    reeds = getRegisteredBlock<BlockReed>("reeds");
    jukebox = getRegisteredBlock<Block>("jukebox");
    oak_fence = getRegisteredBlock<Block>("fence");
    spruce_fence = getRegisteredBlock<Block>("spruce_fence");
    birch_fence = getRegisteredBlock<Block>("birch_fence");
    jungle_fence = getRegisteredBlock<Block>("jungle_fence");
    dark_oak_fence = getRegisteredBlock<Block>("dark_oak_fence");
    acacia_fence = getRegisteredBlock<Block>("acacia_fence");
    pumpkin = getRegisteredBlock<Block>("pumpkin");
    netherrack = getRegisteredBlock<Block>("netherrack");
    soul_sand = getRegisteredBlock<Block>("soul_sand");
    glowstone = getRegisteredBlock<Block>("glowstone");
    portal = getRegisteredBlock<BlockPortal>("portal");
    lit_pumpkin = getRegisteredBlock<Block>("lit_pumpkin");
    cake = getRegisteredBlock<Block>("cake");
    unpowered_repeater = getRegisteredBlock<BlockRedstoneRepeater>("unpowered_repeater");
    powered_repeater = getRegisteredBlock<BlockRedstoneRepeater>("powered_repeater");
    trapdoor = getRegisteredBlock<Block>("trapdoor");
    monster_egg = getRegisteredBlock<Block>("monster_egg");
    stonebrick = getRegisteredBlock<Block>("stonebrick");
    brown_mushroom_block = getRegisteredBlock<Block>("brown_mushroom_block");
    red_mushroom_block = getRegisteredBlock<Block>("red_mushroom_block");
    iron_bars = getRegisteredBlock<Block>("iron_bars");
    glass_pane = getRegisteredBlock<Block>("glass_pane");
    melon_block = getRegisteredBlock<Block>("melon_block");
    pumpkin_stem = getRegisteredBlock<Block>("pumpkin_stem");
    melon_stem = getRegisteredBlock<Block>("melon_stem");
    vine = getRegisteredBlock<Block>("vine");
    oak_fence_gate = getRegisteredBlock<Block>("fence_gate");
    spruce_fence_gate = getRegisteredBlock<Block>("spruce_fence_gate");
    birch_fence_gate = getRegisteredBlock<Block>("birch_fence_gate");
    jungle_fence_gate = getRegisteredBlock<Block>("jungle_fence_gate");
    dark_oak_fence_gate = getRegisteredBlock<Block>("dark_oak_fence_gate");
    acacia_fence_gate = getRegisteredBlock<Block>("acacia_fence_gate");
    brick_stairs = getRegisteredBlock<Block>("brick_stairs");
    stone_brick_stairs = getRegisteredBlock<Block>("stone_brick_stairs");
    mycelium = getRegisteredBlock<BlockMycelium>("mycelium");
    waterlily = getRegisteredBlock<Block>("waterlily");
    nether_brick = getRegisteredBlock<Block>("nether_brick");
    nether_brick_fence = getRegisteredBlock<Block>("nether_brick_fence");
    nether_brick_stairs = getRegisteredBlock<Block>("nether_brick_stairs");
    nether_wart = getRegisteredBlock<Block>("nether_wart");
    enchanting_table = getRegisteredBlock<Block>("enchanting_table");
    brewing_stand = getRegisteredBlock<Block>("brewing_stand");
    cauldron = getRegisteredBlock<BlockCauldron>("cauldron");
    end_portal = getRegisteredBlock<Block>("end_portal");
    end_portal_frame = getRegisteredBlock<Block>("end_portal_frame");
    end_stone = getRegisteredBlock<Block>("end_stone");
    dragon_egg = getRegisteredBlock<Block>("dragon_egg");
    redstone_lamp = getRegisteredBlock<Block>("redstone_lamp");
    lit_redstone_lamp = getRegisteredBlock<Block>("lit_redstone_lamp");
    double_wooden_slab = getRegisteredBlock<BlockSlab>("double_wooden_slab");
    wooden_slab = getRegisteredBlock<BlockSlab>("wooden_slab");
    cocoa = getRegisteredBlock<Block>("cocoa");
    sandstone_stairs = getRegisteredBlock<Block>("sandstone_stairs");
    emerald_ore = getRegisteredBlock<Block>("emerald_ore");
    ender_chest = getRegisteredBlock<Block>("ender_chest");
    tripwire_hook = getRegisteredBlock<BlockTripWireHook>("tripwire_hook");
    tripwire = getRegisteredBlock<Block>("tripwire");
    emerald_block = getRegisteredBlock<Block>("emerald_block");
    spruce_stairs = getRegisteredBlock<Block>("spruce_stairs");
    birch_stairs = getRegisteredBlock<Block>("birch_stairs");
    jungle_stairs = getRegisteredBlock<Block>("jungle_stairs");
    command_block = getRegisteredBlock<Block>("command_block");
    beacon = getRegisteredBlock<BlockBeacon>("beacon");
    cobblestone_wall = getRegisteredBlock<Block>("cobblestone_wall");
    flower_pot = getRegisteredBlock<Block>("flower_pot");
    carrots = getRegisteredBlock<Block>("carrots");
    potatoes = getRegisteredBlock<Block>("potatoes");
    wooden_button = getRegisteredBlock<Block>("wooden_button");
    skull = getRegisteredBlock<BlockSkull>("skull");
    anvil = getRegisteredBlock<Block>("anvil");
    trapped_chest = getRegisteredBlock<Block>("trapped_chest");
    light_weighted_pressure_plate = getRegisteredBlock<Block>("light_weighted_pressure_plate");
    heavy_weighted_pressure_plate = getRegisteredBlock<Block>("heavy_weighted_pressure_plate");
    unpowered_comparator = getRegisteredBlock<BlockRedstoneComparator>("unpowered_comparator");
    powered_comparator = getRegisteredBlock<BlockRedstoneComparator>("powered_comparator");
    daylight_detector = getRegisteredBlock<BlockDaylightDetector>("daylight_detector");
    daylight_detector_inverted = getRegisteredBlock<BlockDaylightDetector>("daylight_detector_inverted");
    redstone_block = getRegisteredBlock<Block>("redstone_block");
    quartz_ore = getRegisteredBlock<Block>("quartz_ore");
    hopper = getRegisteredBlock<BlockHopper>("hopper");
    quartz_block = getRegisteredBlock<Block>("quartz_block");
    quartz_stairs = getRegisteredBlock<Block>("quartz_stairs");
    activator_rail = getRegisteredBlock<Block>("activator_rail");
    dropper = getRegisteredBlock<Block>("dropper");
    stained_hardened_clay = getRegisteredBlock<Block>("stained_hardened_clay");
    barrier = getRegisteredBlock<Block>("barrier");
    iron_trapdoor = getRegisteredBlock<Block>("iron_trapdoor");
    hay_block = getRegisteredBlock<Block>("hay_block");
    carpet = getRegisteredBlock<Block>("carpet");
    hardened_clay = getRegisteredBlock<Block>("hardened_clay");
    coal_block = getRegisteredBlock<Block>("coal_block");
    packed_ice = getRegisteredBlock<Block>("packed_ice");
    acacia_stairs = getRegisteredBlock<Block>("acacia_stairs");
    dark_oak_stairs = getRegisteredBlock<Block>("dark_oak_stairs");
    slime_block = getRegisteredBlock<Block>("slime");
    double_plant = getRegisteredBlock<BlockDoublePlant>("double_plant");
    stained_glass = getRegisteredBlock<BlockStainedGlass>("stained_glass");
    stained_glass_pane = getRegisteredBlock<BlockStainedGlassPane>("stained_glass_pane");
    prismarine = getRegisteredBlock<Block>("prismarine");
    sea_lantern = getRegisteredBlock<Block>("sea_lantern");
    standing_banner = getRegisteredBlock<Block>("standing_banner");
    wall_banner = getRegisteredBlock<Block>("wall_banner");
    red_sandstone = getRegisteredBlock<Block>("red_sandstone");
    red_sandstone_stairs = getRegisteredBlock<Block>("red_sandstone_stairs");
    double_stone_slab2 = getRegisteredBlock<BlockSlab>("double_stone_slab2");
    stone_slab2 = getRegisteredBlock<BlockSlab>("stone_slab2");
}

} // namespace mc::Blocks
