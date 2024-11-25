#pragma once
#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"
#include "RenderResource.h"
#include "RHI.h"
#include "GlobalShader.h"

#include "HLSLTypeAliases.h"
#include "rive/generated/shaders/rhi.exports.h"

namespace rive::gpu
{
struct DrawBatch;
struct FlushDescriptor;
} // namespace rive::gpu

#ifndef RIVESHADERS_API
#define RIVESHADERS_API
#endif

// shader permutation params
// Whole
class FEnableClip : SHADER_PERMUTATION_BOOL("ENABLE_CLIPPING");
class FEnableClipRect : SHADER_PERMUTATION_BOOL("ENABLE_CLIP_RECT");
class FEnableAdvanceBlend : SHADER_PERMUTATION_BOOL("ENABLE_ADVANCED_BLEND");

// FragmentOnly
class FEnableFixedFunctionColorOutput
    : SHADER_PERMUTATION_BOOL("FIXED_FUNCTION_COLOR_OUTPUT");
class FEnableHSLBlendMode : SHADER_PERMUTATION_BOOL("ENABLE_HSL_BLEND_MODES");
class FEnableNestedClip : SHADER_PERMUTATION_BOOL("ENABLE_NESTED_CLIPPING");
class FEnableEvenOdd : SHADER_PERMUTATION_BOOL("ENABLE_EVEN_ODD");

typedef TShaderPermutationDomain<FEnableClip,
                                 FEnableClipRect,
                                 FEnableNestedClip,
                                 FEnableFixedFunctionColorOutput,
                                 FEnableAdvanceBlend,
                                 FEnableEvenOdd,
                                 FEnableHSLBlendMode>
    AtomicPixelPermutationDomain;
typedef TShaderPermutationDomain<FEnableClip,
                                 FEnableClipRect,
                                 FEnableAdvanceBlend>
    AtomicVertexPermutationDomain;

#define USE_ATOMIC_PIXEL_PERMUTATIONS                                          \
    using FPermutationDomain = AtomicPixelPermutationDomain;

#define USE_ATOMIC_VERTEX_PERMUTATIONS                                         \
    using FPermutationDomain = AtomicVertexPermutationDomain;

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FFlushUniforms, RIVESHADERS_API)
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
SHADER_PARAMETER(UE::HLSL::uint,
                 pathIDGranularity) // Spacing between adjacent path IDs (1 if
                                    // IEEE compliant).
SHADER_PARAMETER(float, vertexDiscardValue)
END_GLOBAL_SHADER_PARAMETER_STRUCT();

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FImageDrawUniforms, RIVESHADERS_API)
SHADER_PARAMETER(UE::HLSL::float4, viewMatrix)
SHADER_PARAMETER(UE::HLSL::float2, translate)
SHADER_PARAMETER(float, opacity)
SHADER_PARAMETER(float, padding)
SHADER_PARAMETER(UE::HLSL::float4, clipRectInverseMatrix)
SHADER_PARAMETER(UE::HLSL::float2, clipRectInverseTranslate)
SHADER_PARAMETER(UE::HLSL::uint, clipID)
SHADER_PARAMETER(UE::HLSL::uint, blendMode)
SHADER_PARAMETER(UE::HLSL::uint, zIndex)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

class FRiveGradientPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveGradientPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveGradientPixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveGradientVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveGradientVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveGradientVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveTessPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveTessPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveTessPixelShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveTessVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveTessVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveTessVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_SRV(StructuredBuffer<uint4>, GLSL_pathBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<uint4>, GLSL_contourBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRivePathPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRivePathPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRivePathPixelShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)

    SHADER_PARAMETER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return true;
    }
};

class FRivePathVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRivePathVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRivePathVertexShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)

    SHADER_PARAMETER_SRV(Texture2D<uint4>, GLSL_tessVertexTexture_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<uint4>, GLSL_pathBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<uint4>, GLSL_contourBuffer_raw)
    SHADER_PARAMETER(unsigned int, baseInstance)

    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveInteriorTrianglesPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveInteriorTrianglesPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveInteriorTrianglesPixelShader,
                                FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)

    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)

    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)

    SHADER_PARAMETER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<float4>, GLSL_paintAuxBuffer_raw)

    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveInteriorTrianglesVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveInteriorTrianglesVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveInteriorTrianglesVertexShader,
                                FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_SRV(StructuredBuffer<uint4>, GLSL_pathBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveImageRectPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveImageRectPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageRectPixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)

    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_imageTexture_raw)

    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)

    SHADER_PARAMETER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveImageRectVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveImageRectVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageRectVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveImageMeshPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveImageMeshPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageMeshPixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)

    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_imageTexture_raw)

    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)

    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)

    SHADER_PARAMETER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveImageMeshVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveImageMeshVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageMeshVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveAtomicResolvePixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveAtomicResolvePixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveAtomicResolvePixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SRV(StructuredBuffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(StructuredBuffer<float4>, GLSL_paintAuxBuffer_raw)
    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

class FRiveAtomicResolveVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveAtomicResolveVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveAtomicResolveVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        return GRHISupportsPixelShaderUAVs;
    }
};

/*
 * The shader will convert a PF_R32 texture to PF_R8G8B8A8 in order to visualize
 * it. It does so by writing SourceTexture to the current rendertarget color
 * float4(value, 0, 0, 1)
 */
class FRiveBltU32AsF4PixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveBltU32AsF4PixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveBltU32AsF4PixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_TEXTURE(Texture2D, SourceTexture)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

/*
 * The shader will convert a PF_R32G32B32A32_UINT texture to PF_R8G8B8A8 in
 * order to visualize it. It does so by writing SourceTexture to the current
 * rendertarget color float4( float(val.r), float(val.g), float(val.b), 1.0 );
 */
class FRiveBltU324AsF4PixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveBltU324AsF4PixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveBltU324AsF4PixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_TEXTURE(Texture2D, SourceTexture)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};

/*
 * The shader will convert a uint2 buffer to PF_R8G8B8A8 in order to visualize
 * it. it does so in a slightly more intelligent way by passing the values of
 * buffer through sin and cos when writing it to the bound render target
 */
class FRiveVisualizeBufferPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveVisualizeBufferPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveVisualizeBufferPixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_SRV(Buffer<uint2>, SourceBuffer)
    SHADER_PARAMETER(UE::HLSL::uint2, ViewSize)
    SHADER_PARAMETER(UE::HLSL::uint, BufferSize)
    RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};
