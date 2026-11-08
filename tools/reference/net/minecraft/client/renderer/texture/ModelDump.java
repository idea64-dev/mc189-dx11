package net.minecraft.client.renderer.texture;

import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import net.minecraft.block.Block;
import net.minecraft.block.state.IBlockState;
import net.minecraft.client.renderer.BlockModelShapes;
import net.minecraft.client.renderer.ItemMeshDefinition;
import net.minecraft.client.renderer.block.model.BakedQuad;
import net.minecraft.client.renderer.entity.RenderItem;
import net.minecraft.client.resources.DefaultResourcePack;
import net.minecraft.client.resources.IResource;
import net.minecraft.client.resources.IResourceManager;
import net.minecraft.client.resources.IResourcePack;
import net.minecraft.client.resources.SimpleReloadableResourceManager;
import net.minecraft.client.resources.data.AnimationMetadataSection;
import net.minecraft.client.resources.data.AnimationMetadataSectionSerializer;
import net.minecraft.client.resources.data.IMetadataSerializer;
import net.minecraft.client.resources.data.TextureMetadataSection;
import net.minecraft.client.resources.data.TextureMetadataSectionSerializer;
import net.minecraft.client.resources.model.IBakedModel;
import net.minecraft.client.resources.model.ModelManager;
import net.minecraft.client.resources.model.ModelResourceLocation;
import net.minecraft.client.resources.model.WeightedBakedModel;
import net.minecraft.item.Item;
import net.minecraft.util.EnumFacing;
import net.minecraft.util.MathHelper;
import net.minecraft.util.ResourceLocation;

/**
 * Bakes every block and item model of the vanilla client jar (on the classpath) the way ModelManager does at
 * startup, without a GL context: TextureMap.loadTextureAtlas is replaced by a copy that stitches and mipmaps but
 * writes the atlas into int arrays instead of uploading it. Writes the atlas (size, sprite placement, a hash of
 * every mip level), the state-to-variant mapping, a hash of every baked model and RenderItem's item model
 * registrations. tests/ModelBakeryTest.cpp builds the same text from the port.
 */
public class ModelDump {

    static int[][] atlas;
    static int atlasWidth;
    static int atlasHeight;
    static List<TextureAtlasSprite> stitched = new ArrayList<TextureAtlasSprite>();

    static Object field(Object o, String name) throws Exception {
        Field f = TextureMap.class.getDeclaredField(name);
        f.setAccessible(true);
        return f.get(o);
    }

    static void setField(Object o, String name, Object value) throws Exception {
        Field f = TextureMap.class.getDeclaredField(name);
        f.setAccessible(true);
        f.set(o, value);
    }

    /** TextureMap.loadTextureAtlas without GL (and with Minecraft.getGLMaximumTextureSize() = 16384). */
    static class CpuTextureMap extends TextureMap {
        CpuTextureMap() {
            super("textures");
        }

        @SuppressWarnings("unchecked")
        @Override
        public void loadTextureAtlas(IResourceManager resourceManager) {
            try {
                Map<String, TextureAtlasSprite> mapRegisteredSprites = (Map<String, TextureAtlasSprite>) field(this, "mapRegisteredSprites");
                Map<String, TextureAtlasSprite> mapUploadedSprites = (Map<String, TextureAtlasSprite>) field(this, "mapUploadedSprites");
                List<TextureAtlasSprite> listAnimatedSprites = (List<TextureAtlasSprite>) field(this, "listAnimatedSprites");
                TextureAtlasSprite missingImage = (TextureAtlasSprite) field(this, "missingImage");
                int mipmapLevels = (Integer) field(this, "mipmapLevels");
                Stitcher stitcher = new Stitcher(16384, 16384, true, 0, mipmapLevels);
                mapUploadedSprites.clear();
                listAnimatedSprites.clear();
                int j = Integer.MAX_VALUE;
                int k = 1 << mipmapLevels;
                for (Entry<String, TextureAtlasSprite> entry : mapRegisteredSprites.entrySet()) {
                    TextureAtlasSprite sprite = entry.getValue();
                    ResourceLocation location = new ResourceLocation(sprite.getIconName());
                    ResourceLocation file = new ResourceLocation(location.getResourceDomain(), "textures/" + location.getResourcePath() + ".png");
                    try {
                        IResource resource = resourceManager.getResource(file);
                        java.awt.image.BufferedImage[] images = new java.awt.image.BufferedImage[1 + mipmapLevels];
                        images[0] = TextureUtil.readBufferedImage(resource.getInputStream());
                        TextureMetadataSection texture = (TextureMetadataSection) resource.getMetadata("texture");
                        if (texture != null && !texture.getListMipmaps().isEmpty()) throw new RuntimeException("extra mip levels are not in vanilla");
                        AnimationMetadataSection animation = (AnimationMetadataSection) resource.getMetadata("animation");
                        sprite.loadSprite(images, animation);
                    } catch (RuntimeException e) {
                        continue;
                    } catch (java.io.IOException e) {
                        continue;
                    }
                    j = Math.min(j, Math.min(sprite.getIconWidth(), sprite.getIconHeight()));
                    int l1 = Math.min(Integer.lowestOneBit(sprite.getIconWidth()), Integer.lowestOneBit(sprite.getIconHeight()));
                    if (l1 < k) k = l1;
                    stitcher.addSprite(sprite);
                }
                int k1 = MathHelper.calculateLogBaseTwo(Math.min(j, k));
                if (k1 < mipmapLevels) {
                    mipmapLevels = k1;
                    setField(this, "mipmapLevels", k1);
                }
                for (TextureAtlasSprite sprite : mapRegisteredSprites.values()) sprite.generateMipmaps(mipmapLevels);
                missingImage.generateMipmaps(mipmapLevels);
                stitcher.addSprite(missingImage);
                stitcher.doStitch();
                atlasWidth = stitcher.getCurrentWidth();
                atlasHeight = stitcher.getCurrentHeight();
                atlas = new int[mipmapLevels + 1][];
                for (int level = 0; level <= mipmapLevels; level++) atlas[level] = new int[(atlasWidth >> level) * (atlasHeight >> level)];
                Map<String, TextureAtlasSprite> map = new HashMap<String, TextureAtlasSprite>(mapRegisteredSprites);
                for (TextureAtlasSprite sprite : stitcher.getStichSlots()) {
                    String s = sprite.getIconName();
                    map.remove(s);
                    mapUploadedSprites.put(s, sprite);
                    stitched.add(sprite);
                    int[][] frame = sprite.getFrameTextureData(0);
                    for (int level = 0; level < frame.length; level++) {
                        int w = sprite.getIconWidth() >> level;
                        int h = sprite.getIconHeight() >> level;
                        int x0 = sprite.getOriginX() >> level;
                        int y0 = sprite.getOriginY() >> level;
                        int aw = atlasWidth >> level;
                        for (int y = 0; y < h; y++) System.arraycopy(frame[level], y * w, atlas[level], (y0 + y) * aw + x0, w);
                    }
                    if (sprite.hasAnimationMetadata()) listAnimatedSprites.add(sprite);
                }
                for (TextureAtlasSprite sprite : map.values()) sprite.copyFrom(missingImage);
            } catch (RuntimeException e) {
                throw e;
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
    }

    /** Records RenderItem's registrations instead of resolving them. */
    static final List<String> registrations = new ArrayList<String>();

    static class RecordingRenderItem extends RenderItem {
        RecordingRenderItem(ModelManager manager) {
            super(null, manager);
        }

        @Override
        protected void registerItem(Item item, int subType, String identifier) {
            ResourceLocation name = item == null ? null : (ResourceLocation) Item.itemRegistry.getNameForObject(item);
            registrations.add("register " + (name == null ? "null" : name.toString()) + " " + subType + " " + new ModelResourceLocation(identifier, "inventory"));
        }
    }

    static int hash(int[] data) {
        return Arrays.hashCode(data);
    }

    static int quadHash(BakedQuad quad) {
        return 31 * (31 * hash(quad.getVertexData()) + quad.getTintIndex()) + quad.getFace().ordinal();
    }

    static String quads(List<BakedQuad> list, boolean sorted) {
        List<Integer> hashes = new ArrayList<Integer>();
        for (BakedQuad quad : list) hashes.add(quadHash(quad));
        if (sorted) Collections.sort(hashes);
        int h = 1;
        for (int q : hashes) h = 31 * h + q;
        return list.size() + ":" + h;
    }

    static String model(IBakedModel model, boolean sorted) {
        StringBuilder sb = new StringBuilder();
        sb.append(model.isAmbientOcclusion() ? "ao" : "-").append(model.isGui3d() ? " gui3d" : " -").append(model.isBuiltInRenderer() ? " builtin" : " -");
        sb.append(" particle=").append(model.getParticleTexture() == null ? "null" : model.getParticleTexture().getIconName());
        if (!model.isBuiltInRenderer()) {
            sb.append(" general=").append(quads(model.getGeneralQuads(), sorted));
            for (EnumFacing facing : EnumFacing.values()) sb.append(' ').append(facing.getName()).append('=').append(quads(model.getFaceQuads(facing), sorted));
        }
        return sb.toString();
    }

    @SuppressWarnings("unchecked")
    public static void dump(PrintWriter out) throws Exception {
        IMetadataSerializer serializer = new IMetadataSerializer();
        serializer.registerMetadataSectionType(new TextureMetadataSectionSerializer(), TextureMetadataSection.class);
        serializer.registerMetadataSectionType(new AnimationMetadataSectionSerializer(), AnimationMetadataSection.class);
        SimpleReloadableResourceManager resourceManager = new SimpleReloadableResourceManager(serializer);
        List<IResourcePack> packs = new ArrayList<IResourcePack>();
        packs.add(new DefaultResourcePack(new HashMap<String, java.io.File>()));
        resourceManager.reloadResources(packs);

        CpuTextureMap textureMap = new CpuTextureMap();
        textureMap.setMipmapLevels(4);
        ModelManager modelManager = new ModelManager(textureMap);
        modelManager.onResourceManagerReload(resourceManager);

        out.println("atlas " + atlasWidth + "x" + atlasHeight + " mipmaps " + (atlas.length - 1));
        for (int level = 0; level < atlas.length; level++) out.println("level " + level + " " + hash(atlas[level]));
        for (TextureAtlasSprite sprite : stitched) {
            out.println("sprite " + sprite.getIconName() + " " + sprite.getOriginX() + " " + sprite.getOriginY() + " " + sprite.getIconWidth() + "x"
                + sprite.getIconHeight() + (sprite.rotated ? " rotated" : "") + (sprite.hasAnimationMetadata() ? " animated " + sprite.getFrameCount() : "")
                + " uv " + Integer.toHexString(Float.floatToIntBits(sprite.getMinU())) + " " + Integer.toHexString(Float.floatToIntBits(sprite.getMaxU()))
                + " " + Integer.toHexString(Float.floatToIntBits(sprite.getMinV())) + " " + Integer.toHexString(Float.floatToIntBits(sprite.getMaxV())));
        }

        BlockModelShapes shapes = modelManager.getBlockModelShapes();
        Map<IBlockState, ModelResourceLocation> locations = shapes.getBlockStateMapper().putAllStateModelLocations();
        for (Block block : Block.blockRegistry) {
            for (IBlockState state : block.getBlockState().getValidStates()) {
                ModelResourceLocation location = locations.get(state);
                IBakedModel baked = shapes.getModelForState(state);
                out.println("state " + state + " " + (location == null ? "builtin" : location.toString())
                    + (baked == modelManager.getMissingModel() ? " missing" : "") + " particle=" + shapes.getTexture(state).getIconName());
            }
        }

        Field registryField = ModelManager.class.getDeclaredField("modelRegistry");
        registryField.setAccessible(true);
        net.minecraft.util.RegistrySimple<ModelResourceLocation, IBakedModel> registry = (net.minecraft.util.RegistrySimple<ModelResourceLocation, IBakedModel>) registryField.get(modelManager);
        List<String> keys = new ArrayList<String>();
        Map<String, ModelResourceLocation> byName = new HashMap<String, ModelResourceLocation>();
        for (ModelResourceLocation key : registry.getKeys()) {
            keys.add(key.toString());
            byName.put(key.toString(), key);
        }
        Collections.sort(keys);
        for (String key : keys) {
            IBakedModel baked = registry.getObject(byName.get(key));
            boolean item = byName.get(key).getVariant().equals("inventory");
            if (baked instanceof WeightedBakedModel) {
                Field modelsField = WeightedBakedModel.class.getDeclaredField("models");
                modelsField.setAccessible(true);
                List<?> items = (List<?>) modelsField.get(baked);
                Field weightField = net.minecraft.util.WeightedRandom.Item.class.getDeclaredField("itemWeight");
                weightField.setAccessible(true);
                Field modelField = items.get(0).getClass().getDeclaredField("model");
                modelField.setAccessible(true);
                out.println("model " + key + " weighted " + items.size());
                for (Object o : items) out.println("  weight " + weightField.get(o) + " " + model((IBakedModel) modelField.get(o), item));
            } else {
                out.println("model " + key + " " + model(baked, item));
            }
        }

        new RecordingRenderItem(modelManager);
        for (String line : registrations) out.println(line);
    }
}
