#pragma once
#include <dawn/webgpu.h>

constexpr WGPUStringView shader_test = R"(
    @vertex
    fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
        var p = vec2f(0.0, 0.0);
        if (in_vertex_index == 0u) { p = vec2f( 0.0,  0.5); }
        if (in_vertex_index == 1u) { p = vec2f(-0.5, -0.5); }
        if (in_vertex_index == 2u) { p = vec2f( 0.5, -0.5); }
        return vec4f(p, 0.0, 1.0);
    }

    @fragment
    fn fs_main() -> @location(0) vec4f {
        return vec4f(1.0, 0.0, 0.0, 1.0);
    }
)"_ws;
constexpr WGPUStringView shaderSource = R"(
struct VertexInput {
    @location(0) position : vec3<f32>,
    @location(1) color : vec3<f32>,
};

struct VertexOutput {
    @builtin(position) clip_position : vec4<f32>,
    @location(0) color : vec3<f32>,
};

@vertex
fn vs_main(model : VertexInput) -> VertexOutput {
    var out: VertexOutput;
    
    // For this simple demo, we pass 3D coordinates directly into 4D Clip Space.
    // In a full implementation, you would multiply this by your Projection matrix here.
    out.clip_position = vec4<f32>(model.position, 1.0);
    out.color = model.color;
    return out;
}

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4<f32> {
    // Outputs the interpolated vertex color directly to the texture target
    return vec4<f32>(in.color, 1.0);
}
)"_ws;