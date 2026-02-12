// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"
#include "RenderResource.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"
#include "UniformBuffer.h"

#include "HLSLTypeAliases.h"
#include "rive/generated/shaders/rhi.glsl.exports.h"
#include "Misc/EngineVersionComparison.h"

#ifndef RIVE_FORCE_USE_GENERATED_UNIFORMS
#define RIVE_FORCE_USE_GENERATED_UNIFORMS 1
#endif

#ifndef ENABLE_TYPED_UAV_LOAD_STORE
#define ENABLE_TYPED_UAV_LOAD_STORE 1
#endif

#ifndef FORCE_ATOMIC_BUFFER
#define FORCE_ATOMIC_BUFFER 0
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogRiveShaderCompiler, Display, All);

#if UE_VERSION_OLDER_THAN(5, 4, 0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip)                            \
    TStaticRasterizerState<FillMode, CullMode, false, false, DepthClip>::      \
        GetRHI()
#define SET_PIPELINE_STATE(CommandList, GraphicsPSOInit)                       \
    SetGraphicsPipelineState(CommandList,                                      \
                             GraphicsPSOInit,                                  \
                             0,                                                \
                             EApplyRendertargetOption::CheckApply,             \
                             true,                                             \
                             EPSOPrecacheResult::Unknown)
#elif UE_VERSION_OLDER_THAN(5, 5, 0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICmdList->CreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICmdList.CreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip, EnableMSAA)                \
    TStaticRasterizerState<FillMode, CullMode, DepthClip, EnableMSAA>::GetRHI()
#define SET_PIPELINE_STATE(CommandList, GraphicsPSOInit)                       \
    SetGraphicsPipelineState(CommandList,                                      \
                             GraphicsPSOInit,                                  \
                             0,                                                \
                             EApplyRendertargetOption::CheckApply,             \
                             true,                                             \
                             EPSOPrecacheResult::Unknown)
#else // UE_VERSION_NEWER_OR_EQUAL_TO (5, 5,0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICmdList.CreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICmdList.CreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip, EnableMSAA)                \
    TStaticRasterizerState<FillMode, CullMode, DepthClip, EnableMSAA>::GetRHI()

#define SET_PIPELINE_STATE(CommandList, GraphicsPSOInit, StencilRef)           \
    SetGraphicsPipelineState(CommandList, GraphicsPSOInit, StencilRef)
#endif

namespace rive::gpu
{
struct DrawBatch;
struct FlushDescriptor;
} // namespace rive::gpu

// shader permutation params
// Whole
class FEnableClip : SHADER_PERMUTATION_BOOL("ENABLE_CLIPPING");
class FEnableClipRect : SHADER_PERMUTATION_BOOL("ENABLE_CLIP_RECT");
class FEnableAdvanceBlend : SHADER_PERMUTATION_BOOL("ENABLE_ADVANCED_BLEND");
class FEnableFeather : SHADER_PERMUTATION_BOOL("ENABLE_FEATHER");

// FragmentOnly
class FEnableFixedFunctionColorOutput
    : SHADER_PERMUTATION_BOOL("FIXED_FUNCTION_COLOR_OUTPUT");
class FEnableHSLBlendMode : SHADER_PERMUTATION_BOOL("ENABLE_HSL_BLEND_MODES");
class FEnableNestedClip : SHADER_PERMUTATION_BOOL("ENABLE_NESTED_CLIPPING");
class FEnableEvenOdd : SHADER_PERMUTATION_BOOL("ENABLE_EVEN_ODD");
class FEnableTypedUAVLoads
    : SHADER_PERMUTATION_BOOL("ENABLE_TYPED_UAV_LOAD_STORE");
class FEnableClockwiseFill : SHADER_PERMUTATION_BOOL("CLOCKWISE_FILL");
class FEnableGammaCorrection
    : SHADER_PERMUTATION_BOOL("NEEDS_GAMMA_CORRECTION");
class FCoalescedPlsResolveAndTransfer
    : SHADER_PERMUTATION_BOOL("COALESCED_PLS_RESOLVE_AND_TRANSFER");

typedef TShaderPermutationDomain<FEnableClip,
                                 FEnableClipRect,
                                 FEnableNestedClip,
                                 FEnableFixedFunctionColorOutput,
                                 FCoalescedPlsResolveAndTransfer,
                                 FEnableAdvanceBlend,
                                 FEnableEvenOdd,
                                 FEnableHSLBlendMode,
                                 FEnableFeather,
                                 FEnableClockwiseFill,
                                 FEnableGammaCorrection>

    AtomicPixelPermutationDomain;
typedef TShaderPermutationDomain<FEnableClip,
                                 FEnableClipRect,
                                 FEnableAdvanceBlend,
                                 FEnableFeather>
    AtomicVertexPermutationDomain;

#define USE_ATOMIC_PIXEL_PERMUTATIONS                                          \
    using FPermutationDomain = AtomicPixelPermutationDomain;

#define USE_ATOMIC_VERTEX_PERMUTATIONS                                         \
    using FPermutationDomain = AtomicVertexPermutationDomain;

BEGIN_UNIFORM_BUFFER_STRUCT(FFlushUniforms, RIVESHADERS_API)
SHADER_PARAMETER(float, gradInverseViewportY)
SHADER_PARAMETER(float, tessInverseViewportY)
SHADER_PARAMETER(float, renderTargetInverseViewportX)
SHADER_PARAMETER(float, renderTargetInverseViewportY)
SHADER_PARAMETER(UE::HLSL::uint, renderTargetWidth)
SHADER_PARAMETER(UE::HLSL::uint, renderTargetHeight)
SHADER_PARAMETER(
    UE::HLSL::uint,
    colorClearValue) // Only used if clears are implemented as draws.
SHADER_PARAMETER(
    UE::HLSL::uint,
    coverageClearValue) // Only used if clears are implemented as draws.
SHADER_PARAMETER(UE::HLSL::int4,
                 renderTargetUpdateBounds) // drawBounds, or renderTargetBounds
                                           // if there is a clear. (LTRB.)
SHADER_PARAMETER(UE::HLSL::float2,
                 atlasTextureInverseSize) // 1 / [atlasWidth, atlasHeight]
SHADER_PARAMETER(UE::HLSL::float2,
                 atlasContentInverseViewport) // 2 / atlasContentBounds
SHADER_PARAMETER(UE::HLSL::uint, coverageBufferPrefix)
SHADER_PARAMETER(UE::HLSL::uint,
                 pathIDGranularity) // Spacing between adjacent path IDs (1 if
                                    // IEEE compliant).
SHADER_PARAMETER(float, vertexDiscardValue)
SHADER_PARAMETER(float, mipMapLODBias)
SHADER_PARAMETER(UE::HLSL::uint, maxPathId)
SHADER_PARAMETER(float, ditherScale)
SHADER_PARAMETER(float, ditherBias)
// Debugging.
SHADER_PARAMETER(UE::HLSL::uint, wireframeEnabled)
END_UNIFORM_BUFFER_STRUCT();

RIVESHADERS_API void BindStaticFlushUniforms(FRHICommandList&,
                                             FUniformBufferRHIRef);

BEGIN_UNIFORM_BUFFER_STRUCT(FImageDrawUniforms, RIVESHADERS_API)
SHADER_PARAMETER(UE::HLSL::float4, viewMatrix)
SHADER_PARAMETER(UE::HLSL::float2, translate)
SHADER_PARAMETER(float, opacity)
SHADER_PARAMETER(float, padding)
SHADER_PARAMETER(UE::HLSL::float4, clipRectInverseMatrix)
SHADER_PARAMETER(UE::HLSL::float2, clipRectInverseTranslate)
SHADER_PARAMETER(UE::HLSL::uint, clipID)
SHADER_PARAMETER(UE::HLSL::uint, blendMode)
SHADER_PARAMETER(UE::HLSL::uint, zIndex)
END_UNIFORM_BUFFER_STRUCT()

// One shared uniform struct used for all atomic frament shaders besides
// gradient and tesselation. Params not used will be marked null
BEGIN_SHADER_PARAMETER_STRUCT(FRivePixelDrawUniforms, RIVESHADERS_API)
#if !UE_VERSION_OLDER_THAN(5, 5, 0)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)

SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray<float>, GLSL_featherTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float>, GLSL_atlasTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_imageTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2DMS, GLSL_dstColorTexture_raw)

SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageCountBuffer)
SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageAtomicBuffer)

SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)
SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, scratchColorBuffer)
SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, atlasSampler)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                GLSL_paintAuxBuffer_raw)

END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FRivePixelDrawUniformsAtomicBuffers,
                              RIVESHADERS_API)
#if !UE_VERSION_OLDER_THAN(5, 5, 0)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)

SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray<float>, GLSL_featherTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float>, GLSL_atlasTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_imageTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2DMS, GLSL_dstColorTexture_raw)

SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, coverageCountBuffer)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<uint>, coverageAtomicBuffer)

SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)
SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, scratchColorBuffer)
SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, atlasSampler)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                GLSL_paintAuxBuffer_raw)

END_SHADER_PARAMETER_STRUCT()

// One shared uniform struct used for all atomic vertex shaders besides gradient
// and tesselation. Params not used will be marked null

BEGIN_SHADER_PARAMETER_STRUCT(FRiveVertexDrawUniforms, RIVESHADERS_API)
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)
SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray<float>, GLSL_featherTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D<uint4>, GLSL_tessVertexTexture_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>, GLSL_pathBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>, GLSL_contourBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                GLSL_paintAuxBuffer_raw)
SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)
SHADER_PARAMETER(unsigned int, baseInstance)
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FRiveMSAAPixelDrawUniforms, RIVESHADERS_API)
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)

SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray<float>, GLSL_featherTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float>, GLSL_atlasTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_imageTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2DMS, GLSL_dstColorTexture_raw)

SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)
SHADER_PARAMETER_SAMPLER(SamplerState, atlasSampler)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                GLSL_paintAuxBuffer_raw)

END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FRiveMSAAVertexDrawUniforms, RIVESHADERS_API)
#if !UE_VERSION_OLDER_THAN(5, 5, 0)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray<float>, GLSL_featherTexture_raw)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D<uint4>, GLSL_tessVertexTexture_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>, GLSL_pathBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>, GLSL_contourBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                GLSL_paintAuxBuffer_raw)
SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)
SHADER_PARAMETER(unsigned int, baseInstance)
END_SHADER_PARAMETER_STRUCT()

static constexpr bool IsTargetVulkan(
    const FShaderPermutationParameters& Parameters)
{
    return Parameters.Platform == SP_VULKAN_PCES3_1 ||
           Parameters.Platform == SP_VULKAN_ES3_1_ANDROID ||
           Parameters.Platform == SP_VULKAN_SM5 ||
           Parameters.Platform == SP_VULKAN_SM5_ANDROID ||
           Parameters.Platform == SP_VULKAN_SM6;
}

static constexpr bool IsTargetMetal(
    const FShaderPermutationParameters& Parameters)
{
    return Parameters.Platform == SP_METAL_SM5 ||
           Parameters.Platform == SP_METAL_SM5_IOS ||
           Parameters.Platform == SP_METAL_SM5_TVOS ||
           Parameters.Platform == SP_METAL_SM6;
}

template <typename ShaderClass>
static bool RiveShouldCompilePixelPermutation(
    const FShaderPermutationParameters& Parameters,
    bool isResolve = false,
    bool isAtomicBuffer = false)
{
    typename ShaderClass::FPermutationDomain PermutationVector(
        Parameters.PermutationId);

    if (isAtomicBuffer && !IsTargetMetal(Parameters) &&
        Parameters.Platform != 39)
        return false;

    if (!isAtomicBuffer &&
        (IsTargetMetal(Parameters) || Parameters.Platform == 39))
        return false;

    if (PermutationVector.template Get<FCoalescedPlsResolveAndTransfer>() &&
        !isResolve)
        return false;

    if (PermutationVector.template Get<FEnableHSLBlendMode>() &&
        !PermutationVector.template Get<FEnableAdvanceBlend>())
    {
        return false;
    }

    if (PermutationVector.template Get<FEnableEvenOdd>() &&
        PermutationVector.template Get<FEnableClockwiseFill>())
    {
        return false;
    }

    if (PermutationVector.template Get<FEnableFixedFunctionColorOutput>() &&
        PermutationVector.template Get<FEnableAdvanceBlend>())
    {
        return false;
    }

    if (PermutationVector.template Get<FCoalescedPlsResolveAndTransfer>() &&
        PermutationVector.template Get<FEnableFixedFunctionColorOutput>())
    {
        return false;
    }

    UE_LOG(LogRiveShaderCompiler,
           VeryVerbose,
           TEXT("Building Permutation %i for FRiveRDGPathPixelShader"),
           Parameters.PermutationId)
    return true;
}

template <typename ShaderClass>
bool ShouldCompilePixelMSAAPermutation(
    const FShaderPermutationParameters& Parameters)
{
    return RiveShouldCompilePixelPermutation<ShaderClass>(Parameters);
}

class FRiveBasePixelShader : public FGlobalShader
{
public:
    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveBaseVertexShader : public FGlobalShader
{
public:
    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGGradientPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGGradientPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGGradientPixelShader,
                                FRiveBasePixelShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
    END_SHADER_PARAMETER_STRUCT()
};

class FRiveRDGGradientVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGGradientVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGGradientVertexShader,
                                FRiveBaseVertexShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
    END_SHADER_PARAMETER_STRUCT()
};

class FRiveRDGTessPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGTessPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGTessPixelShader, FRiveBasePixelShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGTessVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGTessVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGTessVertexShader,
                                FRiveBaseVertexShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_pathBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_contourBuffer_raw)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2DArray<float>, GLSL_featherTexture_raw)
    SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)

    END_SHADER_PARAMETER_STRUCT()
};

// Atomic Shaders

class FRiveRDGPathPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGPathPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGPathPixelShader, FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGPathPixelShader>(
            Parameters);
    }
};

class FRiveABRDGPathPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveABRDGPathPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveABRDGPathPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniformsAtomicBuffers;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGPathPixelShader>(
            Parameters,
            false,
            true);
    }
};

class FRiveRDGPathVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGPathVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGPathVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGInteriorTrianglesPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGInteriorTrianglesPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGInteriorTrianglesPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGInteriorTrianglesPixelShader>(Parameters);
    }
};

class FRiveABRDGInteriorTrianglesPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveABRDGInteriorTrianglesPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveABRDGInteriorTrianglesPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniformsAtomicBuffers;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGInteriorTrianglesPixelShader>(Parameters, false, true);
    }
};

class FRiveRDGInteriorTrianglesVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGInteriorTrianglesVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGInteriorTrianglesVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGAtlasBlitPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtlasBlitPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtlasBlitPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGInteriorTrianglesPixelShader>(Parameters);
    }
};

class FRiveABRDGAtlasBlitPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveABRDGAtlasBlitPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveABRDGAtlasBlitPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniformsAtomicBuffers;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGInteriorTrianglesPixelShader>(Parameters, false, true);
    }
};

class FRiveRDGAtlasBlitVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtlasBlitVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtlasBlitVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGImageRectPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageRectPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageRectPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGImageRectPixelShader>(
            Parameters);
    }
};

class FRiveABRDGImageRectPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveABRDGImageRectPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveABRDGImageRectPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniformsAtomicBuffers;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGImageRectPixelShader>(
            Parameters,
            false,
            true);
    }
};

class FRiveRDGImageRectVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageRectVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageRectVertexShader,
                                FRiveBaseVertexShader);

    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGImageMeshPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageMeshPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageMeshPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGImageMeshPixelShader>(
            Parameters);
    }
};

class FRiveABRDGImageMeshPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveABRDGImageMeshPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveABRDGImageMeshPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniformsAtomicBuffers;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGImageMeshPixelShader>(
            Parameters,
            false,
            true);
    }
};

class FRiveRDGImageMeshVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageMeshVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageMeshVertexShader,
                                FRiveBaseVertexShader);

    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGAtomicResolvePixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtomicResolvePixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtomicResolvePixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGAtomicResolvePixelShader>(Parameters, true);
    }
};

class FRiveABRDGAtomicResolvePixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveABRDGAtomicResolvePixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveABRDGAtomicResolvePixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniformsAtomicBuffers;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGAtomicResolvePixelShader>(Parameters, true, true);
    }
};

class FRiveRDGAtomicResolveVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtomicResolveVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtomicResolveVertexShader,
                                FRiveBaseVertexShader);

    USE_ATOMIC_VERTEX_PERMUTATIONS

    using FParameters = FRiveVertexDrawUniforms;
};

// Raster Ordered Shaders

class FRiveRDGRasterOrderPathPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGRasterOrderPathPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGRasterOrderPathPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGPathPixelShader>(
            Parameters);
    }
};

class FRiveRDGRasterOrderPathVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGRasterOrderPathVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGRasterOrderPathVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGRasterOrderInteriorTrianglesPixelShader
    : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(
        FRiveRDGRasterOrderInteriorTrianglesPixelShader,
        RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGRasterOrderInteriorTrianglesPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<
            FRiveRDGInteriorTrianglesPixelShader>(Parameters);
    }
};

class FRiveRDGRasterOrderInteriorTrianglesVertexShader
    : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(
        FRiveRDGRasterOrderInteriorTrianglesVertexShader,
        RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(
        FRiveRDGRasterOrderInteriorTrianglesVertexShader,
        FRiveBaseVertexShader);
    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGRasterOrderImageMeshPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGRasterOrderImageMeshPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGRasterOrderImageMeshPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRivePixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return RiveShouldCompilePixelPermutation<FRiveRDGImageMeshPixelShader>(
            Parameters);
    }
};

class FRiveRDGRasterOrderImageMeshVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGRasterOrderImageMeshVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGRasterOrderImageMeshVertexShader,
                                FRiveBaseVertexShader);

    using FParameters = FRiveVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

// MSAA Shaders

class FRiveRDGAtlasBlitMSAAVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtlasBlitMSAAVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtlasBlitMSAAVertexShader,
                                FRiveBaseVertexShader);

    using FParameters = FRiveMSAAVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGAtlasBlitMSAAPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtlasBlitMSAAPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtlasBlitMSAAPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRiveMSAAPixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return ShouldCompilePixelMSAAPermutation<
            FRiveRDGAtlasBlitMSAAPixelShader>(Parameters);
    }
};

class FRiveRDGImageMeshMSAAVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageMeshMSAAVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageMeshMSAAVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveMSAAVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGImageMeshMSAAPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageMeshMSAAPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageMeshMSAAPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRiveMSAAPixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return ShouldCompilePixelMSAAPermutation<
            FRiveRDGImageMeshMSAAPixelShader>(Parameters);
    }
};

class FRiveRDGPathMSAAVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGPathMSAAVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGPathMSAAVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveMSAAVertexDrawUniforms;

    USE_ATOMIC_VERTEX_PERMUTATIONS
};

class FRiveRDGPathMSAAPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGPathMSAAPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGPathMSAAPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRiveMSAAPixelDrawUniforms;

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return ShouldCompilePixelMSAAPermutation<FRiveRDGPathMSAAPixelShader>(
            Parameters);
    }
};

class FRiveRDGStencilMSAAVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGStencilMSAAVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGStencilMSAAVertexShader,
                                FRiveBaseVertexShader);
    using FParameters = FRiveMSAAVertexDrawUniforms;
};

class FRiveRDGStencilMSAAPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGStencilMSAAPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGStencilMSAAPixelShader,
                                FRiveBasePixelShader);
    using FParameters = FRiveMSAAPixelDrawUniforms;
};

// Atlas Render

class FRiveRDGDrawAtlasVertexShader : public FRiveBaseVertexShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGDrawAtlasVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGDrawAtlasVertexShader,
                                FRiveBaseVertexShader);

    using FParameters = FRiveVertexDrawUniforms;
};

BEGIN_SHADER_PARAMETER_STRUCT(FAtlasDrawUniforms, RIVESHADERS_API)
#if !UE_VERSION_OLDER_THAN(5, 5, 0)
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, GLSL_FlushUniforms_raw)
#endif
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_featherTexture_raw)
SHADER_PARAMETER_SAMPLER(SamplerState, featherSampler)
END_SHADER_PARAMETER_STRUCT()

class FRiveRDGDrawAtlasFillPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGDrawAtlasFillPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGDrawAtlasFillPixelShader,
                                FRiveBasePixelShader);

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);

    using FParameters = FAtlasDrawUniforms;
};

class FRiveRDGDrawAtlasStrokePixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGDrawAtlasStrokePixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGDrawAtlasStrokePixelShader,
                                FRiveBasePixelShader);

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);

    using FParameters = FAtlasDrawUniforms;
};

class FRiveBltTextureAsDrawVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveBltTextureAsDrawVertexShader,
                                   RIVESHADERS_API);

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};
class FEnableMSAASourceTexture : SHADER_PERMUTATION_BOOL("SOURCE_TEXTURE_MSAA");

BEGIN_SHADER_PARAMETER_STRUCT(FRiveBltTextureDrawUniforms, RIVESHADERS_API)
SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_sourceTexture_raw)
END_SHADER_PARAMETER_STRUCT()

class FRiveBltTextureAsDrawPixelShader : public FRiveBasePixelShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveBltTextureAsDrawPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveBltTextureAsDrawPixelShader,
                                FRiveBasePixelShader);

    using FParameters = FRiveBltTextureDrawUniforms;
    using FPermutationDomain =
        TShaderPermutationDomain<FEnableMSAASourceTexture>;
};

/*
 * The shader will convert a PF_R32 texture to PF_R8G8B8A8 in order to visualize
 * it. It does so by writing SourceTexture to the current rendertarget color
 * float4(value, 0, 0, 1)
 */
class FRiveRDGBltU32AsF4PixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGBltU32AsF4PixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGBltU32AsF4PixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

/*
 * The shader will convert a PF_R32G32B32A32_UINT texture to PF_R8G8B8A8 in
 * order to visualize it. It does so by writing SourceTexture to the current
 * rendertarget color float4( float(val.r), float(val.g), float(val.b), 1.0 );
 */
class FRiveRDGBltU324AsF4PixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGBltU324AsF4PixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGBltU324AsF4PixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SourceTexture)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

/*
 * The shader will convert a uint2 buffer to PF_R8G8B8A8 in order to visualize
 * it. it does so in a slightly more intelligent way by passing the values of
 * buffer through sin and cos when writing it to the bound render target
 */
class FRiveRDGVisualizeBufferPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGVisualizeBufferPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGVisualizeBufferPixelShader,
                                FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_BUFFER_SRV(Buffer<uint2>, SourceBuffer)
    SHADER_PARAMETER(UE::HLSL::uint2, ViewSize)
    SHADER_PARAMETER(UE::HLSL::uint, BufferSize)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};
