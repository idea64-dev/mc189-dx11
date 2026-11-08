// Original: Block.registerBlocks() from net/minecraft/block/Block.java -- every block with its id, name and
// the setter chain of the original.

#include "block/Block.h"
#include "block/AllBlocks.h"

namespace mc {

void Block::registerBlocks()
{
    registerBlock(0, ResourceLocation("air"), create<BlockAir>()->setUnlocalizedName("air"));
    registerBlock(1, "stone", create<BlockStone>()->setHardness(1.5f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stone"));
    registerBlock(2, "grass", create<BlockGrass>()->setHardness(0.6f)->setStepSound(soundTypeGrass)->setUnlocalizedName("grass"));
    registerBlock(3, "dirt", create<BlockDirt>()->setHardness(0.5f)->setStepSound(soundTypeGravel)->setUnlocalizedName("dirt"));
    Block* block = create<Block>(Material::rock)->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stonebrick")->setCreativeTab(&CreativeTabs::tabBlock);
    registerBlock(4, "cobblestone", block);
    Block* block1 = create<BlockPlanks>()->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("wood");
    registerBlock(5, "planks", block1);
    registerBlock(6, "sapling", create<BlockSapling>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("sapling"));
    registerBlock(7, "bedrock", create<Block>(Material::rock)->setBlockUnbreakable()->setResistance(6000000.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("bedrock")->disableStats()->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(8, "flowing_water", create<BlockDynamicLiquid>(Material::water)->setHardness(100.0f)->setLightOpacity(3)->setUnlocalizedName("water")->disableStats());
    registerBlock(9, "water", create<BlockStaticLiquid>(Material::water)->setHardness(100.0f)->setLightOpacity(3)->setUnlocalizedName("water")->disableStats());
    registerBlock(10, "flowing_lava", create<BlockDynamicLiquid>(Material::lava)->setHardness(100.0f)->setLightLevel(1.0f)->setUnlocalizedName("lava")->disableStats());
    registerBlock(11, "lava", create<BlockStaticLiquid>(Material::lava)->setHardness(100.0f)->setLightLevel(1.0f)->setUnlocalizedName("lava")->disableStats());
    registerBlock(12, "sand", create<BlockSand>()->setHardness(0.5f)->setStepSound(soundTypeSand)->setUnlocalizedName("sand"));
    registerBlock(13, "gravel", create<BlockGravel>()->setHardness(0.6f)->setStepSound(soundTypeGravel)->setUnlocalizedName("gravel"));
    registerBlock(14, "gold_ore", create<BlockOre>()->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreGold"));
    registerBlock(15, "iron_ore", create<BlockOre>()->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreIron"));
    registerBlock(16, "coal_ore", create<BlockOre>()->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreCoal"));
    registerBlock(17, "log", create<BlockOldLog>()->setUnlocalizedName("log"));
    registerBlock(18, "leaves", create<BlockOldLeaf>()->setUnlocalizedName("leaves"));
    registerBlock(19, "sponge", create<BlockSponge>()->setHardness(0.6f)->setStepSound(soundTypeGrass)->setUnlocalizedName("sponge"));
    registerBlock(20, "glass", create<BlockGlass>(Material::glass, false)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setUnlocalizedName("glass"));
    registerBlock(21, "lapis_ore", create<BlockOre>()->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreLapis"));
    registerBlock(22, "lapis_block", create<Block>(Material::iron, MapColor::lapisColor)->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("blockLapis")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(23, "dispenser", create<BlockDispenser>()->setHardness(3.5f)->setStepSound(soundTypePiston)->setUnlocalizedName("dispenser"));
    Block* block2 = create<BlockSandStone>()->setStepSound(soundTypePiston)->setHardness(0.8f)->setUnlocalizedName("sandStone");
    registerBlock(24, "sandstone", block2);
    registerBlock(25, "noteblock", create<BlockNote>()->setHardness(0.8f)->setUnlocalizedName("musicBlock"));
    registerBlock(26, "bed", create<BlockBed>()->setStepSound(soundTypeWood)->setHardness(0.2f)->setUnlocalizedName("bed")->disableStats());
    registerBlock(27, "golden_rail", create<BlockRailPowered>()->setHardness(0.7f)->setStepSound(soundTypeMetal)->setUnlocalizedName("goldenRail"));
    registerBlock(28, "detector_rail", create<BlockRailDetector>()->setHardness(0.7f)->setStepSound(soundTypeMetal)->setUnlocalizedName("detectorRail"));
    registerBlock(29, "sticky_piston", create<BlockPistonBase>(true)->setUnlocalizedName("pistonStickyBase"));
    registerBlock(30, "web", create<BlockWeb>()->setLightOpacity(1)->setHardness(4.0f)->setUnlocalizedName("web"));
    registerBlock(31, "tallgrass", create<BlockTallGrass>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("tallgrass"));
    registerBlock(32, "deadbush", create<BlockDeadBush>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("deadbush"));
    registerBlock(33, "piston", create<BlockPistonBase>(false)->setUnlocalizedName("pistonBase"));
    registerBlock(34, "piston_head", create<BlockPistonExtension>()->setUnlocalizedName("pistonBase"));
    registerBlock(35, "wool", create<BlockColored>(Material::cloth)->setHardness(0.8f)->setStepSound(soundTypeCloth)->setUnlocalizedName("cloth"));
    registerBlock(36, "piston_extension", create<BlockPistonMoving>());
    registerBlock(37, "yellow_flower", create<BlockYellowFlower>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("flower1"));
    registerBlock(38, "red_flower", create<BlockRedFlower>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("flower2"));
    Block* block3 = create<BlockMushroom>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setLightLevel(0.125f)->setUnlocalizedName("mushroom");
    registerBlock(39, "brown_mushroom", block3);
    Block* block4 = create<BlockMushroom>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("mushroom");
    registerBlock(40, "red_mushroom", block4);
    registerBlock(41, "gold_block", create<Block>(Material::iron, MapColor::goldColor)->setHardness(3.0f)->setResistance(10.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("blockGold")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(42, "iron_block", create<Block>(Material::iron, MapColor::ironColor)->setHardness(5.0f)->setResistance(10.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("blockIron")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(43, "double_stone_slab", create<BlockDoubleStoneSlab>()->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stoneSlab"));
    registerBlock(44, "stone_slab", create<BlockHalfStoneSlab>()->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stoneSlab"));
    Block* block5 = create<Block>(Material::rock, MapColor::redColor)->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("brick")->setCreativeTab(&CreativeTabs::tabBlock);
    registerBlock(45, "brick_block", block5);
    registerBlock(46, "tnt", create<BlockTNT>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("tnt"));
    registerBlock(47, "bookshelf", create<BlockBookshelf>()->setHardness(1.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("bookshelf"));
    registerBlock(48, "mossy_cobblestone", create<Block>(Material::rock)->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stoneMoss")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(49, "obsidian", create<BlockObsidian>()->setHardness(50.0f)->setResistance(2000.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("obsidian"));
    registerBlock(50, "torch", create<BlockTorch>()->setHardness(0.0f)->setLightLevel(0.9375f)->setStepSound(soundTypeWood)->setUnlocalizedName("torch"));
    registerBlock(51, "fire", create<BlockFire>()->setHardness(0.0f)->setLightLevel(1.0f)->setStepSound(soundTypeCloth)->setUnlocalizedName("fire")->disableStats());
    registerBlock(52, "mob_spawner", create<BlockMobSpawner>()->setHardness(5.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("mobSpawner")->disableStats());
    registerBlock(53, "oak_stairs", create<BlockStairs>(block1->getDefaultState()->withProperty(BlockPlanks::VARIANT, BlockPlanks::EnumType::OAK))->setUnlocalizedName("stairsWood"));
    registerBlock(54, "chest", create<BlockChest>(0)->setHardness(2.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("chest"));
    registerBlock(55, "redstone_wire", create<BlockRedstoneWire>()->setHardness(0.0f)->setStepSound(soundTypeStone)->setUnlocalizedName("redstoneDust")->disableStats());
    registerBlock(56, "diamond_ore", create<BlockOre>()->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreDiamond"));
    registerBlock(57, "diamond_block", create<Block>(Material::iron, MapColor::diamondColor)->setHardness(5.0f)->setResistance(10.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("blockDiamond")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(58, "crafting_table", create<BlockWorkbench>()->setHardness(2.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("workbench"));
    registerBlock(59, "wheat", create<BlockCrops>()->setUnlocalizedName("crops"));
    Block* block6 = create<BlockFarmland>()->setHardness(0.6f)->setStepSound(soundTypeGravel)->setUnlocalizedName("farmland");
    registerBlock(60, "farmland", block6);
    registerBlock(61, "furnace", create<BlockFurnace>(false)->setHardness(3.5f)->setStepSound(soundTypePiston)->setUnlocalizedName("furnace")->setCreativeTab(&CreativeTabs::tabDecorations));
    registerBlock(62, "lit_furnace", create<BlockFurnace>(true)->setHardness(3.5f)->setStepSound(soundTypePiston)->setLightLevel(0.875f)->setUnlocalizedName("furnace"));
    registerBlock(63, "standing_sign", create<BlockStandingSign>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("sign")->disableStats());
    registerBlock(64, "wooden_door", create<BlockDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("doorOak")->disableStats());
    registerBlock(65, "ladder", create<BlockLadder>()->setHardness(0.4f)->setStepSound(soundTypeLadder)->setUnlocalizedName("ladder"));
    registerBlock(66, "rail", create<BlockRail>()->setHardness(0.7f)->setStepSound(soundTypeMetal)->setUnlocalizedName("rail"));
    registerBlock(67, "stone_stairs", create<BlockStairs>(block->getDefaultState())->setUnlocalizedName("stairsStone"));
    registerBlock(68, "wall_sign", create<BlockWallSign>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("sign")->disableStats());
    registerBlock(69, "lever", create<BlockLever>()->setHardness(0.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("lever"));
    registerBlock(70, "stone_pressure_plate", create<BlockPressurePlate>(Material::rock, BlockPressurePlate::Sensitivity::MOBS)->setHardness(0.5f)->setStepSound(soundTypePiston)->setUnlocalizedName("pressurePlateStone"));
    registerBlock(71, "iron_door", create<BlockDoor>(Material::iron)->setHardness(5.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("doorIron")->disableStats());
    registerBlock(72, "wooden_pressure_plate", create<BlockPressurePlate>(Material::wood, BlockPressurePlate::Sensitivity::EVERYTHING)->setHardness(0.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("pressurePlateWood"));
    registerBlock(73, "redstone_ore", create<BlockRedstoneOre>(false)->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreRedstone")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(74, "lit_redstone_ore", create<BlockRedstoneOre>(true)->setLightLevel(0.625f)->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreRedstone"));
    registerBlock(75, "unlit_redstone_torch", create<BlockRedstoneTorch>(false)->setHardness(0.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("notGate"));
    registerBlock(76, "redstone_torch", create<BlockRedstoneTorch>(true)->setHardness(0.0f)->setLightLevel(0.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("notGate")->setCreativeTab(&CreativeTabs::tabRedstone));
    registerBlock(77, "stone_button", create<BlockButtonStone>()->setHardness(0.5f)->setStepSound(soundTypePiston)->setUnlocalizedName("button"));
    registerBlock(78, "snow_layer", create<BlockSnow>()->setHardness(0.1f)->setStepSound(soundTypeSnow)->setUnlocalizedName("snow")->setLightOpacity(0));
    registerBlock(79, "ice", create<BlockIce>()->setHardness(0.5f)->setLightOpacity(3)->setStepSound(soundTypeGlass)->setUnlocalizedName("ice"));
    registerBlock(80, "snow", create<BlockSnowBlock>()->setHardness(0.2f)->setStepSound(soundTypeSnow)->setUnlocalizedName("snow"));
    registerBlock(81, "cactus", create<BlockCactus>()->setHardness(0.4f)->setStepSound(soundTypeCloth)->setUnlocalizedName("cactus"));
    registerBlock(82, "clay", create<BlockClay>()->setHardness(0.6f)->setStepSound(soundTypeGravel)->setUnlocalizedName("clay"));
    registerBlock(83, "reeds", create<BlockReed>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("reeds")->disableStats());
    registerBlock(84, "jukebox", create<BlockJukebox>()->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("jukebox"));
    registerBlock(85, "fence", create<BlockFence>(Material::wood, BlockPlanks::getMapColor(BlockPlanks::EnumType::OAK))->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("fence"));
    Block* block7 = create<BlockPumpkin>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("pumpkin");
    registerBlock(86, "pumpkin", block7);
    registerBlock(87, "netherrack", create<BlockNetherrack>()->setHardness(0.4f)->setStepSound(soundTypePiston)->setUnlocalizedName("hellrock"));
    registerBlock(88, "soul_sand", create<BlockSoulSand>()->setHardness(0.5f)->setStepSound(soundTypeSand)->setUnlocalizedName("hellsand"));
    registerBlock(89, "glowstone", create<BlockGlowstone>(Material::glass)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setLightLevel(1.0f)->setUnlocalizedName("lightgem"));
    registerBlock(90, "portal", create<BlockPortal>()->setHardness(-1.0f)->setStepSound(soundTypeGlass)->setLightLevel(0.75f)->setUnlocalizedName("portal"));
    registerBlock(91, "lit_pumpkin", create<BlockPumpkin>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setLightLevel(1.0f)->setUnlocalizedName("litpumpkin"));
    registerBlock(92, "cake", create<BlockCake>()->setHardness(0.5f)->setStepSound(soundTypeCloth)->setUnlocalizedName("cake")->disableStats());
    registerBlock(93, "unpowered_repeater", create<BlockRedstoneRepeater>(false)->setHardness(0.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("diode")->disableStats());
    registerBlock(94, "powered_repeater", create<BlockRedstoneRepeater>(true)->setHardness(0.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("diode")->disableStats());
    registerBlock(95, "stained_glass", create<BlockStainedGlass>(Material::glass)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setUnlocalizedName("stainedGlass"));
    registerBlock(96, "trapdoor", create<BlockTrapDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("trapdoor")->disableStats());
    registerBlock(97, "monster_egg", create<BlockSilverfish>()->setHardness(0.75f)->setUnlocalizedName("monsterStoneEgg"));
    Block* block8 = create<BlockStoneBrick>()->setHardness(1.5f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stonebricksmooth");
    registerBlock(98, "stonebrick", block8);
    registerBlock(99, "brown_mushroom_block", create<BlockHugeMushroom>(Material::wood, MapColor::dirtColor, block3)->setHardness(0.2f)->setStepSound(soundTypeWood)->setUnlocalizedName("mushroom"));
    registerBlock(100, "red_mushroom_block", create<BlockHugeMushroom>(Material::wood, MapColor::redColor, block4)->setHardness(0.2f)->setStepSound(soundTypeWood)->setUnlocalizedName("mushroom"));
    registerBlock(101, "iron_bars", create<BlockPane>(Material::iron, true)->setHardness(5.0f)->setResistance(10.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("fenceIron"));
    registerBlock(102, "glass_pane", create<BlockPane>(Material::glass, false)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setUnlocalizedName("thinGlass"));
    Block* block9 = create<BlockMelon>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("melon");
    registerBlock(103, "melon_block", block9);
    registerBlock(104, "pumpkin_stem", create<BlockStem>(block7)->setHardness(0.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("pumpkinStem"));
    registerBlock(105, "melon_stem", create<BlockStem>(block9)->setHardness(0.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("pumpkinStem"));
    registerBlock(106, "vine", create<BlockVine>()->setHardness(0.2f)->setStepSound(soundTypeGrass)->setUnlocalizedName("vine"));
    registerBlock(107, "fence_gate", create<BlockFenceGate>(BlockPlanks::EnumType::OAK)->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("fenceGate"));
    registerBlock(108, "brick_stairs", create<BlockStairs>(block5->getDefaultState())->setUnlocalizedName("stairsBrick"));
    registerBlock(109, "stone_brick_stairs", create<BlockStairs>(block8->getDefaultState()->withProperty(BlockStoneBrick::VARIANT, BlockStoneBrick::EnumType::DEFAULT))->setUnlocalizedName("stairsStoneBrickSmooth"));
    registerBlock(110, "mycelium", create<BlockMycelium>()->setHardness(0.6f)->setStepSound(soundTypeGrass)->setUnlocalizedName("mycel"));
    registerBlock(111, "waterlily", create<BlockLilyPad>()->setHardness(0.0f)->setStepSound(soundTypeGrass)->setUnlocalizedName("waterlily"));
    Block* block10 = create<BlockNetherBrick>()->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("netherBrick")->setCreativeTab(&CreativeTabs::tabBlock);
    registerBlock(112, "nether_brick", block10);
    registerBlock(113, "nether_brick_fence", create<BlockFence>(Material::rock, MapColor::netherrackColor)->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("netherFence"));
    registerBlock(114, "nether_brick_stairs", create<BlockStairs>(block10->getDefaultState())->setUnlocalizedName("stairsNetherBrick"));
    registerBlock(115, "nether_wart", create<BlockNetherWart>()->setUnlocalizedName("netherStalk"));
    registerBlock(116, "enchanting_table", create<BlockEnchantmentTable>()->setHardness(5.0f)->setResistance(2000.0f)->setUnlocalizedName("enchantmentTable"));
    registerBlock(117, "brewing_stand", create<BlockBrewingStand>()->setHardness(0.5f)->setLightLevel(0.125f)->setUnlocalizedName("brewingStand"));
    registerBlock(118, "cauldron", create<BlockCauldron>()->setHardness(2.0f)->setUnlocalizedName("cauldron"));
    registerBlock(119, "end_portal", create<BlockEndPortal>(Material::portal)->setHardness(-1.0f)->setResistance(6000000.0f));
    registerBlock(120, "end_portal_frame", create<BlockEndPortalFrame>()->setStepSound(soundTypeGlass)->setLightLevel(0.125f)->setHardness(-1.0f)->setUnlocalizedName("endPortalFrame")->setResistance(6000000.0f)->setCreativeTab(&CreativeTabs::tabDecorations));
    registerBlock(121, "end_stone", create<Block>(Material::rock, MapColor::sandColor)->setHardness(3.0f)->setResistance(15.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("whiteStone")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(122, "dragon_egg", create<BlockDragonEgg>()->setHardness(3.0f)->setResistance(15.0f)->setStepSound(soundTypePiston)->setLightLevel(0.125f)->setUnlocalizedName("dragonEgg"));
    registerBlock(123, "redstone_lamp", create<BlockRedstoneLight>(false)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setUnlocalizedName("redstoneLight")->setCreativeTab(&CreativeTabs::tabRedstone));
    registerBlock(124, "lit_redstone_lamp", create<BlockRedstoneLight>(true)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setUnlocalizedName("redstoneLight"));
    registerBlock(125, "double_wooden_slab", create<BlockDoubleWoodSlab>()->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("woodSlab"));
    registerBlock(126, "wooden_slab", create<BlockHalfWoodSlab>()->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("woodSlab"));
    registerBlock(127, "cocoa", create<BlockCocoa>()->setHardness(0.2f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("cocoa"));
    registerBlock(128, "sandstone_stairs", create<BlockStairs>(block2->getDefaultState()->withProperty(BlockSandStone::TYPE, BlockSandStone::EnumType::SMOOTH))->setUnlocalizedName("stairsSandStone"));
    registerBlock(129, "emerald_ore", create<BlockOre>()->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("oreEmerald"));
    registerBlock(130, "ender_chest", create<BlockEnderChest>()->setHardness(22.5f)->setResistance(1000.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("enderChest")->setLightLevel(0.5f));
    registerBlock(131, "tripwire_hook", create<BlockTripWireHook>()->setUnlocalizedName("tripWireSource"));
    registerBlock(132, "tripwire", create<BlockTripWire>()->setUnlocalizedName("tripWire"));
    registerBlock(133, "emerald_block", create<Block>(Material::iron, MapColor::emeraldColor)->setHardness(5.0f)->setResistance(10.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("blockEmerald")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(134, "spruce_stairs", create<BlockStairs>(block1->getDefaultState()->withProperty(BlockPlanks::VARIANT, BlockPlanks::EnumType::SPRUCE))->setUnlocalizedName("stairsWoodSpruce"));
    registerBlock(135, "birch_stairs", create<BlockStairs>(block1->getDefaultState()->withProperty(BlockPlanks::VARIANT, BlockPlanks::EnumType::BIRCH))->setUnlocalizedName("stairsWoodBirch"));
    registerBlock(136, "jungle_stairs", create<BlockStairs>(block1->getDefaultState()->withProperty(BlockPlanks::VARIANT, BlockPlanks::EnumType::JUNGLE))->setUnlocalizedName("stairsWoodJungle"));
    registerBlock(137, "command_block", create<BlockCommandBlock>()->setBlockUnbreakable()->setResistance(6000000.0f)->setUnlocalizedName("commandBlock"));
    registerBlock(138, "beacon", create<BlockBeacon>()->setUnlocalizedName("beacon")->setLightLevel(1.0f));
    registerBlock(139, "cobblestone_wall", create<BlockWall>(block)->setUnlocalizedName("cobbleWall"));
    registerBlock(140, "flower_pot", create<BlockFlowerPot>()->setHardness(0.0f)->setStepSound(soundTypeStone)->setUnlocalizedName("flowerPot"));
    registerBlock(141, "carrots", create<BlockCarrot>()->setUnlocalizedName("carrots"));
    registerBlock(142, "potatoes", create<BlockPotato>()->setUnlocalizedName("potatoes"));
    registerBlock(143, "wooden_button", create<BlockButtonWood>()->setHardness(0.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("button"));
    registerBlock(144, "skull", create<BlockSkull>()->setHardness(1.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("skull"));
    registerBlock(145, "anvil", create<BlockAnvil>()->setHardness(5.0f)->setStepSound(soundTypeAnvil)->setResistance(2000.0f)->setUnlocalizedName("anvil"));
    registerBlock(146, "trapped_chest", create<BlockChest>(1)->setHardness(2.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("chestTrap"));
    registerBlock(147, "light_weighted_pressure_plate", create<BlockPressurePlateWeighted>(Material::iron, 15, MapColor::goldColor)->setHardness(0.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("weightedPlate_light"));
    registerBlock(148, "heavy_weighted_pressure_plate", create<BlockPressurePlateWeighted>(Material::iron, 150)->setHardness(0.5f)->setStepSound(soundTypeWood)->setUnlocalizedName("weightedPlate_heavy"));
    registerBlock(149, "unpowered_comparator", create<BlockRedstoneComparator>(false)->setHardness(0.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("comparator")->disableStats());
    registerBlock(150, "powered_comparator", create<BlockRedstoneComparator>(true)->setHardness(0.0f)->setLightLevel(0.625f)->setStepSound(soundTypeWood)->setUnlocalizedName("comparator")->disableStats());
    registerBlock(151, "daylight_detector", create<BlockDaylightDetector>(false));
    registerBlock(152, "redstone_block", create<BlockCompressedPowered>(Material::iron, MapColor::tntColor)->setHardness(5.0f)->setResistance(10.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("blockRedstone")->setCreativeTab(&CreativeTabs::tabRedstone));
    registerBlock(153, "quartz_ore", create<BlockOre>(MapColor::netherrackColor)->setHardness(3.0f)->setResistance(5.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("netherquartz"));
    registerBlock(154, "hopper", create<BlockHopper>()->setHardness(3.0f)->setResistance(8.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("hopper"));
    Block* block11 = create<BlockQuartz>()->setStepSound(soundTypePiston)->setHardness(0.8f)->setUnlocalizedName("quartzBlock");
    registerBlock(155, "quartz_block", block11);
    registerBlock(156, "quartz_stairs", create<BlockStairs>(block11->getDefaultState()->withProperty(BlockQuartz::VARIANT, BlockQuartz::EnumType::DEFAULT))->setUnlocalizedName("stairsQuartz"));
    registerBlock(157, "activator_rail", create<BlockRailPowered>()->setHardness(0.7f)->setStepSound(soundTypeMetal)->setUnlocalizedName("activatorRail"));
    registerBlock(158, "dropper", create<BlockDropper>()->setHardness(3.5f)->setStepSound(soundTypePiston)->setUnlocalizedName("dropper"));
    registerBlock(159, "stained_hardened_clay", create<BlockColored>(Material::rock)->setHardness(1.25f)->setResistance(7.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("clayHardenedStained"));
    registerBlock(160, "stained_glass_pane", create<BlockStainedGlassPane>()->setHardness(0.3f)->setStepSound(soundTypeGlass)->setUnlocalizedName("thinStainedGlass"));
    registerBlock(161, "leaves2", create<BlockNewLeaf>()->setUnlocalizedName("leaves"));
    registerBlock(162, "log2", create<BlockNewLog>()->setUnlocalizedName("log"));
    registerBlock(163, "acacia_stairs", create<BlockStairs>(block1->getDefaultState()->withProperty(BlockPlanks::VARIANT, BlockPlanks::EnumType::ACACIA))->setUnlocalizedName("stairsWoodAcacia"));
    registerBlock(164, "dark_oak_stairs", create<BlockStairs>(block1->getDefaultState()->withProperty(BlockPlanks::VARIANT, BlockPlanks::EnumType::DARK_OAK))->setUnlocalizedName("stairsWoodDarkOak"));
    registerBlock(165, "slime", create<BlockSlime>()->setUnlocalizedName("slime")->setStepSound(SLIME_SOUND));
    registerBlock(166, "barrier", create<BlockBarrier>()->setUnlocalizedName("barrier"));
    registerBlock(167, "iron_trapdoor", create<BlockTrapDoor>(Material::iron)->setHardness(5.0f)->setStepSound(soundTypeMetal)->setUnlocalizedName("ironTrapdoor")->disableStats());
    registerBlock(168, "prismarine", create<BlockPrismarine>()->setHardness(1.5f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("prismarine"));
    registerBlock(169, "sea_lantern", create<BlockSeaLantern>(Material::glass)->setHardness(0.3f)->setStepSound(soundTypeGlass)->setLightLevel(1.0f)->setUnlocalizedName("seaLantern"));
    registerBlock(170, "hay_block", create<BlockHay>()->setHardness(0.5f)->setStepSound(soundTypeGrass)->setUnlocalizedName("hayBlock")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(171, "carpet", create<BlockCarpet>()->setHardness(0.1f)->setStepSound(soundTypeCloth)->setUnlocalizedName("woolCarpet")->setLightOpacity(0));
    registerBlock(172, "hardened_clay", create<BlockHardenedClay>()->setHardness(1.25f)->setResistance(7.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("clayHardened"));
    registerBlock(173, "coal_block", create<Block>(Material::rock, MapColor::blackColor)->setHardness(5.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("blockCoal")->setCreativeTab(&CreativeTabs::tabBlock));
    registerBlock(174, "packed_ice", create<BlockPackedIce>()->setHardness(0.5f)->setStepSound(soundTypeGlass)->setUnlocalizedName("icePacked"));
    registerBlock(175, "double_plant", create<BlockDoublePlant>());
    registerBlock(176, "standing_banner", create<BlockBanner::BlockBannerStanding>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("banner")->disableStats());
    registerBlock(177, "wall_banner", create<BlockBanner::BlockBannerHanging>()->setHardness(1.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("banner")->disableStats());
    registerBlock(178, "daylight_detector_inverted", create<BlockDaylightDetector>(true));
    Block* block12 = create<BlockRedSandstone>()->setStepSound(soundTypePiston)->setHardness(0.8f)->setUnlocalizedName("redSandStone");
    registerBlock(179, "red_sandstone", block12);
    registerBlock(180, "red_sandstone_stairs", create<BlockStairs>(block12->getDefaultState()->withProperty(BlockRedSandstone::TYPE, BlockRedSandstone::EnumType::SMOOTH))->setUnlocalizedName("stairsRedSandStone"));
    registerBlock(181, "double_stone_slab2", create<BlockDoubleStoneSlabNew>()->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stoneSlab2"));
    registerBlock(182, "stone_slab2", create<BlockHalfStoneSlabNew>()->setHardness(2.0f)->setResistance(10.0f)->setStepSound(soundTypePiston)->setUnlocalizedName("stoneSlab2"));
    registerBlock(183, "spruce_fence_gate", create<BlockFenceGate>(BlockPlanks::EnumType::SPRUCE)->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("spruceFenceGate"));
    registerBlock(184, "birch_fence_gate", create<BlockFenceGate>(BlockPlanks::EnumType::BIRCH)->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("birchFenceGate"));
    registerBlock(185, "jungle_fence_gate", create<BlockFenceGate>(BlockPlanks::EnumType::JUNGLE)->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("jungleFenceGate"));
    registerBlock(186, "dark_oak_fence_gate", create<BlockFenceGate>(BlockPlanks::EnumType::DARK_OAK)->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("darkOakFenceGate"));
    registerBlock(187, "acacia_fence_gate", create<BlockFenceGate>(BlockPlanks::EnumType::ACACIA)->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("acaciaFenceGate"));
    registerBlock(188, "spruce_fence", create<BlockFence>(Material::wood, BlockPlanks::getMapColor(BlockPlanks::EnumType::SPRUCE))->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("spruceFence"));
    registerBlock(189, "birch_fence", create<BlockFence>(Material::wood, BlockPlanks::getMapColor(BlockPlanks::EnumType::BIRCH))->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("birchFence"));
    registerBlock(190, "jungle_fence", create<BlockFence>(Material::wood, BlockPlanks::getMapColor(BlockPlanks::EnumType::JUNGLE))->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("jungleFence"));
    registerBlock(191, "dark_oak_fence", create<BlockFence>(Material::wood, BlockPlanks::getMapColor(BlockPlanks::EnumType::DARK_OAK))->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("darkOakFence"));
    registerBlock(192, "acacia_fence", create<BlockFence>(Material::wood, BlockPlanks::getMapColor(BlockPlanks::EnumType::ACACIA))->setHardness(2.0f)->setResistance(5.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("acaciaFence"));
    registerBlock(193, "spruce_door", create<BlockDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("doorSpruce")->disableStats());
    registerBlock(194, "birch_door", create<BlockDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("doorBirch")->disableStats());
    registerBlock(195, "jungle_door", create<BlockDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("doorJungle")->disableStats());
    registerBlock(196, "acacia_door", create<BlockDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("doorAcacia")->disableStats());
    registerBlock(197, "dark_oak_door", create<BlockDoor>(Material::wood)->setHardness(3.0f)->setStepSound(soundTypeWood)->setUnlocalizedName("doorDarkOak")->disableStats());

    // Java: blockRegistry.validateKey()
    for (Block* block13 : blockRegistry.values()) {
        if (&block13->blockMaterial == &Material::air) {
            block13->useNeighborBrightness = false;
        } else {
            bool flag = false;
            const bool flag1 = dynamic_cast<BlockStairs*>(block13) != nullptr;
            const bool flag2 = dynamic_cast<BlockSlab*>(block13) != nullptr;
            const bool flag3 = block13 == block6;
            const bool flag4 = block13->translucent;
            const bool flag5 = block13->lightOpacity == 0;
            if (flag1 || flag2 || flag3 || flag4 || flag5)
                flag = true;
            block13->useNeighborBrightness = flag;
        }
    }

    for (Block* block14 : blockRegistry.values()) {
        for (const IBlockState* state : block14->getBlockState().getValidStates()) {
            const int i = blockRegistry.getIDForObject(block14) << 4 | block14->getMetaFromState(state);
            BLOCK_STATE_IDS.put(state, i);
        }
    }
}

} // namespace mc
