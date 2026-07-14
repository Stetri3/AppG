#pragma once
#include <dawn/webgpu.h>
#include "engine_def.h"
#include <unordered_map>
#include <vector>

#ifndef E_COUPLED
constexpr bool E_COUPLED = false; //metodo di coupling program/engine.
//Se true, engine modifica direttamente i field di Program senza passare attraverso i metodi
//Se false, program richiede i valori ad engine e ognuno modifica solo i propri membri
#endif

class Program;

struct Vertex {
    float position[3]; // X, Y, Z
    float color[3];    // R, G, B
};

// This struct matches standard 3D uniform layouts alignment rules
struct MeshUniforms {
    float modelViewProjectionMatrix[16]; // 4x4 matrix
    float customColorTint[4];            // RGBA multiplier
    float time;                          // Dynamic time variable for procedural animations
    float padding[3];                    // Explicit 16-byte alignment padding required by WGSL
};

class Engine {
    //Resource vars
    Program* program = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;

    //Global info
    WGPUSupportedFeatures devFeatures{};
    WGPULimits devLimits{};

    //Global pipeline & states
    WGPURenderPipeline renderPipeline = nullptr;
    WGPUBindGroupLayout bindGroupLayout = nullptr;
    WGPUPipelineLayout pipelineLayout = nullptr;

    //Global buffers
    WGPUBuffer globalUniformBuffer = nullptr;
    gfx::GlobalUniforms globalUniformData{};

    //3D framebuffer resources
    WGPUTexture depthTexture = nullptr;
    WGPUTextureView depthTextureView = nullptr;
    WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus;

    //Registri di risorse decentralizzati
    std::unordered_map<uint32_t, gfx::MeshResource> meshRegistry;
    std::unordered_map<uint32_t, gfx::SkeletonResource> skeletonRegistry;
    std::vector<gfx::RenderInstance> activeSceneObjects;

    uint32_t resourceIdCounter = 1;

    //variabili size synced
    float width = 1280.0f;
    float height = 720.0f;

    // Private Pipeline Initializers
    void CreateRenderPipeline();
    void CreateDepthResources();

public:
    Engine(Program* program);
    ~Engine();

    // Initializes the engine pipeline using an already fetched device
    void Initialize();
    void HandleResize(float w, float h);
    
    // --- Asset Registration (How you feed the engine dynamically) ---
    uint32_t RegisterMesh(const std::vector<gfx::Vertex>& vertices, const std::vector<uint32_t>& indices);
    uint32_t RegisterSkeleton(uint32_t jointCount);

    // Create an instance linking a mesh and skeleton to the render loop
    void CreateRenderInstance(uint32_t meshId, uint32_t skeletonId);

    void Update(float deltaTime);
    void Render(WGPUTextureView targetView);
    void Clean();
};