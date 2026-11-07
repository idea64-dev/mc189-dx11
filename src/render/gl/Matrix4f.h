#pragma once

// 4x4 float matrix in OpenGL's column-major layout (m[column * 4 + row]), with the operations of the GL 1.x
// matrix stack. Operations post-multiply, as glTranslate/glRotate/glMultMatrix do.

#include <array>
#include <cmath>

namespace mc {

struct Matrix4f {
    std::array<float, 16> m {};

    static Matrix4f identity()
    {
        Matrix4f r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    float& at(int row, int column) { return m[static_cast<size_t>(column * 4 + row)]; }
    float at(int row, int column) const { return m[static_cast<size_t>(column * 4 + row)]; }

    Matrix4f operator*(const Matrix4f& b) const
    {
        Matrix4f r;
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += at(row, k) * b.at(k, column);
                r.at(row, column) = sum;
            }
        }
        return r;
    }

    // glTranslatef
    void translate(float x, float y, float z)
    {
        for (int row = 0; row < 4; ++row)
            at(row, 3) += at(row, 0) * x + at(row, 1) * y + at(row, 2) * z;
    }

    // glScalef
    void scale(float x, float y, float z)
    {
        for (int row = 0; row < 4; ++row) {
            at(row, 0) *= x;
            at(row, 1) *= y;
            at(row, 2) *= z;
        }
    }

    // glRotatef: angle in degrees around (x, y, z), normalized as the GL spec requires.
    void rotate(float angleDegrees, float x, float y, float z)
    {
        const float length = std::sqrt(x * x + y * y + z * z);
        if (length == 0.0f)
            return;
        x /= length;
        y /= length;
        z /= length;

        const float radians = angleDegrees * (3.14159265358979323846f / 180.0f);
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        const float t = 1.0f - c;

        Matrix4f rotation = identity();
        rotation.at(0, 0) = x * x * t + c;
        rotation.at(0, 1) = x * y * t - z * s;
        rotation.at(0, 2) = x * z * t + y * s;
        rotation.at(1, 0) = y * x * t + z * s;
        rotation.at(1, 1) = y * y * t + c;
        rotation.at(1, 2) = y * z * t - x * s;
        rotation.at(2, 0) = z * x * t - y * s;
        rotation.at(2, 1) = z * y * t + x * s;
        rotation.at(2, 2) = z * z * t + c;
        *this = *this * rotation;
    }

    // glOrtho
    void ortho(double left, double right, double bottom, double top, double zNear, double zFar)
    {
        Matrix4f o = identity();
        o.at(0, 0) = static_cast<float>(2.0 / (right - left));
        o.at(1, 1) = static_cast<float>(2.0 / (top - bottom));
        o.at(2, 2) = static_cast<float>(-2.0 / (zFar - zNear));
        o.at(0, 3) = static_cast<float>(-(right + left) / (right - left));
        o.at(1, 3) = static_cast<float>(-(top + bottom) / (top - bottom));
        o.at(2, 3) = static_cast<float>(-(zFar + zNear) / (zFar - zNear));
        *this = *this * o;
    }

    // Inverse of the upper-left 3x3, transposed: transforms normals into eye space.
    std::array<float, 9> normalMatrix() const
    {
        const float a = at(0, 0), b = at(0, 1), c = at(0, 2);
        const float d = at(1, 0), e = at(1, 1), f = at(1, 2);
        const float g = at(2, 0), h = at(2, 1), i = at(2, 2);
        const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
        const float inv = det != 0.0f ? 1.0f / det : 0.0f;
        // (M^-1)^T, row-major 3x3
        return {
            (e * i - f * h) * inv, -(d * i - f * g) * inv, (d * h - e * g) * inv,
            -(b * i - c * h) * inv, (a * i - c * g) * inv, -(a * h - b * g) * inv,
            (b * f - c * e) * inv, -(a * f - c * d) * inv, (a * e - b * d) * inv,
        };
    }
};

} // namespace mc
