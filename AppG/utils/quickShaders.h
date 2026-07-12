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