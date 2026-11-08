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

// LWJGL's Project.__gluInvertMatrixf: Gauss-Jordan with partial pivoting, in float.
inline bool gluInvertMatrixf(const float* src, float* inverse)
{
    float temp[16];
    for (int i = 0; i < 16; ++i)
        temp[i] = src[i];
    for (int i = 0; i < 16; ++i)
        inverse[i] = (i % 5 == 0) ? 1.0f : 0.0f;

    for (int i = 0; i < 4; ++i) {
        int swap = i;
        for (int j = i + 1; j < 4; ++j) {
            if (std::fabs(temp[j * 4 + i]) > std::fabs(temp[i * 4 + i]))
                swap = j;
        }
        if (swap != i) {
            for (int k = 0; k < 4; ++k) {
                float t = temp[i * 4 + k];
                temp[i * 4 + k] = temp[swap * 4 + k];
                temp[swap * 4 + k] = t;
                t = inverse[i * 4 + k];
                inverse[i * 4 + k] = inverse[swap * 4 + k];
                inverse[swap * 4 + k] = t;
            }
        }
        if (temp[i * 4 + i] == 0)
            return false;
        const float t = temp[i * 4 + i];
        for (int k = 0; k < 4; ++k) {
            temp[i * 4 + k] = temp[i * 4 + k] / t;
            inverse[i * 4 + k] = inverse[i * 4 + k] / t;
        }
        for (int j = 0; j < 4; ++j) {
            if (j != i) {
                const float u = temp[j * 4 + i];
                for (int k = 0; k < 4; ++k) {
                    temp[j * 4 + k] -= temp[i * 4 + k] * u;
                    inverse[j * 4 + k] -= inverse[i * 4 + k] * u;
                }
            }
        }
    }
    return true;
}

// LWJGL's Project.gluUnProject (float), column-major matrices as glGetFloat returns them.
inline bool gluUnProject(float winx, float winy, float winz, const float* modelMatrix, const float* projMatrix, const int* viewport, float* objPos)
{
    float finalMatrix[16];
    // __gluMultMatricesf(modelMatrix, projMatrix, finalMatrix)
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            finalMatrix[i * 4 + j] = modelMatrix[i * 4 + 0] * projMatrix[0 * 4 + j] + modelMatrix[i * 4 + 1] * projMatrix[1 * 4 + j]
                + modelMatrix[i * 4 + 2] * projMatrix[2 * 4 + j] + modelMatrix[i * 4 + 3] * projMatrix[3 * 4 + j];
        }
    }
    if (!gluInvertMatrixf(finalMatrix, finalMatrix))
        return false;

    float in[4] { winx, winy, winz, 1.0f };
    in[0] = (in[0] - static_cast<float>(viewport[0])) / static_cast<float>(viewport[2]);
    in[1] = (in[1] - static_cast<float>(viewport[1])) / static_cast<float>(viewport[3]);
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;

    float out[4];
    // __gluMultMatrixVecf(finalMatrix, in, out)
    for (int i = 0; i < 4; ++i)
        out[i] = in[0] * finalMatrix[0 * 4 + i] + in[1] * finalMatrix[1 * 4 + i] + in[2] * finalMatrix[2 * 4 + i] + in[3] * finalMatrix[3 * 4 + i];
    if (out[3] == 0.0)
        return false;
    out[3] = 1.0f / out[3];
    objPos[0] = out[0] * out[3];
    objPos[1] = out[1] * out[3];
    objPos[2] = out[2] * out[3];
    return true;
}

} // namespace mc::Project
