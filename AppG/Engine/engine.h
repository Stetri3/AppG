#pragma once
#include <dawn/webgpu.h>

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
    Program* program;
    WGPUDevice device;
    WGPUSupportedFeatures devFeatures{};
    WGPULimits devLimits{};

    // --- Added Rendering Handles ---
    WGPUQueue queue = nullptr;                 // Used to submit command buffers and upload data
    WGPURenderPipeline pipeline = nullptr;     // The immutable state blueprint (Shaders + Layout configurations)
    WGPUBuffer vertexBuffer = nullptr;         // GPU memory allocation holding our 3D geometry

    WGPUBuffer uniformBuffer;         // The actual chunk of GPU memory holding your matrix/time data
    WGPUBindGroupLayout bindGroupLayout; // The structural "blueprint" of what the shader expects
    WGPUBindGroup bindGroup;           // The actual link tying the uniformBuffer to the layout slots

    //variabili synced
    float width = 1920.0f;
    float height = 1080.0f;
public:
    Engine(Program* program);
    ~Engine();

    // Initializes the engine pipeline using an already fetched device
    void Initialize();
    void updateWinSize(float w, float h) { width = w; height = h; }
    // Executes the actual rendering steps for a single frame
    void RenderFrame(WGPUTextureView targetView);
    void RenderFrame2(WGPUTextureView targetView, float currentFrameTime);
private:
    // Internal helper methods to organize pipeline building steps
    void CreateRenderPipeline();
    void CreateGeometry();
};