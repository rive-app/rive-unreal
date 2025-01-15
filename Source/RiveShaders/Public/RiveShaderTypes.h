#pragma once
#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"
#include "RenderResource.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "ShaderCompilerCore.h"

#include "HLSLTypeAliases.h"
#include "rive/generated/shaders/rhi.exports.h"
#include "Misc/EngineVersionComparison.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRiveShaderCompiler, Display, All);

#if UE_VERSION_OLDER_THAN(5, 4, 0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip)                            \
    TStaticRasterizerState<FillMode, CullMode, false, false, DepthClip>::      \
        GetRHI()
#else // UE_VERSION_NEWER_OR_EQUAL_TO (5, 4,0)
#define CREATE_TEXTURE_ASYNC(RHICmdList, Desc) RHICmdList->CreateTexture(Desc)
#define CREATE_TEXTURE(RHICmdList, Desc) RHICmdList.CreateTexture(Desc)
#define RASTER_STATE(FillMode, CullMode, DepthClip)                            \
    TStaticRasterizerState<FillMode, CullMode, DepthClip, false>::GetRHI()
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

// FragmentOnly
class FEnableFixedFunctionColorOutput
    : SHADER_PERMUTATION_BOOL("FIXED_FUNCTION_COLOR_OUTPUT");
class FEnableHSLBlendMode : SHADER_PERMUTATION_BOOL("ENABLE_HSL_BLEND_MODES");
class FEnableNestedClip : SHADER_PERMUTATION_BOOL("ENABLE_NESTED_CLIPPING");
class FEnableEvenOdd : SHADER_PERMUTATION_BOOL("ENABLE_EVEN_ODD");
class FEnableTypedUAVLoads
    : SHADER_PERMUTATION_BOOL("ENABLE_TYPED_UAV_LOAD_STORE");

typedef TShaderPermutationDomain<FEnableClip,
                                 FEnableClipRect,
                                 FEnableNestedClip,
                                 FEnableFixedFunctionColorOutput,
                                 FEnableAdvanceBlend,
                                 FEnableEvenOdd,
                                 FEnableHSLBlendMode,
                                 FEnableTypedUAVLoads>

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

RIVESHADERS_API void BindStaticFlushUniforms(FRHICommandList&,
                                             FUniformBufferRHIRef);

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

template <typename VShaderType, typename PShaderType>
void BindShaders(FRHICommandList& CommandList,
                 FGraphicsPipelineStateInitializer& GraphicsPSOInit,
                 TShaderMapRef<VShaderType> VSShader,
                 TShaderMapRef<PShaderType> PSShader,
                 FRHIVertexDeclaration* VertexDeclaration)
{
    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = VertexDeclaration;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VSShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PSShader.GetPixelShader();
    SetGraphicsPipelineState(CommandList,
                             GraphicsPSOInit,
                             0,
                             EApplyRendertargetOption::CheckApply,
                             true,
                             EPSOPrecacheResult::Unknown);
}

template <typename ShaderType>
void SetParameters(FRHICommandList& CommandList,
                   FRHIBatchedShaderParameters& BatchedParameters,
                   TShaderMapRef<ShaderType> Shader,
                   typename ShaderType::FParameters& VParameters)
{
    ClearUnusedGraphResources(Shader, &VParameters);
    SetShaderParameters(BatchedParameters, Shader, VParameters);
    CommandList.SetBatchedShaderParameters(Shader.GetVertexShader(),
                                           BatchedParameters);
}

class FRiveRDGGradientPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGGradientPixelShader,
                                   RIVESHADERS_API);

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGGradientVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGGradientVertexShader,
                                   RIVESHADERS_API);

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGTessPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGTessPixelShader, RIVESHADERS_API);

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGTessVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGTessVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGTessVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_pathBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_contourBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGPathPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGPathPixelShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGPathPixelShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)

    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>,
                                    GLSL_paintBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                    GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);

    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        FPermutationDomain PermutationVector(Parameters.PermutationId);

        // don't compile typed UAV permutations if they aren't supported.
        if (PermutationVector.Get<FEnableTypedUAVLoads>() &&
            !(RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
              GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(LogRiveShaderCompiler,
                   Verbose,
                   TEXT("Skipping Permutation %i for FRiveRDGPathPixelShader "
                        "because typed uavs are not  supported"),
                   Parameters.PermutationId);
            return false;
        }

        // // only compile typed UAV permutations if they are supported
        if (!PermutationVector.Get<FEnableTypedUAVLoads>() &&
            (RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
             GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(LogRiveShaderCompiler,
                   Verbose,
                   TEXT("Skipping Permutation %i for FRiveRDGPathPixelShader "
                        "because typed uavs are supported"),
                   Parameters.PermutationId);
            return false;
        }

        UE_LOG(LogRiveShaderCompiler,
               VeryVerbose,
               TEXT("Building Permutation %i for FRiveRDGPathPixelShader"),
               Parameters.PermutationId)
        return true;
    }
};

class FRiveRDGPathVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGPathVertexShader, RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGPathVertexShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint4>,
                                     GLSL_tessVertexTexture_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_pathBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_contourBuffer_raw)
    SHADER_PARAMETER(unsigned int, baseInstance)

    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGInteriorTrianglesPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGInteriorTrianglesPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGInteriorTrianglesPixelShader,
                                FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)

    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)

    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>,
                                    GLSL_paintBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                    GLSL_paintAuxBuffer_raw)

    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        FPermutationDomain PermutationVector(Parameters.PermutationId);

        // don't compile typed UAV permutations if they aren't supported.
        if (PermutationVector.Get<FEnableTypedUAVLoads>() &&
            !(RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
              GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(LogRiveShaderCompiler,
                   Verbose,
                   TEXT("Skipping Permutation %i for "
                        "FRiveRDGInteriorTrianglesPixelShader because typed "
                        "uavs are not supported"),
                   Parameters.PermutationId);
            return false;
        }

        // only compile typed UAV permutations if they are supported
        if (!PermutationVector.Get<FEnableTypedUAVLoads>() &&
            (RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
             GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(LogRiveShaderCompiler,
                   Verbose,
                   TEXT("Skipping Permutation %i for "
                        "FRiveRDGInteriorTrianglesPixelShader because typed "
                        "uavs are supported"),
                   Parameters.PermutationId);
            return false;
        }

        UE_LOG(LogRiveShaderCompiler,
               VeryVerbose,
               TEXT("Building Permutation %i for "
                    "FRiveRDGInteriorTrianglesPixelShader"),
               Parameters.PermutationId)
        return true;
    }
};

class FRiveRDGInteriorTrianglesVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGInteriorTrianglesVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGInteriorTrianglesVertexShader,
                                FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint4>,
                                    GLSL_pathBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGImageRectPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageRectPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageRectPixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)

    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_imageTexture_raw)

    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)

    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)

    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>,
                                    GLSL_paintBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                    GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        FPermutationDomain PermutationVector(Parameters.PermutationId);

        // don't compile typed UAV permutations if they aren't supported.
        if (PermutationVector.Get<FEnableTypedUAVLoads>() &&
            !(RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
              GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(
                LogRiveShaderCompiler,
                Verbose,
                TEXT(
                    "Skipping Permutation %i  for FRiveRDGImageRectPixelShader "
                    "because typed uavs are not supported"),
                Parameters.PermutationId);
            return false;
        }

        // only compile typed UAV permutations if they are supported
        if (!PermutationVector.Get<FEnableTypedUAVLoads>() &&
            (RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
             GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(
                LogRiveShaderCompiler,
                Verbose,
                TEXT("Skipping Permutation %i for FRiveRDGImageRectPixelShader "
                     "because typed uavs are supported"),
                Parameters.PermutationId);
            return false;
        }

        UE_LOG(LogRiveShaderCompiler,
               VeryVerbose,
               TEXT("Building Permutation %i for FRiveRDGImageRectPixelShader"),
               Parameters.PermutationId)
        return true;
    }
};

class FRiveRDGImageRectVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageRectVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageRectVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGImageMeshPixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageMeshPixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageMeshPixelShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)

    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_imageTexture_raw)

    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)

    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)

    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>,
                                    GLSL_paintBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                    GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        FPermutationDomain PermutationVector(Parameters.PermutationId);

        // don't compile typed UAV permutations if they aren't supported.
        if (PermutationVector.Get<FEnableTypedUAVLoads>() &&
            !(RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
              GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(
                LogRiveShaderCompiler,
                Verbose,
                TEXT("Skipping Permutation %i for FRiveRDGImageMeshPixelShader "
                     "because typed uavs are not supported"),
                Parameters.PermutationId);
            return false;
        }

        // only compile typed UAV permutations if they are supported
        if (!PermutationVector.Get<FEnableTypedUAVLoads>() &&
            (RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
             GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(
                LogRiveShaderCompiler,
                Verbose,
                TEXT(
                    "Skipping Permutation  %i for FRiveRDGImageMeshPixelShader "
                    "because typed uavs are supported"),
                Parameters.PermutationId);
            return false;
        }

        UE_LOG(LogRiveShaderCompiler,
               VeryVerbose,
               TEXT("Building Permutation %i for FRiveRDGImageMeshPixelShader"),
               Parameters.PermutationId)
        return true;
    }
};

class FRiveRDGImageMeshVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGImageMeshVertexShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGImageMeshVertexShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FImageDrawUniforms, ImageDrawUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
};

class FRiveRDGAtomicResolvePixelShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtomicResolvePixelShader,
                                   RIVESHADERS_API);
    SHADER_USE_PARAMETER_STRUCT(FRiveRDGAtomicResolvePixelShader,
                                FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint2>,
                                    GLSL_paintBuffer_raw)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,
                                    GLSL_paintAuxBuffer_raw)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, coverageAtomicBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, colorBuffer)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, clipBuffer)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(
        const FShaderPermutationParameters& Parameters)
    {
        FPermutationDomain PermutationVector(Parameters.PermutationId);

        // don't compile typed UAV permutations if they aren't supported.
        if (PermutationVector.Get<FEnableTypedUAVLoads>() &&
            !(RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
              GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(LogRiveShaderCompiler,
                   Verbose,
                   TEXT("Skipping Permutation %i for "
                        "FRiveRDGAtomicResolvePixelShader because typed uavs "
                        "are not supported"),
                   Parameters.PermutationId);
            return false;
        }

        // only compile typed UAV permutations if they are supported
        if (!PermutationVector.Get<FEnableTypedUAVLoads>() &&
            (RHISupports4ComponentUAVReadWrite(Parameters.Platform) ||
             GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::Vulkan))
        {
            UE_LOG(LogRiveShaderCompiler,
                   Verbose,
                   TEXT("Skipping Permutation %i for "
                        "FRiveRDGAtomicResolvePixelShader because typed uavs "
                        "are supported"),
                   Parameters.PermutationId);
            return false;
        }

        UE_LOG(
            LogRiveShaderCompiler,
            VeryVerbose,
            TEXT(
                "Building Permutation %i for FRiveRDGAtomicResolvePixelShader"),
            Parameters.PermutationId)
        return true;
    }
};

class FRiveRDGAtomicResolveVertexShader : public FGlobalShader
{
public:
    DECLARE_EXPORTED_GLOBAL_SHADER(FRiveRDGAtomicResolveVertexShader,
                                   RIVESHADERS_API);
    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(
        const FShaderPermutationParameters&,
        FShaderCompilerEnvironment&);
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
