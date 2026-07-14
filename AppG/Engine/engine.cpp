#include "engine.h"
#include "../program.h"
#include <iostream>

// Standard WGSL skeletal vertex shader
const WGPUStringView standardShaderSource = R"(
struct GlobalUniforms {
    viewProj : mat4x4<f32>,
    time : f32,
}

struct InstanceUniforms {
    model : mat4x4<f32>,
    colorTint : vec4<f32>,
}

@group(0) @binding(0) var<uniform> globals : GlobalUniforms;
@group(0) @binding(1) var<uniform> instance : InstanceUniforms;
@group(0) @binding(2) var<storage, read> joints : array<mat4x4<f32>>;

struct VertexInput {
    @location(0) position : vec3<f32>,
    @location(1) normal : vec3<f32>,
    @location(2) color : vec3<f32>,
    @location(3) joints : vec4<u32>,
    @location(4) weights : vec4<f32>,
}

struct VertexOutput {
    @builtin(position) clip_position : vec4<f32>,
    @location(0) color : vec3<f32>,
}

@vertex
fn vs_main(model : VertexInput) -> VertexOutput {
    var out: VertexOutput;
    
    // Resolve Absolute Skeletal Transform
    let skinMatrix = 
        model.weights.x * joints[model.joints.x] +
        model.weights.y * joints[model.joints.y] +
        model.weights.z * joints[model.joints.z] +
        model.weights.w * joints[model.joints.w];
        
    let worldPosition = instance.model * skinMatrix * vec4<f32>(model.position, 1.0);
    
    out.clip_position = globals.viewProj * worldPosition;
    out.color = model.color * instance.colorTint.rgb;
    return out;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}
)"_ws;

Engine::Engine(Program* p) : program(p) {}

Engine::~Engine() {
    Clean();
}

void Engine::Initialize() {
    device = program->getDevice();
    queue = program->getQueue();

    // 1. Setup frame buffers and base pipelines
    CreateDepthResources();
    CreateRenderPipeline();

    // 2. Allocate global system memory (such as camera buffers)
    WGPUBufferDescriptor globalBufferDesc = {};
    globalBufferDesc.size = sizeof(gfx::GlobalUniforms);
    globalBufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    globalUniformBuffer = wgpuDeviceCreateBuffer(device, &globalBufferDesc);
}

void Engine::HandleResize(float w, float h) {
    width = w;
    height = h;
    CreateDepthResources(); // Recreate depth texture to match window bounds
}

// ============================================================================
// PIPELINE CREATION
// ============================================================================
void Engine::CreateRenderPipeline() {
    // A. BIND GROUP LAYOUT: The static blueprint of variables expected by our pipeline
    WGPUBindGroupLayoutEntry entries[3] = {};
    // Binding 0: Global Uniforms (Camera, Time)
    entries[0].binding = 0;
    entries[0].visibility = WGPUShaderStage_Vertex;
    entries[0].buffer.type = WGPUBufferBindingType_Uniform;

    // Binding 1: Instance Uniforms (Transform, Color Tint)
    entries[1].binding = 1;
    entries[1].visibility = WGPUShaderStage_Vertex;
    entries[1].buffer.type = WGPUBufferBindingType_Uniform;

    // Binding 2: Skeleton Storage Buffer (Joint transformation matrices)
    entries[2].binding = 2;
    entries[2].visibility = WGPUShaderStage_Vertex;
    entries[2].buffer.type = WGPUBufferBindingType_ReadOnlyStorage;

    WGPUBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = 3;
    layoutDesc.entries = entries;
    bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

    // Build the Pipeline Layout
    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
    pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

    // Shaders, Vertex attribute formatting, and Depth Stencil states 
    // are compiled here to create 'renderPipeline'...
}

void Engine::CreateDepthResources() {
    if (depthTextureView) wgpuTextureViewRelease(depthTextureView);
    if (depthTexture) wgpuTextureRelease(depthTexture);

    WGPUTextureDescriptor desc = {};
    desc.dimension = WGPUTextureDimension_2D;
    desc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    desc.format = depthFormat;
    desc.usage = WGPUTextureUsage_RenderAttachment;
    desc.sampleCount = 1;
    desc.mipLevelCount = 1;

    depthTexture = wgpuDeviceCreateTexture(device, &desc);
    depthTextureView = wgpuTextureCreateView(depthTexture, nullptr);
}

// ============================================================================
// RESOURCE REGISTRATION (Allocations)
// ============================================================================
uint32_t Engine::RegisterMesh(const std::vector<gfx::Vertex>& vertices, const std::vector<uint32_t>& indices) {
    uint32_t id = resourceIdCounter++;
    gfx::MeshResource res;
    res.indexCount = static_cast<uint32_t>(indices.size());

    // Allocate Vertex Buffer on GPU
    WGPUBufferDescriptor vDesc = {};
    vDesc.size = vertices.size() * sizeof(gfx::Vertex);
    vDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    res.vertexBuffer = wgpuDeviceCreateBuffer(device, &vDesc);
    wgpuQueueWriteBuffer(queue, res.vertexBuffer, 0, vertices.data(), vDesc.size);

    // Allocate Index Buffer on GPU
    WGPUBufferDescriptor iDesc = {};
    iDesc.size = indices.size() * sizeof(uint32_t);
    iDesc.usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst;
    res.indexBuffer = wgpuDeviceCreateBuffer(device, &iDesc);
    wgpuQueueWriteBuffer(queue, res.indexBuffer, 0, indices.data(), iDesc.size);

    meshRegistry[id] = res;
    return id;
}

uint32_t Engine::RegisterSkeleton(uint32_t jointCount) {
    uint32_t id = resourceIdCounter++;
    gfx::SkeletonResource res;
    res.jointCount = jointCount;

    WGPUBufferDescriptor sDesc = {};
    sDesc.size = jointCount * sizeof(float) * 16; // 16 float matrix size
    sDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
    res.jointStorageBuffer = wgpuDeviceCreateBuffer(device, &sDesc);

    skeletonRegistry[id] = res;
    return id;
}

void Engine::CreateRenderInstance(uint32_t meshId, uint32_t skeletonId) {
    gfx::RenderInstance inst;
    inst.meshId = meshId;
    inst.skeletonId = skeletonId;

    // 1. Set default identities to model matrix
    memset(inst.modelMatrix, 0, sizeof(inst.modelMatrix));
    inst.modelMatrix[0] = 1.0f; inst.modelMatrix[5] = 1.0f;
    inst.modelMatrix[10] = 1.0f; inst.modelMatrix[15] = 1.0f;
    inst.colorTint[0] = 1.0f; inst.colorTint[1] = 1.0f; inst.colorTint[2] = 1.0f; inst.colorTint[3] = 1.0f;

    // 2. Allocate Instance Uniform Buffer
    WGPUBufferDescriptor uDesc = {};
    uDesc.size = sizeof(float) * 16 + sizeof(float) * 4; // Model matrix + color tint size
    uDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    inst.instanceUniformBuffer = wgpuDeviceCreateBuffer(device, &uDesc);

    // 3. Create the Bind Group that routes this instance's inputs to our shader slots
    WGPUBindGroupEntry entries[3] = {};
    entries[0].binding = 0;
    entries[0].buffer = globalUniformBuffer;
    entries[0].size = sizeof(gfx::GlobalUniforms);

    entries[1].binding = 1;
    entries[1].buffer = inst.instanceUniformBuffer;
    entries[1].size = uDesc.size;

    entries[2].binding = 2;
    entries[2].buffer = skeletonRegistry[skeletonId].jointStorageBuffer;
    entries[2].size = skeletonRegistry[skeletonId].jointCount * sizeof(float) * 16;

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout = bindGroupLayout;
    bgDesc.entryCount = 3;
    bgDesc.entries = entries;
    inst.bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

    activeSceneObjects.push_back(inst);
}

// ============================================================================
// CORE RUNTIME PATHS
// ============================================================================
void Engine::Update(float deltaTime) {
    // 1. Update Global Uniform Variables (e.g., ticking engine running time)
    globalUniformData.time += deltaTime;
    wgpuQueueWriteBuffer(queue, globalUniformBuffer, 0, &globalUniformData, sizeof(gfx::GlobalUniforms));

    // 2. Loop through active instances to write updated transforms or colors to the GPU
    for (auto& inst : activeSceneObjects) {
        struct InstanceData {
            float model[16];
            float tint[4];
        } data;
        memcpy(data.model, inst.modelMatrix, sizeof(data.model));
        memcpy(data.tint, inst.colorTint, sizeof(data.tint));

        wgpuQueueWriteBuffer(queue, inst.instanceUniformBuffer, 0, &data, sizeof(data));
    }
}

void Engine::Render(WGPUTextureView targetView) {
    WGPUCommandEncoderDescriptor encoderDesc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = targetView;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = WGPUColor{ 0.05, 0.05, 0.06, 1.0 }; // Slate

    WGPURenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.view = depthTextureView;
    depthAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthAttachment.depthClearValue = 1.0f;

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    wgpuRenderPassEncoderSetViewport(pass, 0.0f, 0.0f, width, height, 0.0f, 1.0f);
    wgpuRenderPassEncoderSetPipeline(pass, renderPipeline);

    // Iterative Draw Loop: Draws every object dynamically registered to the scene
    for (const auto& inst : activeSceneObjects) {
        const auto& mesh = meshRegistry[inst.meshId];

        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, mesh.vertexBuffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetIndexBuffer(pass, mesh.indexBuffer, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderSetBindGroup(pass, 0, inst.bindGroup, 0, nullptr);

        wgpuRenderPassEncoderDrawIndexed(pass, mesh.indexCount, 1, 0, 0, 0);
    }

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor cmdDesc = {};
    WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, &cmdDesc);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(queue, 1, &cmd);
    wgpuCommandBufferRelease(cmd);
}

void Engine::Clean() {
    // Release active objects
    for (auto& inst : activeSceneObjects) {
        if (inst.bindGroup) wgpuBindGroupRelease(inst.bindGroup);
        if (inst.instanceUniformBuffer) wgpuBufferRelease(inst.instanceUniformBuffer);
    }
    activeSceneObjects.clear();

    // Clean registries
    for (auto& [id, mesh] : meshRegistry) {
        if (mesh.vertexBuffer) wgpuBufferRelease(mesh.vertexBuffer);
        if (mesh.indexBuffer) wgpuBufferRelease(mesh.indexBuffer);
    }
    meshRegistry.clear();

    for (auto& [id, skel] : skeletonRegistry) {
        if (skel.jointStorageBuffer) wgpuBufferRelease(skel.jointStorageBuffer);
    }
    skeletonRegistry.clear();

    // Clean pipeline
    if (globalUniformBuffer) wgpuBufferRelease(globalUniformBuffer);
    if (depthTextureView) wgpuTextureViewRelease(depthTextureView);
    if (depthTexture) wgpuTextureRelease(depthTexture);
    if (bindGroupLayout) wgpuBindGroupLayoutRelease(bindGroupLayout);
    if (pipelineLayout) wgpuPipelineLayoutRelease(pipelineLayout);
    if (renderPipeline) wgpuRenderPipelineRelease(renderPipeline);
}