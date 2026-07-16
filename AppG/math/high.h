#pragma once
#include <cmath>
//File per funzioni e tipi math "high" level, ovvero a scopo operativo ma ancora da piazzare nella struttura correttamente
inline void GetPerspectiveMatrix(float fovRadians, float aspect, float nearZ, float farZ, float* out16) {
    float f = 1.0f / std::tan(fovRadians * 0.5f);

    out16[0] = f / aspect; out16[1] = 0.0f; out16[2] = 0.0f; out16[3] = 0.0f;
    out16[4] = 0.0f;      out16[5] = f;      out16[6] = 0.0f; out16[7] = 0.0f;

    // WebGPU Depth range is [0.0, 1.0]
    out16[8] = 0.0f;      out16[9] = 0.0f;      out16[10] = farZ / (nearZ - farZ); out16[11] = -1.0f;
    out16[12] = 0.0f;     out16[13] = 0.0f;     out16[14] = (nearZ * farZ) / (nearZ - farZ); out16[15] = 0.0f;
}

inline void GetTranslationMatrix(float tx, float ty, float tz, float* out16) {
    for (int i = 0; i < 16; ++i) out16[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    out16[12] = tx; out16[13] = ty; out16[14] = tz;
}