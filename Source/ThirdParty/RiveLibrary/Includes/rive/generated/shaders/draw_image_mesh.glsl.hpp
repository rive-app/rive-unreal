#pragma once

#include "draw_image_mesh.exports.h"

namespace rive {
namespace gpu {
namespace glsl {
const char draw_image_mesh[] = R"===(/*
 * Copyright 2023 Rive
 */

#ifdef _EXPORTED_VERTEX
ATTR_BLOCK_BEGIN(PositionAttr)
ATTR(0, float2, _EXPORTED_a_position);
ATTR_BLOCK_END

ATTR_BLOCK_BEGIN(UVAttr)
ATTR(1, float2, _EXPORTED_a_texCoord);
ATTR_BLOCK_END
#endif

VARYING_BLOCK_BEGIN
NO_PERSPECTIVE VARYING(0, float2, v_texCoord);
#ifdef _EXPORTED_ENABLE_CLIPPING
_EXPORTED_OPTIONALLY_FLAT VARYING(1, half, v_clipID);
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
NO_PERSPECTIVE VARYING(2, float4, v_clipRect);
#endif
VARYING_BLOCK_END

#ifdef _EXPORTED_VERTEX
VERTEX_TEXTURE_BLOCK_BEGIN
VERTEX_TEXTURE_BLOCK_END

IMAGE_MESH_VERTEX_MAIN(_EXPORTED_drawVertexMain, PositionAttr, position, UVAttr, uv, _vertexID)
{
    ATTR_UNPACK(_vertexID, position, _EXPORTED_a_position, float2);
    ATTR_UNPACK(_vertexID, uv, _EXPORTED_a_texCoord, float2);

    VARYING_INIT(v_texCoord, float2);
#ifdef _EXPORTED_ENABLE_CLIPPING
    VARYING_INIT(v_clipID, half);
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
    VARYING_INIT(v_clipRect, float4);
#endif

    float2 vertexPosition =
        MUL(make_float2x2(imageDrawUniforms.viewMatrix), _EXPORTED_a_position) + imageDrawUniforms.translate;
    v_texCoord = _EXPORTED_a_texCoord;
#ifdef _EXPORTED_ENABLE_CLIPPING
    if (_EXPORTED_ENABLE_CLIPPING)
    {
        v_clipID = id_bits_to_f16(imageDrawUniforms.clipID, uniforms.pathIDGranularity);
    }
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
    if (_EXPORTED_ENABLE_CLIP_RECT)
    {
#ifndef _EXPORTED_USING_DEPTH_STENCIL
        v_clipRect = find_clip_rect_coverage_distances(
            make_float2x2(imageDrawUniforms.clipRectInverseMatrix),
            imageDrawUniforms.clipRectInverseTranslate,
            vertexPosition);
#else  // USING_DEPTH_STENCIL
        set_clip_rect_plane_distances(make_float2x2(imageDrawUniforms.clipRectInverseMatrix),
                                      imageDrawUniforms.clipRectInverseTranslate,
                                      vertexPosition);
#endif // USING_DEPTH_STENCIL
    }
#endif // ENABLE_CLIP_RECT
    float4 pos = RENDER_TARGET_COORD_TO_CLIP_COORD(vertexPosition);
#ifdef _EXPORTED_USING_DEPTH_STENCIL
    pos.z = normalize_z_index(imageDrawUniforms.zIndex);
#endif

    VARYING_PACK(v_texCoord);
#ifdef _EXPORTED_ENABLE_CLIPPING
    VARYING_PACK(v_clipID);
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
    VARYING_PACK(v_clipRect);
#endif
    EMIT_VERTEX(pos);
}
#endif

#ifdef _EXPORTED_FRAGMENT
FRAG_TEXTURE_BLOCK_BEGIN
TEXTURE_RGBA8(PER_DRAW_BINDINGS_SET, IMAGE_TEXTURE_IDX, _EXPORTED_imageTexture);
#ifdef _EXPORTED_USING_DEPTH_STENCIL
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
TEXTURE_RGBA8(PER_FLUSH_BINDINGS_SET, DST_COLOR_TEXTURE_IDX, _EXPORTED_dstColorTexture);
#endif
#endif
FRAG_TEXTURE_BLOCK_END

SAMPLER_MIPMAP(IMAGE_TEXTURE_IDX, imageSampler)

FRAG_STORAGE_BUFFER_BLOCK_BEGIN
FRAG_STORAGE_BUFFER_BLOCK_END

#ifndef _EXPORTED_USING_DEPTH_STENCIL

PLS_BLOCK_BEGIN
PLS_DECL4F(COLOR_PLANE_IDX, colorBuffer);
#if defined(_EXPORTED_ENABLE_CLIPPING) || defined(_EXPORTED_PLS_IMPL_ANGLE)
PLS_DECLUI(CLIP_PLANE_IDX, clipBuffer);
#endif
PLS_DECL4F(SCRATCH_COLOR_PLANE_IDX, scratchColorBuffer);
PLS_DECLUI(COVERAGE_PLANE_IDX, coverageCountBuffer);
PLS_BLOCK_END

PLS_MAIN_WITH_IMAGE_UNIFORMS(_EXPORTED_drawFragmentMain)
{
    VARYING_UNPACK(v_texCoord, float2);
#ifdef _EXPORTED_ENABLE_CLIPPING
    VARYING_UNPACK(v_clipID, half);
#endif
#ifdef _EXPORTED_ENABLE_CLIP_RECT
    VARYING_UNPACK(v_clipRect, float4);
#endif

    half4 color = TEXTURE_SAMPLE(_EXPORTED_imageTexture, imageSampler, v_texCoord);
    half coverage = 1.;

#ifdef _EXPORTED_ENABLE_CLIP_RECT
    if (_EXPORTED_ENABLE_CLIP_RECT)
    {
        half clipRectCoverage = min_value(cast_float4_to_half4(v_clipRect));
        coverage = clamp(clipRectCoverage, make_half(.0), coverage);
    }
#endif

    PLS_INTERLOCK_BEGIN;

#ifdef _EXPORTED_ENABLE_CLIPPING
    if (_EXPORTED_ENABLE_CLIPPING && v_clipID != .0)
    {
        half2 clipData = unpackHalf2x16(PLS_LOADUI(clipBuffer));
        half clipContentID = clipData.y;
        half clipCoverage = clipContentID == v_clipID ? clipData.x : make_half(.0);
        coverage = min(coverage, clipCoverage);
    }
#endif

    // Blend with the framebuffer color.
    color.w *= imageDrawUniforms.opacity * coverage;
    half4 dstColor = PLS_LOAD4F(colorBuffer);
#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
    if (_EXPORTED_ENABLE_ADVANCED_BLEND && imageDrawUniforms.blendMode != BLEND_SRC_OVER)
    {
        color = advanced_blend(color,
                               unmultiply(dstColor),
                               cast_uint_to_ushort(imageDrawUniforms.blendMode));
    }
    else
#endif
    {
        color.xyz *= color.w;
        color = color + dstColor * (1. - color.w);
    }

    PLS_STORE4F(colorBuffer, color);
#ifdef _EXPORTED_ENABLE_CLIPPING
    PLS_PRESERVE_UI(clipBuffer);
#endif

    PLS_INTERLOCK_END;

    EMIT_PLS;
}

#else // USING_DEPTH_STENCIL

FRAG_DATA_MAIN(half4, _EXPORTED_drawFragmentMain)
{
    VARYING_UNPACK(v_texCoord, float2);

    half4 color = TEXTURE_SAMPLE(_EXPORTED_imageTexture, imageSampler, v_texCoord);
    color.w *= imageDrawUniforms.opacity;

#ifdef _EXPORTED_ENABLE_ADVANCED_BLEND
    if (_EXPORTED_ENABLE_ADVANCED_BLEND)
    {
        half4 dstColor = TEXEL_FETCH(_EXPORTED_dstColorTexture, int2(floor(_fragCoord.xy)));
        color = advanced_blend(color, unmultiply(dstColor), imageDrawUniforms.blendMode);
    }
    else
#endif // !ENABLE_ADVANCED_BLEND
    {
        color = premultiply(color);
    }

    EMIT_FRAG_DATA(color);
}

#endif // USING_DEPTH_STENCIL
#endif // FRAGMENT
)===";
} // namespace glsl
} // namespace gpu
} // namespace rive