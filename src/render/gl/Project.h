#pragma once

// Original: org.lwjgl.util.glu.Project (LWJGL 2), computed in float exactly as LWJGL does.

#include "render/gl/GL.h"

#include <cmath>

namespace mc::Project {

inline void gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
    constexpr float PI = 3.14159265358979323846f; // GLU.PI
    const float radians = fovy / 2 * PI / 180;
    const float deltaZ = zFar - zNear;
    const float sine = static_cast<float>(std::sin(static_cast<double>(radians)));
    if (deltaZ == 0 || sine == 0 || aspect == 0)
        return;
    const float cotangent = static_cast<float>(std::cos(static_cast<double>(radians))) / sine;

    float matrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    matrix[0 * 4 + 0] = cotangent / aspect;
    matrix[1 * 4 + 1] = cotangent;
    matrix[2 * 4 + 2] = -(zFar + zNear) / deltaZ;
    matrix[2 * 4 + 3] = -1;
    matrix[3 * 4 + 2] = -2 * zNear * zFar / deltaZ;
    matrix[3 * 4 + 3] = 0;
    GL::glMultMatrixf(matrix);
}

} // namespace mc::Project
