package net.minecraft.util;

// Minimal stand-in so the real MathHelper.java compiles without Guava.
public class Vec3i {
    private final int x, y, z;
    public Vec3i(int x, int y, int z) { this.x = x; this.y = y; this.z = z; }
    public int getX() { return x; }
    public int getY() { return y; }
    public int getZ() { return z; }
}
