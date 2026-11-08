import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import net.minecraft.nbt.CompressedStreamTools;
import net.minecraft.nbt.JsonToNBT;
import net.minecraft.nbt.NBTException;
import net.minecraft.nbt.NBTTagCompound;
import net.minecraft.util.AxisAlignedBB;
import net.minecraft.util.BlockPos;
import net.minecraft.util.EnumFacing;
import net.minecraft.util.MovingObjectPosition;
import net.minecraft.util.Vec3;

/**
 * Reference data computed by the real game code (MCP source compiled by compile_mcp.ps1), for tests that compare
 * the port with the original:
 *
 *   tests/reference/NbtReference.inc   JsonToNBT literals -> toString(), hashCode() and the written bytes
 *   tests/reference/MathReference.inc  BlockPos, EnumFacing, Vec3 and AxisAlignedBB results for random inputs
 *   tests/reference/blocks.txt         the block registry (net/minecraft/block/BlockDump.java)
 *   tests/reference/items.txt          the item registry (net/minecraft/item/ItemDump.java)
 *
 * Run by generate_game.ps1. Everything is hex-encoded UTF-8 so any text survives the trip into C++.
 */
public class GenerateGameReference {

    static String hex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) sb.append(String.format("%02x", b & 0xFF));
        return sb.toString();
    }

    static String hex(String text) {
        return hex(text.getBytes(StandardCharsets.UTF_8));
    }

    static final String[] NBT_CASES = {
        "{}",
        "{a:1}",
        "{Count:1b,Damage:0s,id:\"minecraft:stone\"}",
        "{display:{Name:\"Fancy \\\"Sword\\\"\",Lore:[\"line one\",\"line two\"]},ench:[{id:16s,lvl:5s},{id:20s,lvl:2s}]}",
        "{f:1.5f,d:2.25d,l:123456789012L,s:-5s,b:127b,i:-2147483648,u:0.5,t:true,F:FALSE}",
        "{arr:[1,2,3,-4],empty:[],nested:[[1,2],[3]],words:[a,b,c]}",
        "{big:300b,huge:99999999999,longOverflow:99999999999999999999L,weird:1|,x:+5,y:-.5,z:.5d}",
        "{text:\"with: colon, and {braces}\",esc:\"back\\\\\\\\slash\",uni:\"\u00e9\u4e2d\ud83d\ude00\"}",
        "{  spaced  :  \"value\"  ,  other : 7 }",
        "{dup:1,dup:2,dup:3}",
        "{list:[{a:1},{b:2}],mixed:[1,\"two\"]}",
        "{a:1}{b:2}",
        "[1,2]",
        "{a:\"unterminated}",
        "{a:{b:1}",
        "{a:1,,b:2}",
        "{:1}",
        "{a b:1}",
        "{0:a,1:b,2:c,3:d,4:e,5:f,6:g,7:h,8:i,9:j,10:k,11:l,12:m,13:n,14:o,15:p,16:q}",
        "{Aa:1,BB:2,AaAa:3,BBBB:4,AaBB:5,BBAa:6}",
    };

    // Random literals with many keys (to grow the HashMap through several resizes) and every value type.
    static String randomValue(Random r, int depth) {
        switch (r.nextInt(depth > 2 ? 9 : 11)) {
            case 0: return r.nextInt(256) - 128 + "b";
            case 1: return r.nextInt(65536) - 32768 + "s";
            case 2: return Integer.toString(r.nextInt());
            case 3: return r.nextLong() + "L";
            case 4: return r.nextFloat() * 1000 + "f";
            case 5: return r.nextGaussian() * 1e5 + "d";
            case 6: return "\"" + randomKey(r) + " " + randomKey(r) + "\"";
            case 7: {
                StringBuilder sb = new StringBuilder("[");
                int n = r.nextInt(5);
                for (int i = 0; i < n; i++) sb.append(i > 0 ? "," : "").append(r.nextInt(1000) - 500);
                return sb.append("]").toString();
            }
            case 8: return Integer.toString(r.nextInt(100));
            case 9: return randomCompound(r, depth + 1, 1 + r.nextInt(6));
            default: {
                StringBuilder sb = new StringBuilder("[");
                int n = 1 + r.nextInt(3);
                for (int i = 0; i < n; i++) sb.append(i > 0 ? "," : "").append(randomCompound(r, depth + 1, 1 + r.nextInt(3)));
                return sb.append("]").toString();
            }
        }
    }

    static String randomKey(Random r) {
        String letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
        StringBuilder sb = new StringBuilder();
        int n = 1 + r.nextInt(8);
        for (int i = 0; i < n; i++) sb.append(letters.charAt(r.nextInt(letters.length())));
        return sb.toString();
    }

    static String randomCompound(Random r, int depth, int keys) {
        StringBuilder sb = new StringBuilder("{");
        for (int i = 0; i < keys; i++) sb.append(i > 0 ? "," : "").append(randomKey(r)).append(':').append(randomValue(r, depth));
        return sb.append("}").toString();
    }

    static void nbt(PrintWriter out) throws Exception {
        List<String> inputs = new ArrayList<String>();
        for (String s : NBT_CASES) inputs.add(s);
        Random r = new Random(1189L);
        for (int i = 0; i < 40; i++) inputs.add(randomCompound(r, 0, 1 + r.nextInt(i < 30 ? 12 : 120)));

        out.println("// Generated by tools/reference/GenerateGameReference.java from the MCP 1.8.9 source. Do not edit.");
        out.println("// { JsonToNBT input, toString() or \"!\" + NBTException message, hashCode(), CompressedStreamTools.write bytes }, hex UTF-8.");
        out.println("struct NbtCase { const char* input; const char* text; int hash; const char* bytes; };");
        out.println("inline constexpr NbtCase nbtCases[] = {");
        for (String s : inputs) {
            String text;
            int hash = 0;
            String bytes = "";
            try {
                NBTTagCompound tag = JsonToNBT.getTagFromJson(s);
                text = tag.toString();
                hash = tag.hashCode();
                ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                DataOutputStream data = new DataOutputStream(buffer);
                CompressedStreamTools.write(tag, data);
                data.close();
                bytes = hex(buffer.toByteArray());
            } catch (NBTException e) {
                text = "!" + e.getMessage();
            }
            out.println("    { \"" + hex(s) + "\", \"" + hex(text) + "\", " + hash + ", \"" + bytes + "\" },");
        }
        out.println("};");
    }


    static String d(double v) {
        return "0x" + Long.toHexString(Double.doubleToRawLongBits(v)) + "ULL";
    }

    static void math(PrintWriter out) {
        Random r = new Random(1189L);
        out.println("// Generated by tools/reference/GenerateGameReference.java from the MCP 1.8.9 source. Do not edit.");
        out.println("// Doubles as raw IEEE bits.");

        // BlockPos packing and hashing, including coordinates outside the world.
        out.println("struct BlockPosCase { int x, y, z; long long packed; int hash; int ux, uy, uz; };");
        out.println("inline constexpr BlockPosCase blockPosCases[] = {");
        int[][] fixed = { {0, 0, 0}, {-1, -1, -1}, {30000000, 255, -30000000}, {-30000000, 0, 30000000}, {33554431, 2047, -33554432}, {Integer.MAX_VALUE, Integer.MIN_VALUE, 12345} };
        List<int[]> inputs = new ArrayList<int[]>();
        for (int[] f : fixed) inputs.add(f);
        for (int i = 0; i < 40; i++) inputs.add(new int[] { r.nextInt(), r.nextInt(512) - 128, r.nextInt(60000001) - 30000000 });
        for (int[] c : inputs) {
            BlockPos pos = new BlockPos(c[0], c[1], c[2]);
            long packed = pos.toLong();
            BlockPos back = BlockPos.fromLong(packed);
            out.println("    { " + c[0] + ", " + c[1] + ", " + c[2] + ", " + packed + "LL, " + pos.hashCode() + ", " + back.getX() + ", " + back.getY() + ", " + back.getZ() + " },");
        }
        out.println("};");

        // EnumFacing helpers.
        out.println("struct FacingAngleCase { unsigned long long angle; int facing; };");
        out.println("inline constexpr FacingAngleCase facingAngleCases[] = {");
        for (int i = 0; i < 40; i++) {
            double angle = i < 8 ? i * 45.0 - 180.0 + (i % 2 == 0 ? 0.0 : 0.25) : (r.nextDouble() - 0.5) * 2000.0;
            out.println("    { " + d(angle) + ", " + EnumFacing.fromAngle(angle).ordinal() + " },");
        }
        out.println("};");
        out.println("struct FacingVectorCase { float x, y, z; int facing; };");
        out.println("inline constexpr FacingVectorCase facingVectorCases[] = {");
        for (int i = 0; i < 40; i++) {
            float x = i == 0 ? 0.0F : r.nextFloat() * 2.0F - 1.0F;
            float y = i == 0 ? 0.0F : r.nextFloat() * 2.0F - 1.0F;
            float z = i == 0 ? 0.0F : r.nextFloat() * 2.0F - 1.0F;
            out.println("    { " + Float.toString(x) + "f, " + Float.toString(y) + "f, " + Float.toString(z) + "f, " + EnumFacing.getFacingFromVector(x, y, z).ordinal() + " },");
        }
        out.println("};");

        // Vec3 operations.
        out.println("struct Vec3Case { unsigned long long x, y, z; float angle; unsigned long long nx, ny, nz, px, py, pz, yx, yy, yz, len; };");
        out.println("inline constexpr Vec3Case vec3Cases[] = {");
        for (int i = 0; i < 40; i++) {
            Vec3 v = new Vec3(r.nextGaussian() * 10.0, r.nextGaussian() * 10.0, i == 3 ? -0.0 : r.nextGaussian() * 10.0);
            float angle = r.nextFloat() * 10.0F - 5.0F;
            Vec3 n = v.normalize();
            Vec3 p = v.rotatePitch(angle);
            Vec3 y = v.rotateYaw(angle);
            out.println("    { " + d(v.xCoord) + ", " + d(v.yCoord) + ", " + d(v.zCoord) + ", " + Float.toString(angle) + "f, " + d(n.xCoord) + ", " + d(n.yCoord) + ", " + d(n.zCoord)
                + ", " + d(p.xCoord) + ", " + d(p.yCoord) + ", " + d(p.zCoord) + ", " + d(y.xCoord) + ", " + d(y.yCoord) + ", " + d(y.zCoord) + ", " + d(v.lengthVector()) + " },");
        }
        out.println("};");

        // Ray / box intersection and collision offsets against a unit-ish box.
        out.println("struct InterceptCase { unsigned long long box[6], a[3], b[3]; int side; unsigned long long hit[3]; unsigned long long move; unsigned long long offsets[3]; };");
        out.println("inline constexpr InterceptCase interceptCases[] = {");
        for (int i = 0; i < 120; i++) {
            double x0 = r.nextInt(4) - 2 + (r.nextBoolean() ? 0.0 : r.nextDouble());
            double y0 = r.nextInt(4) - 2 + (r.nextBoolean() ? 0.0 : r.nextDouble());
            double z0 = r.nextInt(4) - 2 + (r.nextBoolean() ? 0.0 : r.nextDouble());
            AxisAlignedBB box = new AxisAlignedBB(x0, y0, z0, x0 + (i % 7 == 0 ? 0.0 : r.nextDouble() * 2), y0 + r.nextDouble() * 2, z0 + r.nextDouble() * 2);
            Vec3 a = new Vec3(r.nextGaussian() * 3, r.nextGaussian() * 3, r.nextGaussian() * 3);
            Vec3 b = new Vec3(r.nextGaussian() * 3, r.nextGaussian() * 3, r.nextGaussian() * 3);
            MovingObjectPosition hit = box.calculateIntercept(a, b);
            AxisAlignedBB other = new AxisAlignedBB(a.xCoord, a.yCoord, a.zCoord, a.xCoord + 0.6, a.yCoord + 1.8, a.zCoord + 0.6);
            double move = r.nextGaussian() * 2;
            out.println("    { { " + d(box.minX) + ", " + d(box.minY) + ", " + d(box.minZ) + ", " + d(box.maxX) + ", " + d(box.maxY) + ", " + d(box.maxZ) + " }, { "
                + d(a.xCoord) + ", " + d(a.yCoord) + ", " + d(a.zCoord) + " }, { " + d(b.xCoord) + ", " + d(b.yCoord) + ", " + d(b.zCoord) + " }, "
                + (hit == null ? -1 : hit.sideHit.ordinal()) + ", { "
                + (hit == null ? "0, 0, 0" : d(hit.hitVec.xCoord) + ", " + d(hit.hitVec.yCoord) + ", " + d(hit.hitVec.zCoord)) + " }, " + d(move) + ", { "
                + d(box.calculateXOffset(other, move)) + ", " + d(box.calculateYOffset(other, move)) + ", " + d(box.calculateZOffset(other, move)) + " } },");
        }
        out.println("};");
    }

    public static void main(String[] args) throws Exception {
        try (PrintWriter out = new PrintWriter(Files.newBufferedWriter(Paths.get(args[0], "NbtReference.inc"), StandardCharsets.UTF_8))) {
            nbt(out);
        }
        try (PrintWriter out = new PrintWriter(Files.newBufferedWriter(Paths.get(args[0], "MathReference.inc"), StandardCharsets.UTF_8))) {
            math(out);
        }
        net.minecraft.init.Bootstrap.register();
        try (PrintWriter out = new PrintWriter(Files.newBufferedWriter(Paths.get(args[0], "blocks.txt"), StandardCharsets.UTF_8))) {
            net.minecraft.block.BlockDump.dump(out);
        }
        try (PrintWriter out = new PrintWriter(Files.newBufferedWriter(Paths.get(args[0], "items.txt"), StandardCharsets.UTF_8))) {
            net.minecraft.item.ItemDump.dump(out);
        }
    }
}
