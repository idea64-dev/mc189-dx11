#pragma once

// The parts of org.lwjgl.util.vector (LWJGL 2.9.4) that the model code uses: Vector3f, Vector4f and Matrix4f,
// with the same float operations in the same order, so baked models come out bit for bit. The matrix keeps
// LWJGL's field names: mRC is column R, row C (m30 is the x translation).

#include "core/JavaMath.h"

namespace mc::lwjgl {

struct Vector3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vector3f() = default;
    constexpr Vector3f(float xIn, float yIn, float zIn)
        : x(xIn)
        , y(yIn)
        , z(zIn)
    {
    }

    void set(float xIn, float yIn, float zIn)
    {
        x = xIn;
        y = yIn;
        z = zIn;
    }

    void scale(float s)
    {
        x *= s;
        y *= s;
        z *= s;
    }

    // Float ==, so -0 equals 0 and NaN equals nothing.
    bool operator==(const Vector3f& other) const { return x == other.x && y == other.y && z == other.z; }

    static Vector3f add(const Vector3f& left, const Vector3f& right) { return { left.x + right.x, left.y + right.y, left.z + right.z }; }
    static Vector3f sub(const Vector3f& left, const Vector3f& right) { return { left.x - right.x, left.y - right.y, left.z - right.z }; }
    static Vector3f cross(const Vector3f& left, const Vector3f& right)
    {
        return { left.y * right.z - left.z * right.y, right.x * left.z - right.z * left.x, left.x * right.y - left.y * right.x };
    }
    static float dot(const Vector3f& left, const Vector3f& right) { return left.x * right.x + left.y * right.y + left.z * right.z; }
};

struct Vector4f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct Matrix4f {
    // new Matrix4f() is the identity in LWJGL.
    float m00 = 1.0f, m01 = 0.0f, m02 = 0.0f, m03 = 0.0f;
    float m10 = 0.0f, m11 = 1.0f, m12 = 0.0f, m13 = 0.0f;
    float m20 = 0.0f, m21 = 0.0f, m22 = 1.0f, m23 = 0.0f;
    float m30 = 0.0f, m31 = 0.0f, m32 = 0.0f, m33 = 1.0f;

    // Matrix4f.rotate(angle, axis, src, dest): src times the rotation. Math.sin/cos, as LWJGL calls them.
    static Matrix4f rotate(float angle, const Vector3f& axis, const Matrix4f& src)
    {
        const float c = static_cast<float>(java::mathCos(static_cast<double>(angle)));
        const float s = static_cast<float>(java::mathSin(static_cast<double>(angle)));
        const float oneminusc = 1.0f - c;
        const float xy = axis.x * axis.y;
        const float yz = axis.y * axis.z;
        const float xz = axis.x * axis.z;
        const float xs = axis.x * s;
        const float ys = axis.y * s;
        const float zs = axis.z * s;

        const float f00 = axis.x * axis.x * oneminusc + c;
        const float f01 = xy * oneminusc + zs;
        const float f02 = xz * oneminusc - ys;
        const float f10 = xy * oneminusc - zs;
        const float f11 = axis.y * axis.y * oneminusc + c;
        const float f12 = yz * oneminusc + xs;
        const float f20 = xz * oneminusc + ys;
        const float f21 = yz * oneminusc - xs;
        const float f22 = axis.z * axis.z * oneminusc + c;

        Matrix4f dest = src;
        const float t00 = src.m00 * f00 + src.m10 * f01 + src.m20 * f02;
        const float t01 = src.m01 * f00 + src.m11 * f01 + src.m21 * f02;
        const float t02 = src.m02 * f00 + src.m12 * f01 + src.m22 * f02;
        const float t03 = src.m03 * f00 + src.m13 * f01 + src.m23 * f02;
        const float t10 = src.m00 * f10 + src.m10 * f11 + src.m20 * f12;
        const float t11 = src.m01 * f10 + src.m11 * f11 + src.m21 * f12;
        const float t12 = src.m02 * f10 + src.m12 * f11 + src.m22 * f12;
        const float t13 = src.m03 * f10 + src.m13 * f11 + src.m23 * f12;
        dest.m20 = src.m00 * f20 + src.m10 * f21 + src.m20 * f22;
        dest.m21 = src.m01 * f20 + src.m11 * f21 + src.m21 * f22;
        dest.m22 = src.m02 * f20 + src.m12 * f21 + src.m22 * f22;
        dest.m23 = src.m03 * f20 + src.m13 * f21 + src.m23 * f22;
        dest.m00 = t00;
        dest.m01 = t01;
        dest.m02 = t02;
        dest.m03 = t03;
        dest.m10 = t10;
        dest.m11 = t11;
        dest.m12 = t12;
        dest.m13 = t13;
        return dest;
    }

    // Matrix4f.mul(left, right, dest)
    static Matrix4f mul(const Matrix4f& left, const Matrix4f& right)
    {
        Matrix4f d;
        d.m00 = left.m00 * right.m00 + left.m10 * right.m01 + left.m20 * right.m02 + left.m30 * right.m03;
        d.m01 = left.m01 * right.m00 + left.m11 * right.m01 + left.m21 * right.m02 + left.m31 * right.m03;
        d.m02 = left.m02 * right.m00 + left.m12 * right.m01 + left.m22 * right.m02 + left.m32 * right.m03;
        d.m03 = left.m03 * right.m00 + left.m13 * right.m01 + left.m23 * right.m02 + left.m33 * right.m03;
        d.m10 = left.m00 * right.m10 + left.m10 * right.m11 + left.m20 * right.m12 + left.m30 * right.m13;
        d.m11 = left.m01 * right.m10 + left.m11 * right.m11 + left.m21 * right.m12 + left.m31 * right.m13;
        d.m12 = left.m02 * right.m10 + left.m12 * right.m11 + left.m22 * right.m12 + left.m32 * right.m13;
        d.m13 = left.m03 * right.m10 + left.m13 * right.m11 + left.m23 * right.m12 + left.m33 * right.m13;
        d.m20 = left.m00 * right.m20 + left.m10 * right.m21 + left.m20 * right.m22 + left.m30 * right.m23;
        d.m21 = left.m01 * right.m20 + left.m11 * right.m21 + left.m21 * right.m22 + left.m31 * right.m23;
        d.m22 = left.m02 * right.m20 + left.m12 * right.m21 + left.m22 * right.m22 + left.m32 * right.m23;
        d.m23 = left.m03 * right.m20 + left.m13 * right.m21 + left.m23 * right.m22 + left.m33 * right.m23;
        d.m30 = left.m00 * right.m30 + left.m10 * right.m31 + left.m20 * right.m32 + left.m30 * right.m33;
        d.m31 = left.m01 * right.m30 + left.m11 * right.m31 + left.m21 * right.m32 + left.m31 * right.m33;
        d.m32 = left.m02 * right.m30 + left.m12 * right.m31 + left.m22 * right.m32 + left.m32 * right.m33;
        d.m33 = left.m03 * right.m30 + left.m13 * right.m31 + left.m23 * right.m32 + left.m33 * right.m33;
        return d;
    }

    // Matrix4f.transform(left, right, dest)
    static Vector4f transform(const Matrix4f& left, const Vector4f& right)
    {
        Vector4f d;
        d.x = left.m00 * right.x + left.m10 * right.y + left.m20 * right.z + left.m30 * right.w;
        d.y = left.m01 * right.x + left.m11 * right.y + left.m21 * right.z + left.m31 * right.w;
        d.z = left.m02 * right.x + left.m12 * right.y + left.m22 * right.z + left.m32 * right.w;
        d.w = left.m03 * right.x + left.m13 * right.y + left.m23 * right.z + left.m33 * right.w;
        return d;
    }
};

} // namespace mc::lwjgl
