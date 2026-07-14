#pragma once
#include <dawn/webgpu.h>
namespace gfx {
    // Rigid, predictable vertex layout for 3D meshes
    struct Vertex {
        float position[3];  // X, Y, Z
        float normal[3];    // Nx, Ny, Nz (Crucial for 3D lighting/creases)
        float color[3];     // R, G, B
        uint32_t joints[4]; // Skeletal joint index influences
        float weights[4];   // Normalized joint influence weights
    };

    // Global engine-wide constants (Camera matrices, environment settings)
    struct GlobalUniforms {
        float viewProjection[16]; // 4x4 Camera Matrix
        float time;
        float padding[3];         // 16-byte boundary alignment
    };

    // Handle to a GPU-allocated mesh
    struct MeshResource {
        WGPUBuffer vertexBuffer = nullptr;
        WGPUBuffer indexBuffer = nullptr;
        uint32_t indexCount = 0;
    };

    // Handle to a GPU-allocated skeletal joint palette
    struct SkeletonResource {
        WGPUBuffer jointStorageBuffer = nullptr; // Raw Storage buffer (O(N) updates go here)
        uint32_t jointCount = 0;
    };

    // Represents a single drawable entity in your scene
    struct RenderInstance {
        uint32_t meshId = 0;
        uint32_t skeletonId = 0; // Index into skeletons map, 0 if static

        // Per-instance transform & material uniforms
        float modelMatrix[16];
        float colorTint[4];

        WGPUBuffer instanceUniformBuffer = nullptr;
        WGPUBindGroup bindGroup = nullptr; // Combines GlobalUniforms + InstanceUniforms + Skeleton
    };
}