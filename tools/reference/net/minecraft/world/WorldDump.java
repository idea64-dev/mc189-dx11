package net.minecraft.world;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import net.minecraft.block.state.IBlockState;
import net.minecraft.entity.EnumCreatureType;
import net.minecraft.init.Blocks;
import net.minecraft.nbt.CompressedStreamTools;
import net.minecraft.nbt.NBTTagCompound;
import net.minecraft.profiler.Profiler;
import net.minecraft.util.BlockPos;
import net.minecraft.util.IProgressUpdate;
import net.minecraft.world.biome.BiomeGenBase;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.ChunkPrimer;
import net.minecraft.world.chunk.EmptyChunk;
import net.minecraft.world.chunk.IChunkProvider;
import net.minecraft.world.chunk.storage.AnvilChunkLoader;
import net.minecraft.world.chunk.storage.ExtendedBlockStorage;
import net.minecraft.world.storage.WorldInfo;

/**
 * The chunk storage and light engine against the original: a client-side world (no block callbacks) of
 * synthetic chunks, built and edited by a fixed recipe, with hashes of every chunk after each step and samples of
 * the light queries. tests/WorldLightTest.cpp follows the same recipe.
 */
public class WorldDump {

    static IBlockState[] palette() {
        return new IBlockState[] {Blocks.air.getDefaultState(), Blocks.stone.getDefaultState(), Blocks.dirt.getDefaultState(), Blocks.glass.getDefaultState(),
            Blocks.leaves.getDefaultState(), Blocks.water.getDefaultState(), Blocks.glowstone.getDefaultState(), Blocks.torch.getDefaultState(),
            Blocks.ice.getDefaultState(), Blocks.stone_slab.getDefaultState(), Blocks.oak_stairs.getDefaultState(), Blocks.lava.getDefaultState(),
            Blocks.lit_pumpkin.getDefaultState(), Blocks.web.getDefaultState(), Blocks.farmland.getDefaultState(), Blocks.redstone_torch.getDefaultState()};
    }

    static ChunkPrimer primer(int cx, int cz, IBlockState[] pal) {
        Random r = new Random(cx * 341873128712L + cz * 132897987541L + 12345L);
        ChunkPrimer p = new ChunkPrimer();
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                int h = 40 + r.nextInt(24);
                for (int y = 0; y < h; ++y) {
                    int v = r.nextInt(100);
                    int idx = v < 70 ? 1 : v < 75 ? 0 : v < 78 ? 3 : v < 81 ? 4 : v < 84 ? 5 : v < 85 ? 6 : v < 86 ? 7 : v < 88 ? 8 : v < 90 ? 9 : v < 92 ? 10
                        : v < 93 ? 11 : v < 94 ? 12 : v < 95 ? 13 : v < 97 ? 2 : v < 98 ? 14 : 15;
                    p.setBlockState(x, y, z, pal[idx]);
                }
                if (r.nextInt(8) == 0) {
                    p.setBlockState(x, h + 5 + r.nextInt(20), z, pal[r.nextInt(pal.length)]);
                }
            }
        }
        return p;
    }

    static int hash(Chunk chunk) {
        int h = 0;
        for (int v : chunk.getHeightMap()) h = h * 31 + v;
        h = h * 31 + chunk.getLowestHeight();
        h = h * 31 + (chunk.isLightPopulated() ? 1 : 0);
        h = h * 31 + (chunk.isTerrainPopulated() ? 1 : 0);
        for (ExtendedBlockStorage s : chunk.getBlockStorageArray()) {
            if (s == null) {
                h = h * 31 - 1;
                continue;
            }
            for (char c : s.getData()) h = h * 31 + c;
            for (byte b : s.getBlocklightArray().getData()) h = h * 31 + (b & 255);
            for (byte b : s.getSkylightArray().getData()) h = h * 31 + (b & 255);
            h = h * 31 + (s.isEmpty() ? 1 : 0);
            h = h * 31 + (s.getNeedsRandomTick() ? 1 : 0);
        }
        return h;
    }

    static class RefWorld extends World {
        final Map<Long, Chunk> chunks = new HashMap<Long, Chunk>();
        Chunk blank;

        RefWorld(WorldInfo info) {
            super(null, info, WorldProvider.getProviderForDimension(0), new Profiler(), true);
            this.provider.registerWorld(this);
            this.chunkProvider = this.createChunkProvider();
            this.calculateInitialSkylight();
        }

        protected IChunkProvider createChunkProvider() {
            blank = new EmptyChunk(this, 0, 0);
            return new IChunkProvider() {
                public boolean chunkExists(int x, int z) { return chunks.containsKey(ChunkCoordIntPair.chunkXZ2Int(x, z)); }
                public Chunk provideChunk(int x, int z) {
                    Chunk c = chunks.get(ChunkCoordIntPair.chunkXZ2Int(x, z));
                    return c != null ? c : blank;
                }
                public Chunk provideChunk(BlockPos pos) { return provideChunk(pos.getX() >> 4, pos.getZ() >> 4); }
                public void populate(IChunkProvider p, int x, int z) { }
                public boolean populateChunk(IChunkProvider p, Chunk c, int x, int z) { return false; }
                public boolean saveChunks(boolean all, IProgressUpdate u) { return true; }
                public boolean unloadQueuedChunks() { return false; }
                public boolean canSave() { return false; }
                public String makeString() { return "Ref"; }
                public List<BiomeGenBase.SpawnListEntry> getPossibleCreatures(EnumCreatureType t, BlockPos pos) { return null; }
                public BlockPos getStrongholdGen(World w, String name, BlockPos pos) { return null; }
                public int getLoadedChunkCount() { return chunks.size(); }
                public void recreateStructures(Chunk c, int x, int z) { }
                public void saveExtraData() { }
            };
        }

        protected int getRenderDistanceChunks() { return 8; }
    }

    static void dumpChunks(PrintWriter out, RefWorld world, String step) {
        StringBuilder sb = new StringBuilder("step " + step);
        for (int cx = -3; cx <= 3; ++cx) {
            for (int cz = -3; cz <= 3; ++cz) {
                sb.append(' ').append(hash(world.getChunkFromChunkCoords(cx, cz)));
            }
        }
        out.println(sb);
    }

    public static void dump(PrintWriter out) throws Exception {
        WorldInfo info = new WorldInfo(new WorldSettings(12345L, WorldSettings.GameType.SURVIVAL, true, false, WorldType.DEFAULT).enableCommands(), "Ref");
        RefWorld world = new RefWorld(info);
        IBlockState[] pal = palette();
        out.println("skylightSubtracted " + world.getSkylightSubtracted());

        for (int cx = -3; cx <= 3; ++cx) {
            for (int cz = -3; cz <= 3; ++cz) {
                Chunk chunk = new Chunk(world, primer(cx, cz, pal), cx, cz);
                chunk.generateSkylightMap();
                world.chunks.put(ChunkCoordIntPair.chunkXZ2Int(cx, cz), chunk);
            }
        }
        dumpChunks(out, world, "generated");

        for (int cx = -1; cx <= 1; ++cx) {
            for (int cz = -1; cz <= 1; ++cz) {
                world.getChunkFromChunkCoords(cx, cz).func_150809_p();
            }
        }
        dumpChunks(out, world, "lightPopulated");

        Random ops = new Random(4242L);
        StringBuilder results = new StringBuilder("set");
        for (int i = 0; i < 300; ++i) {
            BlockPos pos = new BlockPos(ops.nextInt(48) - 24, ops.nextInt(80) + 10, ops.nextInt(48) - 24);
            IBlockState state = pal[ops.nextInt(pal.length)];
            results.append(world.setBlockState(pos, state, 2) ? '1' : '0');
        }
        out.println(results);
        dumpChunks(out, world, "edited");

        Chunk center = world.getChunkFromChunkCoords(0, 0);
        center.resetRelightChecks();
        for (int i = 0; i < 600; ++i) center.enqueueRelightChecks();
        for (int cx = -1; cx <= 1; ++cx) {
            for (int cz = -1; cz <= 1; ++cz) {
                world.getChunkFromChunkCoords(cx, cz).func_150804_b(false);
            }
        }
        dumpChunks(out, world, "rechecked");

        Random samples = new Random(777L);
        for (int i = 0; i < 200; ++i) {
            BlockPos pos = new BlockPos(samples.nextInt(64) - 32, samples.nextInt(110), samples.nextInt(64) - 32);
            out.println("sample " + pos.getX() + " " + pos.getY() + " " + pos.getZ() + " light=" + world.getLight(pos) + " neighbors=" + world.getLightFromNeighbors(pos)
                + " combined=" + world.getCombinedLight(pos, 0) + " sky=" + world.getLightFor(EnumSkyBlock.SKY, pos) + " block=" + world.getLightFor(EnumSkyBlock.BLOCK, pos)
                + " seesSky=" + world.canSeeSky(pos) + " blockSeesSky=" + world.canBlockSeeSky(pos) + " brightness=" + world.getLightBrightness(pos)
                + " precipitation=" + world.getPrecipitationHeight(pos).getY() + " top=" + world.getTopSolidOrLiquidBlock(pos).getY() + " height=" + world.getHeight(pos).getY()
                + " lowest=" + world.getChunksLowestHorizon(pos.getX(), pos.getZ()));
        }

        Method write = AnvilChunkLoader.class.getDeclaredMethod("writeChunkToNBT", Chunk.class, World.class, NBTTagCompound.class);
        write.setAccessible(true);
        for (int c = 0; c < 2; ++c) {
            NBTTagCompound level = new NBTTagCompound();
            write.invoke(new AnvilChunkLoader(null), world.getChunkFromChunkCoords(c, -c), world, level);
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            CompressedStreamTools.write(level, new DataOutputStream(bytes));
            int h = 0;
            for (byte b : bytes.toByteArray()) h = h * 31 + b;
            out.println("chunkNBT " + c + " " + bytes.size() + " " + h);
        }

        NBTTagCompound levelDat = info.getNBTTagCompound();
        levelDat.removeTag("LastPlayed");
        out.println("levelDat " + levelDat);
    }
}
