package net.minecraft.block;

import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.IdentityHashMap;
import java.util.Map;

import net.minecraft.block.material.Material;
import net.minecraft.block.state.IBlockState;
import net.minecraft.block.properties.IProperty;

/**
 * Text dump of the block registry as the original builds it: per block the constructor-time fields, the
 * world-independent queries, every valid state and the meta/state-id mappings. Lives in net.minecraft.block to
 * read the protected fields. tests/BlockRegistryTest.cpp writes the same text from the port and compares lines.
 */
public class BlockDump {

    static String itemName(net.minecraft.item.Item item) {
        return item == null ? "null" : net.minecraft.item.Item.itemRegistry.getNameForObject(item).getResourcePath();
    }

    /**
     * The silk touch stack, then quantity and item for fortune 0 and 3 with a fresh Random per seed 0..3 (the
     * quantity is drawn first, as dropBlockAsItemWithChance does).
     */
    static String drops(Block block, IBlockState state) {
        net.minecraft.item.ItemStack silk = block.createStackedBlock(state);
        StringBuilder sb = new StringBuilder("silk=" + itemName(silk.getItem()) + ":" + silk.getItemDamage());
        for (int fortune = 0; fortune <= 3; fortune += 3) {
            sb.append(" f").append(fortune).append('=');
            for (long seed = 0; seed < 4; seed++) {
                java.util.Random rand = new java.util.Random(seed);
                int quantity = block.quantityDroppedWithBonus(fortune, rand);
                net.minecraft.item.Item item = block.getItemDropped(state, rand, fortune);
                sb.append(seed == 0 ? "" : ",").append(quantity).append('*').append(itemName(item));
            }
        }
        return sb.toString();
    }

    static Map<Material, String> materialNames() throws Exception {
        Map<Material, String> names = new IdentityHashMap<Material, String>();
        for (Field f : Material.class.getFields()) {
            if (Modifier.isStatic(f.getModifiers()) && f.getType() == Material.class) names.put((Material) f.get(null), f.getName());
        }
        return names;
    }

    static String f(float v) {
        return Float.toString(v);
    }

    public static void dump(PrintWriter out) throws Exception {
        Map<Material, String> materials = materialNames();
        for (Block block : Block.blockRegistry) {
            int id = Block.getIdFromBlock(block);
            out.println("block " + id + " " + Block.blockRegistry.getNameForObject(block) + " " + block.getClass().getSimpleName());
            out.println(" unlocalized=" + block.getUnlocalizedName() + " tab=" + (block.getCreativeTabToDisplayOn() == null ? "none" : block.getCreativeTabToDisplayOn().getTabLabel()));
            out.println(" material=" + materials.get(block.getMaterial()) + " hardness=" + f(block.blockHardness) + " resistance=" + f(block.blockResistance)
                + " light=" + block.lightValue + " opacity=" + block.lightOpacity + " translucent=" + block.translucent + " neighborBrightness=" + block.useNeighborBrightness
                + " fullBlock=" + block.fullBlock);
            out.println(" stats=" + block.enableStats + " randomTick=" + block.needsRandomTick + " container=" + block.isBlockContainer + " slipperiness=" + f(block.slipperiness)
                + " particleGravity=" + f(block.blockParticleGravity));
            out.println(" sound=" + block.stepSound.soundName + "," + f(block.stepSound.getVolume()) + "," + f(block.stepSound.getFrequency()) + ","
                + block.stepSound.getBreakSound() + "," + block.stepSound.getStepSound() + "," + block.stepSound.getPlaceSound());
            out.println(" bounds=" + block.minX + "," + block.minY + "," + block.minZ + "," + block.maxX + "," + block.maxY + "," + block.maxZ);
            out.println(" render=" + block.getRenderType() + " layer=" + block.getBlockLayer().name() + " opaque=" + block.isOpaqueCube() + " fullCube=" + block.isFullCube()
                + " normal=" + block.isNormalCube() + " blockNormal=" + block.isBlockNormalCube() + " visuallyOpaque=" + block.isVisuallyOpaque()
                + " power=" + block.canProvidePower() + " mobility=" + block.getMobilityFlag() + " offset=" + block.getOffsetType().name()
                + " collidable=" + block.isCollidable() + " updates=" + block.requiresUpdates() + " comparator=" + block.hasComparatorInputOverride()
                + " blockColor=" + block.getBlockColor() + " ao=" + f(block.getAmbientOcclusionLightValue()) + " spawnIn=" + block.canSpawnInBlock()
                + " silk=" + block.canSilkHarvest() + " explosion=" + f(block.getExplosionResistance(null)));
            StringBuilder props = new StringBuilder();
            for (IProperty p : block.getBlockState().getProperties()) {
                props.append(' ').append(p.getName()).append('=');
                boolean first = true;
                for (Object v : p.getAllowedValues()) {
                    props.append(first ? "" : ",").append(p.getName((Comparable) v));
                    first = false;
                }
            }
            out.println(" properties" + props + " default=" + block.getDefaultState());
            for (IBlockState state : block.getBlockState().getValidStates()) {
                out.println(" state " + state + " meta=" + block.getMetaFromState(state) + " id=" + Block.BLOCK_STATE_IDS.get(state) + " stateId=" + Block.getStateId(state)
                    + " map=" + block.getMapColor(state).colorIndex + " renderColor=" + block.getRenderColor(state) + " dropped=" + block.damageDropped(state));
                out.println(" drops " + drops(block, state));
            }
            for (int meta = 0; meta < 16; meta++) {
                String s;
                try {
                    s = String.valueOf(block.getStateFromMeta(meta));
                } catch (Exception e) {
                    s = "!" + e.getClass().getSimpleName();
                }
                out.println(" meta " + meta + " " + s);
            }
        }
        for (int i = 0; i < 4096 * 16; i++) {
            IBlockState state = (IBlockState) Block.BLOCK_STATE_IDS.getByValue(i);
            if (state != null) out.println("id " + i + " " + state);
        }
    }
}
