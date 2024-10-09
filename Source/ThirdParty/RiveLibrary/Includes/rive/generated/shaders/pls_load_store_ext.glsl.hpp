#pragma once

#include "pls_load_store_ext.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char pls_load_store_ext[] = R"===(/*
 * Copyright 2022 Rive
 */

// The EXT_shader_pixel_local_storage extension does not provide a mechanism to load, store, or
// clear pixel local storage contents. This shader performs custom load, store, and clear
// operations via fullscreen draws.

#ifdef _EXPORTED_VERTEX
void main()
{
    // [-1, -1] .. [+1, +1]
    gl_Position =
        vec4(mix(vec2(-1, 1), vec2(1, -1), equal(gl_VertexID & ivec2(1, 2), ivec2(0))), 0, 1);
}
#endif

#ifdef _EXPORTED_FRAGMENT

#extension GL_EXT_shader_pixel_local_storage : enable
#extension GL_ARM_shader_framebuffer_fetch : enable
#extension GL_EXT_shader_framebuffer_fetch : enable

#ifdef _EXPORTED_CLEAR_COLOR
#if __VERSION__ >= 310
layout(binding = 0, std140) uniform ClearColor { uniform highp vec4 value; }
clearColor;
#else
uniform mediump vec4 _EXPORTED_clearColor;
#endif
#endif

#ifdef GL_EXT_shader_pixel_local_storage

#ifdef _EXPORTED_STORE_COLOR
__pixel_local_inEXT PLS
#else
__pixel_local_outEXT PLS
#endif
{
    layout(rgba8) mediump vec4 colorBuffer;
#ifdef _EXPORTED_ENABLE_CLIPPING
    layout(r32ui) highp uint clipBuffer;
#endif
    layout(rgba8) mediump vec4 scratchColorBuffer;
    layout(r32ui) highp uint coverageCountBuffer;
};

#ifndef GL_ARM_shader_framebuffer_fetch
#ifdef _EXPORTED_LOAD_COLOR
layout(location = 0) inout mediump vec4 fragColor;
#endif
#endif

#ifdef _EXPORTED_STORE_COLOR
layout(location = 0) out mediump vec4 fragColor;
#endif

void main()
{
#ifdef _EXPORTED_CLEAR_COLOR
#if __VERSION__ >= 310
    colorBuffer = clearColor.value;
#else
    colorBuffer = _EXPORTED_clearColor;
#endif
#endif

#ifdef _EXPORTED_LOAD_COLOR
#ifdef GL_ARM_shader_framebuffer_fetch
    colorBuffer = gl_LastFragColorARM;
#else
    colorBuffer = fragColor;
#endif
#endif

#ifdef _EXPORTED_CLEAR_COVERAGE
    coverageCountBuffer = 0u;
#endif

#ifdef _EXPORTED_CLEAR_CLIP
    clipBuffer = 0u;
#endif

#ifdef _EXPORTED_STORE_COLOR
    fragColor = colorBuffer;
#endif
}

#else

// This shader is being parsed by WebGPU for introspection purposes.
layout(location = 0) out mediump vec4 unused;
void main() { unused = vec4(0, 1, 0, 1); }

#endif // GL_EXT_shader_pixel_local_storage

#endif // FRAGMENT
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive