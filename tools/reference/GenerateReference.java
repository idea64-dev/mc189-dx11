import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.IllegalFormatException;
import java.util.Random;
import java.util.regex.Pattern;

import net.minecraft.util.MathHelper;

/**
 * Writes tests/reference/JavaReference.inc: inputs and expected outputs of java.util.Random, MCP's MathHelper and
 * String.hashCode, computed by a real JVM. tests/JavaParityTest.cpp runs the same calls in the same order and
 * compares bit for bit. Every "script" below must stay in sync with its C++ twin in that test.
 *
 * Floats and doubles are stored as raw bits; NaN results are compared as "is NaN" (the sign of NaN differs
 * between the JVM and the C runtime and never matters to the game).
 */
public class GenerateReference {

    static final long[] RANDOM_SEEDS = {
        0L, 1L, -1L, 42L, 12345L, 0x5DEECE66DL, Long.MIN_VALUE, Long.MAX_VALUE, -6447939183519582063L, 2147483648L
    };

    static final int[] BOUNDS = { 1, 2, 3, 7, 10, 16, 100, 256, 1000, 1 << 30, 0x40000001, Integer.MAX_VALUE };

    static void randomScript(Random r, List<Long> out) {
        for (int i = 0; i < 8; i++) out.add((long) r.nextInt());
        for (int b : BOUNDS) for (int i = 0; i < 4; i++) out.add((long) r.nextInt(b));
        for (int i = 0; i < 4; i++) out.add(r.nextLong());
        for (int i = 0; i < 4; i++) out.add((long) Float.floatToRawIntBits(r.nextFloat()));
        for (int i = 0; i < 4; i++) out.add(Double.doubleToRawLongBits(r.nextDouble()));
        for (int i = 0; i < 4; i++) out.add(r.nextBoolean() ? 1L : 0L);
        for (int i = 0; i < 7; i++) out.add(Double.doubleToRawLongBits(r.nextGaussian()));
        r.setSeed(r.nextLong());
        out.add(Double.doubleToRawLongBits(r.nextGaussian()));
        byte[] bytes = new byte[7];
        r.nextBytes(bytes);
        for (byte b : bytes) out.add((long) b);
    }

    // FNV-1a over the low `bytes` bytes of each value, little endian.
    static long fnv(long hash, long value, int bytes) {
        for (int i = 0; i < bytes; i++) {
            hash ^= (value >>> (8 * i)) & 0xFF;
            hash *= 0x100000001B3L;
        }
        return hash;
    }

    static final long FNV_OFFSET = 0xCBF29CE484222325L;

    static long randomStressHash() {
        Random r = new Random(2024L);
        long h = FNV_OFFSET;
        for (int i = 0; i < 20000; i++) {
            h = fnv(h, Double.doubleToRawLongBits(r.nextGaussian()), 8);
            h = fnv(h, r.nextInt(1 + (i % 1000)), 4);
            h = fnv(h, Float.floatToRawIntBits(r.nextFloat()), 4);
            h = fnv(h, r.nextLong(), 8);
        }
        return h;
    }

    static final float[] FLOATS = {
        0.0f, -0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 3.14159265f, -2.7182817f, 100.25f, -100.75f, 359.9f, 540.0f, -540.0f,
        1.0e6f, -1.0e6f, 123456.79f, 2.5e9f, -2.5e9f, 1.0e20f, -1.0e20f, 1.0e-30f, -1.0e-30f, 2147483520.0f,
        Float.NaN, Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY, Float.MAX_VALUE, Float.MIN_VALUE, 0.99999994f
    };

    static final double[] DOUBLES = {
        0.0, -0.0, 0.5, -0.5, 1.0, -1.0, 1.5, -1.5, 3.141592653589793, -2.718281828459045, 359.99999, 720.5, -720.5,
        1.0e9, -1.0e9, 2147483647.5, -2147483648.5, 3.0e9, -3.0e9, 1.0e19, -1.0e19, 1.0e-300, 4.9e-324,
        Double.NaN, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY, 0.9999999999999999, 29999999.999
    };

    static final int[] INTS = {
        0, 1, -1, 2, 3, 5, 15, 16, 17, 100, 255, 256, 1000, 65535, 65536, 1 << 30, (1 << 30) + 1,
        Integer.MAX_VALUE, Integer.MIN_VALUE, -7, -1000, -360, 361
    };

    static final double[] ATAN_VALUES = { -3.0, -1.0, -0.5, -1.0e-9, 0.0, 0.25, 1.0, 2.5, 1000.0 };

    static final String[] INT_STRINGS = {
        "0", "42", "-17", "+5", "007", "2147483647", "2147483648", "-2147483648", "-2147483649", "abc", "", " 5", "5 ",
        "1.5", "--1", "+-1", "-", "+"
    };

    static final String[] DOUBLE_STRINGS = {
        "1.5", " 2.5 ", "-3e2", "Infinity", "-Infinity", "+Infinity", "NaN", "1d", "2f", "3.0D", "abc", "", ".5", "5.",
        "1e400", "-1e-400", "inf", "nan", "1e", "+.5e1", "12345678901234567890"
    };

    static final String[] HASH_STRINGS = {
        "", "a", "minecraft", "Notch", "minecraft:textures/gui/title/minecraft.png", "Привет",
        "😀 smile", "The quick brown fox jumps over the lazy dog", "§cRed §lBold"
    };

    static final float[] TO_STRING_FLOATS = {
        0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 0.44366196f, 0.5412844f, 0.31690142f, 70.0f, 110.0f, 0.1f, 0.2f, 0.3f,
        1.0E-3f, 9.99E-4f, 1.0E-5f, 1234567.0f, 1.0E7f, 1.2345678E7f, 3.4028235E38f, 1.4E-45f, 100.0f, 0.075f,
        Float.NaN, Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY, 2.5E-4f, 123.456f, -98.765f
    };

    static final double[] TO_STRING_DOUBLES = {
        0.0, 1.0, 0.1, 0.3, 1.0 / 3.0, 2.0 / 3.0, 1.0E-3, 9.99E-4, 1.0E7, 1.2345678901234E7, 1.0E-300, 6.02214076E23,
        Double.MIN_VALUE, Double.MAX_VALUE, -1234.5678, 100.0
    };

    // Locale's rewrite pattern and String.format with three string arguments, as I18n would run them.
    static final Pattern LOCALE_PATTERN = Pattern.compile("%(\\d+\\$)?[\\d\\.]*[df]");
    static final String[] FORMAT_PATTERNS = {
        "plain text", "%s", "%s and %s", "%2$s then %1$s", "%d items", "%.2f%%", "%3$d/%1$s", "100%", "%", "%s %s %s %s",
        "%1$s%1$s", "%-5s|", "%5s|", "%n", "%0$s", "%S!", "%5.1f", "%.3d", "%%d", "%1$d%2$f", "%x", "%c", "50%% off %s"
    };

    static void mathScript(List<Long> out) {
        for (float f : FLOATS) {
            out.add((long) Float.floatToRawIntBits(MathHelper.sin(f)));
            out.add((long) Float.floatToRawIntBits(MathHelper.cos(f)));
            out.add((long) MathHelper.floor_float(f));
            out.add((long) MathHelper.ceiling_float_int(f));
            out.add((long) Float.floatToRawIntBits(MathHelper.wrapAngleTo180_float(f)));
            out.add((long) Float.floatToRawIntBits(MathHelper.sqrt_float(f)));
            out.add((long) Float.floatToRawIntBits(MathHelper.abs(f)));
        }
        for (double d : DOUBLES) {
            out.add((long) MathHelper.floor_double(d));
            out.add(MathHelper.floor_double_long(d));
            out.add((long) MathHelper.ceiling_double_int(d));
            out.add((long) MathHelper.truncateDoubleToInt(d));
            out.add((long) MathHelper.func_154353_e(d));
            out.add(Double.doubleToRawLongBits(MathHelper.wrapAngleTo180_double(d)));
            out.add(Double.doubleToRawLongBits(MathHelper.func_181162_h(d)));
            out.add(Double.doubleToRawLongBits(MathHelper.func_181161_i(Math.abs(d))));
            out.add((long) Float.floatToRawIntBits(MathHelper.sqrt_double(d)));
        }
        for (double y : ATAN_VALUES) for (double x : ATAN_VALUES) out.add(Double.doubleToRawLongBits(MathHelper.atan2(y, x)));
        for (int v : INTS) {
            out.add((long) MathHelper.roundUpToPowerOfTwo(v));
            out.add((long) MathHelper.calculateLogBaseTwo(v));
            out.add((long) MathHelper.abs_int(v));
            out.add((long) MathHelper.bucketInt(v, 16));
            out.add((long) MathHelper.bucketInt(v, 3));
            out.add((long) MathHelper.normalizeAngle(v, 360));
            out.add((long) MathHelper.roundUp(v, 16));
            out.add((long) MathHelper.roundUp(v, -7));
            out.add((long) MathHelper.roundUp(v, 0));
            out.add((long) MathHelper.func_180181_b(v, v >> 3, v ^ 0x55));
            for (int w : INTS) out.add((long) MathHelper.func_180188_d(v, w));
        }
        int[][] coords = { {0, 0, 0}, {1, 2, 3}, {-1, -64, -1}, {30000000, 255, -30000000}, {Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MAX_VALUE}, {123, 45, -678} };
        for (int[] c : coords) out.add(MathHelper.getCoordinateRandom(c[0], c[1], c[2]));
        float[] unit = { 0.0f, 0.1f, 0.33f, 0.5f, 0.66f, 0.83f, 0.99f, 1.0f };
        for (float h : unit) for (float s : unit) for (float v : unit) out.add((long) MathHelper.hsvToRGB(h, s, v));
        for (float r : unit) for (float g : unit) out.add((long) MathHelper.func_180183_b(r, g, 1.0f - r));
        for (String s : INT_STRINGS) {
            out.add((long) MathHelper.parseIntWithDefault(s, -999));
            out.add((long) MathHelper.parseIntWithDefaultAndMax(s, -999, 10));
        }
        for (String s : DOUBLE_STRINGS) {
            out.add(Double.doubleToRawLongBits(MathHelper.parseDoubleWithDefault(s, -999.0)));
            out.add(Double.doubleToRawLongBits(MathHelper.parseDoubleWithDefaultAndMax(s, -999.0, 2.0)));
        }
        for (float a : FLOATS) for (float b : new float[] { 0.0f, 1.0f, 1.000001f, 1.0001f }) out.add(MathHelper.epsilonEquals(a, b) ? 1L : 0L);
        for (double d : DOUBLES) {
            out.add(Double.doubleToRawLongBits(MathHelper.denormalizeClamp(-5.0, 7.5, d)));
            out.add(Double.doubleToRawLongBits(MathHelper.abs_max(d, -2.0)));
            out.add(Double.doubleToRawLongBits(MathHelper.func_181160_c(d, -1.0, 3.0)));
        }
        out.add(Double.doubleToRawLongBits(MathHelper.average(new long[] { 1L, 2L, 4L, Long.MAX_VALUE })));
        Random r = new Random(5L);
        for (int i = 0; i < 16; i++) {
            out.add((long) MathHelper.getRandomIntegerInRange(r, -10, 10 + i));
            out.add((long) Float.floatToRawIntBits(MathHelper.randomFloatClamp(r, -1.5f, 2.5f + i)));
            out.add(Double.doubleToRawLongBits(MathHelper.getRandomDoubleInRange(r, -3.0, 4.0 + i)));
        }
        out.add((long) Float.floatToRawIntBits(MathHelper.SQRT_2));
    }

    static long sinTableHash() {
        long h = FNV_OFFSET;
        for (int i = 0; i < 65536; i++) {
            // sin(i * 2pi / 65536) through the public API hits table entry i exactly
            float angle = (float) i / 10430.378F;
            h = fnv(h, Float.floatToRawIntBits(MathHelper.sin(angle)), 4);
        }
        return h;
    }

    static String cppString(String s) {
        StringBuilder b = new StringBuilder("u8\"");
        for (int i = 0; i < s.length(); ) {
            int cp = s.codePointAt(i);
            i += Character.charCount(cp);
            if (cp == '"' || cp == '\\') b.append('\\').append((char) cp);
            else if (cp >= 0x20 && cp < 0x7F) b.append((char) cp);
            else if (cp <= 0xFFFF) b.append(String.format("\\u%04X", cp));
            else b.append(String.format("\\U%08X", cp));
        }
        return b.append('"').toString();
    }

    static void writeLongs(PrintWriter w, String name, List<Long> values) {
        w.println("inline constexpr int64_t " + name + "[] = {");
        for (int i = 0; i < values.size(); i += 6) {
            StringBuilder line = new StringBuilder("   ");
            for (int j = i; j < Math.min(i + 6, values.size()); j++) {
                long v = values.get(j);
                line.append(' ').append(v == Long.MIN_VALUE ? "INT64_MIN" : v + "LL").append(',');
            }
            w.println(line);
        }
        w.println("};");
        w.println();
    }

    /**
     * MathHelper's atan2 lookup tables, exactly as its static initializer builds them on this JVM. The C runtime's
     * asin/cos differ from the JVM's in the last bit for some entries, so the game embeds these values.
     */
    static void writeAtanTables(String path) throws Exception {
        try (PrintWriter w = new PrintWriter(Files.newBufferedWriter(Paths.get(path), StandardCharsets.UTF_8))) {
            w.println("// Generated by tools/reference/GenerateReference.java on " + System.getProperty("java.vendor") + " "
                + System.getProperty("java.version") + ". Do not edit; rerun tools/reference/generate.bat.");
            w.println("// MathHelper.field_181164_e (asin) and field_181165_f (cos of asin) as raw double bits.");
            w.println("// clang-format off");
            w.println();
            List<Long> asin = new ArrayList<>();
            List<Long> cos = new ArrayList<>();
            for (int j = 0; j < 257; ++j) {
                double d0 = (double) j / 256.0D;
                double d1 = Math.asin(d0);
                cos.add(Double.doubleToRawLongBits(Math.cos(d1)));
                asin.add(Double.doubleToRawLongBits(d1));
            }
            writeLongs(w, "kAsinTableBits", asin);
            writeLongs(w, "kCosOfAsinTableBits", cos);
        }
    }

    public static void main(String[] args) throws Exception {
        List<Long> random = new ArrayList<>();
        for (long seed : RANDOM_SEEDS) randomScript(new Random(seed), random);
        List<Long> math = new ArrayList<>();
        mathScript(math);

        try (PrintWriter w = new PrintWriter(Files.newBufferedWriter(Paths.get(args[0]), StandardCharsets.UTF_8))) {
            w.println("// Generated by tools/reference/GenerateReference.java on " + System.getProperty("java.vendor") + " "
                + System.getProperty("java.version") + ". Do not edit; rerun tools/reference/generate.bat.");
            w.println("// clang-format off");
            w.println("#pragma once");
            w.println();
            w.println("#include <cstdint>");
            w.println();
            w.println("namespace javaref {");
            w.println();
            List<Long> seeds = new ArrayList<>();
            for (long s : RANDOM_SEEDS) seeds.add(s);
            writeLongs(w, "randomSeeds", seeds);
            List<Long> bounds = new ArrayList<>();
            for (int b : BOUNDS) bounds.add((long) b);
            writeLongs(w, "randomBounds", bounds);
            writeLongs(w, "randomScript", random);
            w.println("inline constexpr uint64_t randomStressHash = 0x" + Long.toHexString(randomStressHash()) + "ULL;");
            w.println("inline constexpr uint64_t sinTableHash = 0x" + Long.toHexString(sinTableHash()) + "ULL;");
            w.println();

            List<Long> floats = new ArrayList<>();
            for (float f : FLOATS) floats.add((long) Float.floatToRawIntBits(f));
            writeLongs(w, "floatInputBits", floats);
            List<Long> doubles = new ArrayList<>();
            for (double d : DOUBLES) doubles.add(Double.doubleToRawLongBits(d));
            writeLongs(w, "doubleInputBits", doubles);
            List<Long> atan = new ArrayList<>();
            for (double d : ATAN_VALUES) atan.add(Double.doubleToRawLongBits(d));
            writeLongs(w, "atanInputBits", atan);
            List<Long> ints = new ArrayList<>();
            for (int v : INTS) ints.add((long) v);
            writeLongs(w, "intInputs", ints);

            w.println("inline constexpr const char8_t* intStrings[] = {");
            for (String s : INT_STRINGS) w.println("    " + cppString(s) + ",");
            w.println("};");
            w.println("inline constexpr const char8_t* doubleStrings[] = {");
            for (String s : DOUBLE_STRINGS) w.println("    " + cppString(s) + ",");
            w.println("};");
            w.println();
            writeLongs(w, "mathScript", math);

            w.println("struct FloatString { int32_t bits; const char8_t* text; };");
            w.println("inline constexpr FloatString floatToStrings[] = {");
            for (float f : TO_STRING_FLOATS) w.println("    { " + Float.floatToRawIntBits(f) + ", " + cppString(Float.toString(f)) + " },");
            w.println("};");
            w.println("struct DoubleString { int64_t bits; const char8_t* text; };");
            w.println("inline constexpr DoubleString doubleToStrings[] = {");
            for (double d : TO_STRING_DOUBLES) {
                long bits = Double.doubleToRawLongBits(d);
                w.println("    { " + (bits == Long.MIN_VALUE ? "INT64_MIN" : bits + "LL") + ", " + cppString(Double.toString(d)) + " },");
            }
            w.println("};");
            w.println("struct FormatCase { const char8_t* pattern; const char8_t* rewritten; bool ok; const char8_t* result; };");
            w.println("inline constexpr FormatCase formatCases[] = {");
            for (String p : FORMAT_PATTERNS) {
                String rewritten = LOCALE_PATTERN.matcher(p).replaceAll("%$1s");
                String result;
                boolean ok;
                try {
                    result = String.format(rewritten, "A", "B", "C");
                    ok = true;
                } catch (IllegalFormatException e) {
                    result = "";
                    ok = false;
                }
                w.println("    { " + cppString(p) + ", " + cppString(rewritten) + ", " + ok + ", " + cppString(result) + " },");
            }
            w.println("};");
            w.println();
            w.println("struct StringHash { const char8_t* text; int32_t hash; };");
            w.println("inline constexpr StringHash stringHashes[] = {");
            for (String s : HASH_STRINGS) w.println("    { " + cppString(s) + ", " + s.hashCode() + " },");
            w.println("};");
            w.println();
            w.println("} // namespace javaref");
        }
        writeAtanTables(args[1]);
        System.out.println("Wrote " + args[0] + " (" + random.size() + " random values, " + math.size() + " math values)");
    }
}
