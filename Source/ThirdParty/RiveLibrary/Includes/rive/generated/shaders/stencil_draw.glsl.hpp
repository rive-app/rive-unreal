#pragma once

#include "stencil_draw.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char stencil_draw[] = R"===(/*
 * Copyright 2024 Rive
 */

#ifdef _EXPORTED_VERTEX
ATTR_BLOCK_BEGIN(Attrs)
ATTR(0, packed_float3, _EXPORTED_a_triangleVertex);
ATTR_BLOCK_END

VERTEX_TEXTURE_BLOCK_BEGIN
VERTEX_TEXTURE_BLOCK_END

VERTEX_STORAGE_BUFFER_BLOCK_BEGIN
VERTEX_STORAGE_BUFFER_BLOCK_END

VERTEX_MAIN(_EXPORTED_stencilVertexMain, Attrs, attrs, _vertexID, _instanceID)
{
    float4 pos = RENDER_TARGET_COORD_TO_CLIP_COORD(_EXPORTED_a_triangleVertex.xy);
    uint zIndex = floatBitsToUint(_EXPORTED_a_triangleVertex.z) & 0xffffu;
    pos.z = normalize_z_index(zIndex);
    EMIT_VERTEX(pos);
}
#endif

#ifdef _EXPORTED_FRAGMENT
FRAG_TEXTURE_BLOCK_BEGIN
FRAG_TEXTURE_BLOCK_END

FRAG_DATA_MAIN(half4, _EXPORTED_blitFragmentMain) { EMIT_FRAG_DATA(make_half4(.0)); }
#endif // FRAGMENT
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive