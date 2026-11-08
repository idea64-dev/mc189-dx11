package net.minecraft.item;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import net.minecraft.block.Block;

/**
 * Text dump of the item registry as the original builds it: per item the constructor-time fields, the
 * world-independent queries and every creative sub-item. Lives in net.minecraft.item to read protected fields.
 * tests/ItemRegistryTest.cpp writes the same text from the port and compares lines.
 */
public class ItemDump {

    static String f(float v) {
        return Float.toString(v);
    }

    /** Private fields of ItemFood/ItemTool have no getters; read them reflectively. */
    static Object field(Object obj, Class<?> cls, String name) {
        try {
            java.lang.reflect.Field fld = cls.getDeclaredField(name);
            fld.setAccessible(true);
            Object v = fld.get(obj);
            return v instanceof Float ? f((Float) v) : v;
        } catch (ReflectiveOperationException e) {
            throw new RuntimeException(e);
        }
    }

    static String itemName(Item item) {
        return item == null ? "null" : String.valueOf(Item.itemRegistry.getNameForObject(item));
    }

    public static void dump(PrintWriter out) {
        for (Item item : Item.itemRegistry) {
            int id = Item.getIdFromItem(item);
            ItemStack stack = new ItemStack(item, 1, 0);
            out.println("item " + id + " " + itemName(item) + " " + item.getClass().getSimpleName());
            out.println(" unlocalized=" + item.getUnlocalizedName() + " tab=" + (item.getCreativeTab() == null ? "none" : item.getCreativeTab().getTabLabel())
                + " stack=" + item.getItemStackLimit() + " damage=" + item.getMaxDamage() + " subtypes=" + item.getHasSubtypes() + " full3d=" + item.isFull3D()
                + " rotate=" + item.shouldRotateAroundWhenRendering() + " container=" + itemName(item.getContainerItem()) + " enchantability=" + item.getItemEnchantability()
                + " map=" + item.isMap() + " shareTag=" + item.getShareTag() + " damageable=" + item.isDamageable() + " tool=" + item.isItemTool(stack)
                + " editBlocks=" + item.canItemEditBlocks());
            out.println(" use=" + item.getItemUseAction(stack).name() + " duration=" + item.getMaxItemUseDuration(stack) + " rarity=" + item.getRarity(stack).name()
                + " effect=" + item.hasEffect(stack) + " potion=" + item.getPotionEffect(stack));
            if (item instanceof ItemBlock) {
                out.println(" block=" + Block.blockRegistry.getNameForObject(((ItemBlock) item).getBlock()));
            }
            if (item instanceof ItemFood) {
                ItemFood food = (ItemFood) item;
                out.println(" food heal=" + food.getHealAmount(stack) + " saturation=" + f(food.getSaturationModifier(stack)) + " wolf=" + food.isWolfsFavoriteMeat()
                    + " always=" + field(food, ItemFood.class, "alwaysEdible") + " potionId=" + field(food, ItemFood.class, "potionId")
                    + " potionDuration=" + field(food, ItemFood.class, "potionDuration") + " potionAmplifier=" + field(food, ItemFood.class, "potionAmplifier")
                    + " potionChance=" + field(food, ItemFood.class, "potionEffectProbability"));
            }
            if (item instanceof ItemArmor) {
                ItemArmor armor = (ItemArmor) item;
                out.println(" armor type=" + armor.armorType + " reduce=" + armor.damageReduceAmount + " renderIndex=" + armor.renderIndex + " material=" + armor.getArmorMaterial().name()
                    + " durability=" + armor.getArmorMaterial().getDurability(armor.armorType) + " enchantability=" + armor.getArmorMaterial().getEnchantability());
            }
            if (item instanceof ItemTool) {
                ItemTool tool = (ItemTool) item;
                out.println(" tool efficiency=" + f(tool.efficiencyOnProperMaterial) + " damageVsEntity=" + field(tool, ItemTool.class, "damageVsEntity") + " material=" + tool.getToolMaterial().name());
            }
            if (item instanceof ItemSword) {
                out.println(" sword damage=" + f(((ItemSword) item).getDamageVsEntity()) + " material=" + ((ItemSword) item).getToolMaterialName());
            }
            if (item instanceof ItemHoe) {
                out.println(" hoe material=" + ((ItemHoe) item).getMaterialName());
            }
            StringBuilder speeds = new StringBuilder();
            StringBuilder harvest = new StringBuilder();
            for (Block block : Block.blockRegistry) {
                float speed = item.getStrVsBlock(stack, block);
                if (speed != 1.0F) speeds.append(' ').append(Block.blockRegistry.getNameForObject(block).getResourcePath()).append('=').append(f(speed));
                if (item.canHarvestBlock(block)) harvest.append(' ').append(Block.blockRegistry.getNameForObject(block).getResourcePath());
            }
            if (speeds.length() > 0) out.println(" speeds" + speeds);
            if (harvest.length() > 0) out.println(" harvest" + harvest);
            if (item.getCreativeTab() != null) {
                List<ItemStack> subs = new ArrayList<ItemStack>();
                item.getSubItems(item, item.getCreativeTab(), subs);
                for (ItemStack sub : subs) {
                    out.println(" sub damage=" + sub.getItemDamage() + " meta=" + sub.getMetadata() + " count=" + sub.stackSize + " name=" + sub.getUnlocalizedName()
                        + " color0=" + sub.getItem().getColorFromItemStack(sub, 0) + " color1=" + sub.getItem().getColorFromItemStack(sub, 1)
                        + " effect=" + sub.hasEffect() + " rarity=" + sub.getRarity().name() + " tag=" + sub.getTagCompound());
                }
            }
        }
        for (Block block : Block.blockRegistry) {
            Item item = Item.getItemFromBlock(block);
            out.println("blockitem " + Block.blockRegistry.getNameForObject(block) + " " + itemName(item));
        }
    }
}
