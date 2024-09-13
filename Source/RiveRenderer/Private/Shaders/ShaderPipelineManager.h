#pragma once
#include "ShaderParameterStruct.h"
#include "RenderResource.h"
#include "RHI.h"

#include "HLSLTypeAliases.h"
#include "rive/shaders/out/generated/shaders/rhi.exports.h"

namespace rive::gpu
{
struct DrawBatch;
struct FlushDescriptor;
}

// shader permutation params
// Whole
class FEnableClip : SHADER_PERMUTATION_BOOL("ENABLE_CLIPPING");
class FEnableClipRect : SHADER_PERMUTATION_BOOL("ENABLE_CLIP_RECT");
class FEnableAdvanceBlend : SHADER_PERMUTATION_BOOL("ENABLE_ADVANCED_BLEND");

// FragmentOnly
class FEnableFixedFunctionColorBlend : SHADER_PERMUTATION_BOOL("FIXED_FUNCTION_COLOR_BLEND");
class FEnableHSLBlendMode : SHADER_PERMUTATION_BOOL("ENABLE_HSL_BLEND_MODES");
class FEnableNestedClip : SHADER_PERMUTATION_BOOL("ENABLE_NESTED_CLIPPING");
class FEnableEvenOdd: SHADER_PERMUTATION_BOOL("ENABLE_EVEN_ODD");

typedef TShaderPermutationDomain<FEnableClip, FEnableClipRect, FEnableNestedClip,
                                FEnableFixedFunctionColorBlend, FEnableAdvanceBlend, FEnableEvenOdd,
                                FEnableHSLBlendMode> AtomicPixelPermutationDomain; 
typedef TShaderPermutationDomain<FEnableClip, FEnableClipRect, FEnableAdvanceBlend> AtomicVertexPermutationDomain; 


#define USE_ATOMIC_PIXEL_PERMUTATIONS \
using FPermutationDomain = AtomicPixelPermutationDomain;

#define USE_ATOMIC_VERTEX_PERMUTATIONS \
using FPermutationDomain = AtomicVertexPermutationDomain;    

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FFlushUniforms, )
    SHADER_PARAMETER(float, gradInverseViewportY)
    SHADER_PARAMETER(float, tessInverseViewportY)
    SHADER_PARAMETER(float, renderTargetInverseViewportX)
    SHADER_PARAMETER(float, renderTargetInverseViewportY)
    SHADER_PARAMETER(UE::HLSL::uint, renderTargetWidth)
    SHADER_PARAMETER(UE::HLSL::uint, renderTargetHeight)
    SHADER_PARAMETER(UE::HLSL::uint, colorClearValue)          // Only used if clears are implemented as draws.
    SHADER_PARAMETER(UE::HLSL::uint, coverageClearValue)       // Only used if clears are implemented as draws.
    SHADER_PARAMETER(UE::HLSL::int4, renderTargetUpdateBounds) // drawBounds, or renderTargetBounds if there is a clear. (LTRB.)
    SHADER_PARAMETER(UE::HLSL::uint, pathIDGranularity)        // Spacing between adjacent path IDs (1 if IEEE compliant).
    SHADER_PARAMETER(float, vertexDiscardValue)
END_GLOBAL_SHADER_PARAMETER_STRUCT();

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FImageDrawUniforms, )
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

    DECLARE_GLOBAL_SHADER(FRiveGradientPixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveGradientPixelShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()
    
   static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
};

class FRiveGradientVertexShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FRiveGradientVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveGradientVertexShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()
    
   static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
};

class FRiveTessPixelShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FRiveTessPixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveTessPixelShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
};

class FRiveTessVertexShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FRiveTessVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveTessVertexShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_SRV(Buffer<float>, GLSL_pathBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float>, GLSL_contourBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
};

class FRivePathPixelShader : public FGlobalShader
{
public:
    
    DECLARE_GLOBAL_SHADER(FRivePathPixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRivePathPixelShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageCountBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}

};

class FRivePathVertexShader : public FGlobalShader
{
public:
    
    DECLARE_GLOBAL_SHADER(FRivePathVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRivePathVertexShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)

    SHADER_PARAMETER_SRV(Texture2D<uint4>, GLSL_tessVertexTexture_raw)
    SHADER_PARAMETER_SRV(Buffer<uint4>, GLSL_pathBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<uint4>, GLSL_contourBuffer_raw)
    SHADER_PARAMETER(unsigned int, baseInstance)
    
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}

};

class FRiveInteriorTrianglesPixelShader : public FGlobalShader
{
public:
    
    DECLARE_GLOBAL_SHADER(FRiveInteriorTrianglesPixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveInteriorTrianglesPixelShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageCountBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)

    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}

};

class FRiveInteriorTrianglesVertexShader : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FRiveInteriorTrianglesVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveInteriorTrianglesVertexShader, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_SRV(Buffer<uint4>, GLSL_pathBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS

    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}

};

class FRiveImageRectPixelShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FRiveImageRectPixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageRectPixelShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
    
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_imageTexture_raw)
    
    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageCountBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
    
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}
};

class FRiveImageRectVertexShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER( FRiveImageRectVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageRectVertexShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}
};

class FRiveImageMeshPixelShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER(FRiveImageMeshPixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageMeshPixelShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
    
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_imageTexture_raw)

    SHADER_PARAMETER_UAV(Texture2D<uint>, coverageCountBuffer)
    SHADER_PARAMETER_UAV(Texture2D<uint>, clipBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
    
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS

    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}
};

class FRiveImageMeshVertexShader : public FGlobalShader
{
public:

    DECLARE_GLOBAL_SHADER( FRiveImageMeshVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveImageMeshVertexShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    SHADER_PARAMETER_STRUCT_REF(FImageDrawUniforms, ImageDrawUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}
};

class FRiveAtomiResolvePixelShader : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER( FRiveAtomiResolvePixelShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveAtomiResolvePixelShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

    SHADER_PARAMETER_TEXTURE(Texture2D, GLSL_gradTexture_raw)
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)
    SHADER_PARAMETER_UAV(Texture2D, coverageCountBuffer)
    SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    SHADER_PARAMETER_UAV(Texture2D, clipBuffer)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_PIXEL_PERMUTATIONS
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
};

class FRiveAtomiResolveVertexShader : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER( FRiveAtomiResolveVertexShader);
    SHADER_USE_PARAMETER_STRUCT(FRiveAtomiResolveVertexShader, FGlobalShader);
    
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    SHADER_PARAMETER_STRUCT_REF(FFlushUniforms, FlushUniforms)
    END_SHADER_PARAMETER_STRUCT()

    USE_ATOMIC_VERTEX_PERMUTATIONS
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
    static bool ShouldCompilePermutation(const FShaderPermutationParameters& Parameters)
    {return true;}
};
