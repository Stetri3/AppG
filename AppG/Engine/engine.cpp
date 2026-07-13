#include "Engine.h"
#include "../program.h"
#include <vector>
#include <cmath>
#include <string.h>
#include "../utils.h"

// ============================================================================
// THE WGSL SHADER SYSTEM
// ============================================================================
// Shaders are code executing inside the physical execution blocks of the GPU.
// WebGPU uses WGSL (WebGPU Shading Language) as its modern shading syntax.
const WGPUStringView shaderSource = R"(
// 1. Uniform Struct Blueprint: This MUST perfectly mimic the memory layout 
// of the 'MeshUniforms' structure defined in our C++ header file. 
struct MeshUniforms {
    modelViewProjectionMatrix : mat4x4<f32>,
    customColorTint : vec4<f32>,
    time : f32,
}

// Group 0, Binding 0 matches our pipeline bindings layout layout Entry configurations.
// 'var<uniform>' signals that this variable is static memory per draw instruction.
@group(0) @binding(0) var<uniform> uniforms : MeshUniforms;

// 2. Vertex Inputs structure: maps data coming from the Vertex Buffer attributes.
struct VertexInput {
    @location(0) position : vec3<f32>, // Binds to Location 0 (Position X,Y,Z)
    @location(1) color : vec3<f32>,    // Binds to Location 1 (Color R,G,B)
}

// 3. Output Stage Interface between the Vertex Processor and Fragment Processor.
struct VertexOutput {
    @builtin(position) clip_position : vec4<f32>, // Required: Coordinates in 4D Clip Space
    @location(0) color : vec3<f32>,               // Extrapolated color variable sent down the wire
}

@vertex
fn vs_main(
    model : VertexInput,
    @builtin(instance_index) instanceIdx : u32 // Built-in: tells us which instance copy this vertex belongs to
) -> VertexOutput {
    var out: VertexOutput;
    
    // Procedural Instancing Offset Math:
    // To prevent all 5 triangle duplicates from rendering directly on top of each other,
    // we use the 'instanceIdx' (0, 1, 2, 3, 4) to shift the X and Y coordinates.
    let offsetX = f32(instanceIdx) * 0.25 - 0.5;
    let offsetY = f32(instanceIdx) * 0.15 - 0.3;
    let offset = vec3<f32>(offsetX, offsetY, 0.0);
    
    // Add the structural offset directly to our raw local mesh coordinates
    let shiftedPosition = vec4<f32>(model.position + offset, 1.0);
    
    // The Ultimate 3D-to-2D calculation step:
    // We multiply our 3D coordinates by the transformation matrix passed from the uniform buffer.
    out.clip_position = uniforms.modelViewProjectionMatrix * shiftedPosition;
    
    // Send the color down the pipeline. The GPU rasterizer will automatically interpolate 
    // this color across the face of the triangle before sending it to the fragment shader.
    out.color = model.color;
    return out;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4<f32> {
    // Collect the hardware interpolated vertex gradient color (RGB) 
    // and multiply it by our dynamic oscillating color tint passed from C++.
    let finalColor = vec4<f32>(in.color, 1.0) * uniforms.customColorTint;
    
    // Output the final computed pixel state to color target attachment 0 (our screen surface)
    return finalColor;
}
)"_ws;

// ============================================================================
// ENGINE LIFECYCLE MANAGEMENT
// ============================================================================

// Engine Constructor: Hooks into the initialized core properties of your 'Program' instance
Engine::Engine(Program* p)
    : program(p), device(p->device), queue(nullptr), pipeline(nullptr),
    vertexBuffer(nullptr), uniformBuffer(nullptr), bindGroupLayout(nullptr), bindGroup(nullptr)
{
    // Queries the underlying physical graphics card properties and features 
    // directly through your parent window device context
    wgpuDeviceGetFeatures(device, &devFeatures);
    wgpuDeviceGetLimits(device, &devLimits);
}

// Destructor: Safely de-allocates raw WebGPU handles. 
// WebGPU structures are reference-counted objects managed by the underlying C runtime layer.
Engine::~Engine() {
    if (bindGroup) wgpuBindGroupRelease(bindGroup);
    if (bindGroupLayout) wgpuBindGroupLayoutRelease(bindGroupLayout);
    if (uniformBuffer) wgpuBufferRelease(uniformBuffer);
    if (vertexBuffer) wgpuBufferRelease(vertexBuffer);
    if (pipeline) wgpuRenderPipelineRelease(pipeline);
}

// Initialization Pass: Prepares the static structures needed by the engine to run
void Engine::Initialize() {
    // Grab the queue interface from our engine program container
    queue = program->queue;

    // IMPORTANT PIPELINE SETUP ORDER:
    // 1. Create geometry and allocate data buffers first, so their sizes and handles exist.
    CreateGeometry();

    // 2. Build the shader modules, establish bind layouts, and hook up pipeline blueprints next.
    CreateRenderPipeline();
}

// ============================================================================
// RESOURCE ALLOCATION METHODS
// ============================================================================

void Engine::CreateGeometry() {
    // Define a perfect geometric triangle in local coordinates
    std::vector<Vertex> vertices = {
        // Local Coordinates {X, Y, Z},   Vertex Color Channels {R, G, B}
        { {  0.0f,  0.4f, 0.0f },         { 1.0f, 0.0f, 0.0f } }, // Top point (Solid Red)
        { { -0.4f, -0.4f, 0.0f },         { 0.0f, 1.0f, 0.0f } }, // Bottom-Left (Solid Green)
        { {  0.4f, -0.4f, 0.0f },         { 0.0f, 0.0f, 1.0f } }  // Bottom-Right (Solid Blue)
    };

    // 1. Allocate Vertex Buffer Allocation on the GPU Hardware Frame Buffer
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = vertices.size() * sizeof(Vertex); // Total memory footprints in bytes
    // Usage Flags tell the hardware how to optimize caches:
    // WGPUBufferUsage_Vertex: optimized for raw vertex attribute streams.
    // WGPUBufferUsage_CopyDst: allows writing data into it from the CPU via wgpuQueueWriteBuffer.
    bufferDesc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
    bufferDesc.mappedAtCreation = false;

    vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

    // Synchronously copy our CPU array data over the PCIe bus straight into the GPU memory allocation
    wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertices.data(), bufferDesc.size);

    // 2. Allocate Uniform Buffer Allocation for Dynamic Matrices & Constants
    WGPUBufferDescriptor uniformDesc = {};
    uniformDesc.size = sizeof(MeshUniforms); // Size must be a multiple of 16 bytes for proper WebGPU structure alignment
    // WGPUBufferUsage_Uniform: flag optimizing memory caches for simultaneous access by thousands of shader cores.
    uniformDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    uniformDesc.mappedAtCreation = false;

    uniformBuffer = wgpuDeviceCreateBuffer(device, &uniformDesc);

    // Create a default identity matrix so the old RenderFrame doesn't multiply by all zeros!
    MeshUniforms defaultUniforms = {};
    for (int i = 0; i < 16; i++) {
        defaultUniforms.modelViewProjectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    defaultUniforms.customColorTint[0] = 1.0f; // Red
    defaultUniforms.customColorTint[1] = 1.0f; // Green
    defaultUniforms.customColorTint[2] = 1.0f; // Blue
    defaultUniforms.customColorTint[3] = 1.0f; // Alpha (Solid opacity)

    // Upload the default identity data immediately to the GPU uniform memory slot
    wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &defaultUniforms, sizeof(MeshUniforms));
}

void Engine::CreateRenderPipeline() {
    // 1. Compile Shaders
    WGPUShaderSourceWGSL wgslDesc = {};
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslDesc.code = shaderSource; // Set up explicit Dawn string view wrapper

    WGPUShaderModuleDescriptor shaderDesc = {};
    shaderDesc.nextInChain = &wgslDesc.chain;
    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

    // 2. Configure Vertex Attrib Formatting: Tells the hardware how to parse our byte array into vectors
    std::vector<WGPUVertexAttribute> vertexAttributes(2);

    // Position Attribute Layout (Location 0 in WGSL Input layout)
    vertexAttributes[0].format = WGPUVertexFormat_Float32x3; // Matches float[3]
    vertexAttributes[0].offset = offsetof(Vertex, position); // Byte distance from the start of the struct
    vertexAttributes[0].shaderLocation = 0;

    // Color Attribute Layout (Location 1 in WGSL Input layout)
    vertexAttributes[1].format = WGPUVertexFormat_Float32x3; // Matches float[3]
    vertexAttributes[1].offset = offsetof(Vertex, color);    // Byte offset past position float components
    vertexAttributes[1].shaderLocation = 1;

    WGPUVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = sizeof(Vertex); // Gap size in bytes between individual vertices
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex; // Jump to next index per vertex loop
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();

    // 3. Configure Alpha Transparency Blending rules
    WGPUColorTargetState colorTarget = {};
    colorTarget.format = WGPUTextureFormat_BGRA8Unorm; // Standard swapchain frame texture format
    colorTarget.writeMask = WGPUColorWriteMask_All;   // Allow writes to Red, Green, Blue, and Alpha channels

    // Blending math parameters: FinalColor = (SrcColor * SrcAlpha) + (DstColor * (1 - SrcAlpha))
    // This blend state allows us to see right through the overlapping transparent mesh copies!
    WGPUBlendState blendState = {};
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor = WGPUBlendFactor_One;
    blendState.alpha.dstFactor = WGPUBlendFactor_Zero;
    blendState.alpha.operation = WGPUBlendOperation_Add;
    colorTarget.blend = &blendState;

    WGPUFragmentState fragmentState = {};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main"_ws; // StringView literal for entry marker
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // 4. Create the Bind Group Layout (The structural layout contract)
    WGPUBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0; // Matches @binding(0) in the uniform shader space
    layoutEntry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment; // Visible to both processing blocks
    layoutEntry.buffer.type = WGPUBufferBindingType_Uniform;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = sizeof(MeshUniforms);

    WGPUBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = 1;
    layoutDesc.entries = &layoutEntry;
    bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);

    // Combine all binding layouts layouts arrays into our grand Pipeline Layout Blueprint
    WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {};
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);

    // 5. Finalize the Complete Immutable Pipeline State Object (PSO) Assembly
    WGPURenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.layout = pipelineLayout; // Attach our binding constraints layout

    // Connect Shader Execution Blocks
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main"_ws;
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.fragment = &fragmentState;

    // Define Topology Rules
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
    pipelineDesc.primitive.cullMode = WGPUCullMode_None; // Keep face boundaries double-sided for now

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = 0xFFFFFFFF;

    // Instantiates the permanent state machine engine setup on the graphics card GPU core
    pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

    // Clean up transient compilation pipeline helper objects
    wgpuPipelineLayoutRelease(pipelineLayout);
    wgpuShaderModuleRelease(shaderModule);

    // 6. Connect your allocated GPU buffer to your pipeline's binding slots
    WGPUBindGroupEntry bindGroupEntry = {};
    bindGroupEntry.binding = 0;
    bindGroupEntry.buffer = uniformBuffer; // Route straight to our uniform allocation buffer variable
    bindGroupEntry.offset = 0;
    bindGroupEntry.size = sizeof(MeshUniforms);

    WGPUBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &bindGroupEntry;

    bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

// ============================================================================
// PIPELINE EXECUTION LOOP PASSTHROUGHS
// ============================================================================

void Engine::RenderFrame(WGPUTextureView targetView) {
    // Basic Render Pass left as a fallback path (unbound to the new uniform configuration)
    WGPUCommandEncoderDescriptor encoderDesc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    colorAttachment.view = targetView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.loadOp = WGPULoadOp_Clear;
    colorAttachment.storeOp = WGPUStoreOp_Store;
    colorAttachment.clearValue = WGPUColor{ 0.05, 0.05, 0.05, 1.0 };

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    wgpuRenderPassEncoderSetPipeline(pass, pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertexBuffer, 0, WGPU_WHOLE_SIZE);

    if (bindGroup) { //Fix dato che la pipeline è shared nei due renderFrame
        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
    }

    wgpuRenderPassEncoderDraw(pass, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(queue, 1, &commandBuffer);
    wgpuCommandBufferRelease(commandBuffer);
}

void Engine::RenderFrame2(WGPUTextureView targetView, float currentFrameTime) {
    // -------------------------------------------------------------------------
    // PHASE 1: DYNAMIC MATH COMPUTATIONS & CPU TO GPU STREAMING
    // -------------------------------------------------------------------------
    MeshUniforms frameData = {};

    // Set up a standard 4x4 Identity Matrix. In a real 3D engine, this matrix would be calculated
    // using Projection * View * Model matrix multiplication to handle cameras and positioning.
    for (int i = 0; i < 16; i++) {
        frameData.modelViewProjectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }

    // Procedural color animations: Change the tint dynamically every single frame over running time.
    frameData.customColorTint[0] = 1.0f; // Red channel full blast
    frameData.customColorTint[1] = 0.5f + 0.5f * sinf(currentFrameTime); // Green channel fluctuates cleanly between 0 and 1
    frameData.customColorTint[2] = 0.3f; // Keep blue values steady
    frameData.customColorTint[3] = 0.5f; // Alpha opacity is fixed at 50% translucent transparency!
    frameData.time = currentFrameTime;

    // Stream our freshly calculated frame metadata straight up into the active uniform storage buffer memory allocation
    if (uniformBuffer) {
        wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &frameData, sizeof(MeshUniforms));
    }

    // -------------------------------------------------------------------------
    // PHASE 2: PACKAGING HARDWARE COMMAND BUFFERS
    // -------------------------------------------------------------------------
    WGPUCommandEncoderDescriptor encoderDesc = {};
    // Giving names to encoders makes looking through Dawn native profiling logs significantly easier!
    encoderDesc.label = WGPUStringView{ "Advanced Engine Frame Encoder", strlen("Advanced Engine Frame Encoder") };
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    // Setup color destination targets
    WGPURenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = targetView;
    colorAttachment.resolveTarget = nullptr;
    colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED; // Magic sentinel required to indicate standard 2D targets
    colorAttachment.loadOp = WGPULoadOp_Clear;              // Force frame cache clear prior to script draw invocation
    colorAttachment.storeOp = WGPUStoreOp_Store;            // Lock color buffers in place once drawing routines finish
    colorAttachment.clearValue = WGPUColor{ 0.02, 0.02, 0.04, 1.0 }; // Gorgeous Deep Dark Night Blue Clear Tone

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;

    // Boot the active command writer stream loop
    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    // -------------------------------------------------------------------------
    // PHASE 3: SCISSOR CUTS AND SCREEN SPACE CORRECTIONS
    // -------------------------------------------------------------------------
    // Set Viewport: Maps the geometric NDC space coordinates to physical window pixels.
    wgpuRenderPassEncoderSetViewport(pass, 0.0f, 0.0f, width, height, 0.0f, 1.0f);

    // Set Scissor Rect: A razor-sharp border check. Anything drawn outside these coordinates 
    // is instantly discarded by hardware raster blocks before wasting processing power.
    float capWidth = utils::max_zero(width - 40);
    float capHeight = utils::max_zero(height - 40);
    wgpuRenderPassEncoderSetScissorRect(pass, 20, 20, capWidth, capHeight);

    // -------------------------------------------------------------------------
    // PHASE 4: DISPATCH DRAW SCRIPTS TO HARDWARE GRAPHICS PIPELINE
    // -------------------------------------------------------------------------
    wgpuRenderPassEncoderSetPipeline(pass, pipeline); // Bind the pipeline configuration

    wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertexBuffer, 0, WGPU_WHOLE_SIZE); // Map our raw mesh coordinates array

    if (bindGroup) {
        // Map Uniform memory resources directly to binding locations within the active shader pipeline code
        wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
    }

    // Execute Instanced Multi-Drawing:
    // Arguments: (pass, vertexCount, instanceCount, firstVertex, firstInstance)
    // This draw command tells the GPU to take our 3 triangle vertices and draw them 5 distinct times
    // in a single instruction loop, creating 5 beautiful overlapping transparent triangles on screen!
    wgpuRenderPassEncoderDraw(pass, 3, 5, 0, 0);

    // Close down recorded operations pass channels safely
    wgpuRenderPassEncoderEnd(pass);
    wgpuRenderPassEncoderRelease(pass);

    // Convert recorded instruction paths into an executable Command Buffer object token
    WGPUCommandBufferDescriptor cmdBufferDesc = {};
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdBufferDesc);
    wgpuCommandEncoderRelease(encoder);

    // Instantly queue up execution sequences to be processed directly by the graphics hardware
    wgpuQueueSubmit(queue, 1, &commandBuffer);
    wgpuCommandBufferRelease(commandBuffer);
}