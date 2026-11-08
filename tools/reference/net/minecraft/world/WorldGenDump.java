package net.minecraft.world;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import net.minecraft.entity.Entity;
import net.minecraft.entity.EnumCreatureType;
import net.minecraft.profiler.Profiler;
import net.minecraft.util.BlockPos;
import net.minecraft.util.IProgressUpdate;
import net.minecraft.world.biome.BiomeGenBase;
import net.minecraft.world.chunk.Chunk;
import net.minecraft.world.chunk.IChunkProvider;
import net.minecraft.world.storage.MapStorage;
import net.minecraft.world.storage.WorldInfo;

/**
 * World generation against the original: for a list of (seed, world type, generator options, dimension) cases, a
 * server-less world with a ChunkProviderServer-like cache loads the chunks around the origin in the order of
 * MinecraftServer.initialWorldChunkLoad (a 9x9 square instead of 25x25), which generates and populates them and
 * their neighbours; every populated chunk is then hashed. tests/WorldGenTest.cpp runs the same cases.
 *
 * Differences from a WorldServer that don't change the blocks: scheduled block ticks are dropped (the base World
 * does nothing; populating never runs them either), block events run at once, entities spawned by the
 * structures and the animal spawner are discarded, and world.rand is seeded so both sides draw the same.
 */
public class WorldGenDump {

    static final long WORLD_RAND_SEED = 0x5DEECE66DL;

    static class Case {
        final String name;
        final long seed;
        final WorldType type;
        final String options;
        final int dimension;

        Case(String name, long seed, WorldType type, String options, int dimension) {
            this.name = name;
            this.seed = seed;
            this.type = type;
            this.options = options;
            this.dimension = dimension;
        }
    }

    static final String CUSTOMIZED = "{\"coordinateScale\":738.41864, \"heightScale\":157.69133, \"upperLimitScale\":801.4267, \"lowerLimitScale\":1254.1643, \"depthNoiseScaleX\":374.93652, \"depthNoiseScaleZ\":288.65228, \"depthNoiseScaleExponent\":1.2092624, \"mainNoiseScaleX\":1355.9908, \"mainNoiseScaleY\":745.5343, \"mainNoiseScaleZ\":1183.464, \"baseSize\":1.8758626, \"stretchY\":1.7137525, \"biomeDepthWeight\":1.7553768, \"biomeDepthOffset\":3.4701107, \"biomeScaleWeight\":1.0, \"biomeScaleOffset\":2.535211, \"useCaves\":true, \"useDungeons\":true, \"dungeonChance\":8, \"useStrongholds\":true, \"useVillages\":true, \"useMineShafts\":true, \"useTemples\":true, \"useRavines\":true, \"useWaterLakes\":true, \"waterLakeChance\":4, \"useLavaLakes\":true, \"lavaLakeChance\":80, \"useLavaOceans\":false, \"seaLevel\":63 }";
    static final String FLAT_CUSTOM = "3;minecraft:bedrock,2*minecraft:dirt,minecraft:grass;1;village,decoration,lake";

    // Keep in step with the list in tests/WorldGenTest.cpp.
    static List<Case> cases() {
        List<Case> list = new ArrayList<Case>();
        list.add(new Case("default-0", 0L, WorldType.DEFAULT, "", 0));
        list.add(new Case("default-12345", 12345L, WorldType.DEFAULT, "", 0));
        list.add(new Case("default-big", -6017234389458431232L, WorldType.DEFAULT, "", 0));
        list.add(new Case("default-text", (long) "MCP-919".hashCode(), WorldType.DEFAULT, "", 0));
        list.add(new Case("flat-default", 12345L, WorldType.FLAT, "", 0));
        list.add(new Case("flat-custom", 12345L, WorldType.FLAT, FLAT_CUSTOM, 0));
        list.add(new Case("large-biomes", 12345L, WorldType.LARGE_BIOMES, "", 0));
        list.add(new Case("amplified", 12345L, WorldType.AMPLIFIED, "", 0));
        list.add(new Case("customized", 12345L, WorldType.CUSTOMIZED, CUSTOMIZED, 0));
        list.add(new Case("debug", 12345L, WorldType.DEBUG_WORLD, "", 0));
        list.add(new Case("nether", 12345L, WorldType.DEFAULT, "", -1));
        list.add(new Case("end", 12345L, WorldType.DEFAULT, "", 1));
        return list;
    }

    static class GenWorld extends World {
        final Map<Long, Chunk> chunks = new HashMap<Long, Chunk>();
        final List<Chunk> loaded = new ArrayList<Chunk>();
        IChunkProvider generator;

        GenWorld(WorldInfo info, int dimension) {
            super(null, info, WorldProvider.getProviderForDimension(dimension), new Profiler(), false);
            this.provider.registerWorld(this);
            this.mapStorage = new MapStorage(null);
            this.generator = this.provider.createChunkGenerator();
            this.chunkProvider = this.createChunkProvider();
            this.calculateInitialSkylight();
            this.rand.setSeed(WORLD_RAND_SEED);
        }

        // Java: ChunkProviderServer.loadChunk with no chunk loader.
        Chunk loadChunk(int x, int z) {
            long key = ChunkCoordIntPair.chunkXZ2Int(x, z);
            Chunk chunk = chunks.get(key);
            if (chunk == null) {
                chunk = generator.provideChunk(x, z);
                chunks.put(key, chunk);
                loaded.add(chunk);
                chunk.onChunkLoad();
                chunk.populateChunk(this.chunkProvider, this.chunkProvider, x, z);
            }
            return chunk;
        }

        protected IChunkProvider createChunkProvider() {
            return new IChunkProvider() {
                public boolean chunkExists(int x, int z) { return chunks.containsKey(ChunkCoordIntPair.chunkXZ2Int(x, z)); }
                public Chunk provideChunk(int x, int z) {
                    Chunk c = chunks.get(ChunkCoordIntPair.chunkXZ2Int(x, z));
                    return c != null ? c : loadChunk(x, z);
                }
                public Chunk provideChunk(BlockPos pos) { return provideChunk(pos.getX() >> 4, pos.getZ() >> 4); }
                public void populate(IChunkProvider p, int x, int z) {
                    Chunk chunk = provideChunk(x, z);
                    if (!chunk.isTerrainPopulated()) {
                        chunk.func_150809_p();
                        generator.populate(p, x, z);
                        chunk.setChunkModified();
                    }
                }
                public boolean populateChunk(IChunkProvider p, Chunk c, int x, int z) {
                    if (generator.populateChunk(p, c, x, z)) {
                        provideChunk(x, z).setChunkModified();
                        return true;
                    }
                    return false;
                }
                public boolean saveChunks(boolean all, IProgressUpdate u) { return true; }
                public boolean unloadQueuedChunks() { return false; }
                public boolean canSave() { return false; }
                public String makeString() { return "Gen"; }
                public List<BiomeGenBase.SpawnListEntry> getPossibleCreatures(EnumCreatureType t, BlockPos pos) { return generator.getPossibleCreatures(t, pos); }
                public BlockPos getStrongholdGen(World w, String name, BlockPos pos) { return generator.getStrongholdGen(w, name, pos); }
                public int getLoadedChunkCount() { return chunks.size(); }
                public void recreateStructures(Chunk c, int x, int z) { }
                public void saveExtraData() { }
            };
        }

        public boolean spawnEntityInWorld(Entity entityIn) { return false; }

        protected int getRenderDistanceChunks() { return 8; }
    }

    public static void dump(PrintWriter out) throws Exception {
        for (Case c : cases()) {
            WorldInfo info = new WorldInfo(new WorldSettings(c.seed, WorldSettings.GameType.SURVIVAL, true, false, c.type).setWorldName(c.options), "Gen");
            GenWorld world = new GenWorld(info, c.dimension);
            for (int l1 = -64; l1 <= 64; l1 += 16) {
                for (int i2 = -64; i2 <= 64; i2 += 16) {
                    world.loadChunk(l1 >> 4, i2 >> 4);
                }
            }
            List<Chunk> populated = new ArrayList<Chunk>();
            for (Chunk chunk : world.loaded) {
                if (chunk.isTerrainPopulated()) populated.add(chunk);
            }
            java.util.Collections.sort(populated, new java.util.Comparator<Chunk>() {
                public int compare(Chunk a, Chunk b) {
                    return a.xPosition != b.xPosition ? Integer.compare(a.xPosition, b.xPosition) : Integer.compare(a.zPosition, b.zPosition);
                }
            });
            out.println("case " + c.name + " " + world.loaded.size() + " " + populated.size());
            for (Chunk chunk : populated) {
                out.println(chunk.xPosition + " " + chunk.zPosition + " " + WorldDump.hash(chunk));
            }
        }
    }
}
