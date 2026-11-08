package net.minecraft.world;
import java.io.*;
import net.minecraft.world.chunk.*;
import net.minecraft.world.chunk.storage.*;
import net.minecraft.world.storage.*;
/**
 * Chunk hashes of a save written by the vanilla client, for tests/VanillaSaveTest.cpp (which skips without them):
 *   java net.minecraft.world.SaveDump "run/cmp/vanilla/game/saves/New World" run/cmp/vanilla_save_chunks.txt
 * with the classpath generate_game.ps1 uses.
 */
public class SaveDump {
    public static void main(String[] a) throws Exception {
        net.minecraft.init.Bootstrap.register();
        File dir = new File(a[0]);
        AnvilSaveConverter format = new AnvilSaveConverter(dir.getParentFile());
        WorldInfo info = format.getWorldInfo(dir.getName());
        WorldDump.RefWorld world = new WorldDump.RefWorld(info);
        AnvilChunkLoader loader = new AnvilChunkLoader(dir);
        PrintWriter out = new PrintWriter(new FileWriter(a[1]));
        out.println("seed " + info.getSeed() + " name " + info.getWorldName() + " spawn " + info.getSpawnX() + " " + info.getSpawnY() + " " + info.getSpawnZ());
        for (int cx = -32; cx < 32; ++cx) for (int cz = -32; cz < 32; ++cz) {
            Chunk c = loader.loadChunk(world, cx, cz);
            if (c != null) out.println("chunk " + cx + " " + cz + " " + WorldDump.hash(c) + " " + c.getInhabitedTime());
        }
        out.close();
    }
}
