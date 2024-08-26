#pragma once
#include "ShaderParameterStruct.h"
#include "RenderResource.h"
#include "RHI.h"
#include <string>

#include "D3D11RHIPrivate.h"
#include "HLSLTypeAliases.h"
#include "rive/shaders/out/generated/rhi.exports.h"

namespace rive::pls
{
struct DrawBatch;
struct FlushDescriptor;
}

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
    
    SHADER_PARAMETER_UAV(Texture2D, coverageCountBuffer)
    
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
    
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
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
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
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

    SHADER_PARAMETER_UAV(Texture2D, coverageCountBuffer)
    //SHADER_PARAMETER_UAV(Texture2D, colorBuffer)
    
    SHADER_PARAMETER_SAMPLER(SamplerState, gradSampler)
    SHADER_PARAMETER_SAMPLER(SamplerState, imageSampler)
    
    SHADER_PARAMETER_SRV(Buffer<uint2>, GLSL_paintBuffer_raw)
    SHADER_PARAMETER_SRV(Buffer<float4>, GLSL_paintAuxBuffer_raw)
    END_SHADER_PARAMETER_STRUCT()

    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
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
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
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
    END_SHADER_PARAMETER_STRUCT()
    
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
    
    static void ModifyCompilationEnvironment(const FShaderPermutationParameters&, FShaderCompilerEnvironment&);
};

class FRiveTestVertexShader : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER( FRiveTestVertexShader);
};

class FRiveTestPixelShader : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER( FRiveTestPixelShader);
};

template<typename VertexShaderType, typename PixelShaderType>
class SimpleGraphicsPipeline
{
public:
    explicit SimpleGraphicsPipeline(FRHIVertexDeclaration* InVertexDeclaration, FGlobalShaderMap* ShaderMap) : CachedVertexDeclaration(InVertexDeclaration), VSShader(ShaderMap), PSShader(ShaderMap)
    {
    }

    void BindShaders(FRHICommandList& CommandList, FGraphicsPipelineStateInitializer& GraphicsPSOInit)
    {
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = CachedVertexDeclaration;
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VSShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PSShader.GetPixelShader();
        SetGraphicsPipelineState(CommandList, GraphicsPSOInit, 0, EApplyRendertargetOption::CheckApply, true, EPSOPrecacheResult::NotSupported);
    }

private:
    FRHIVertexDeclaration* CachedVertexDeclaration;
    TShaderMapRef<VertexShaderType> VSShader;
    TShaderMapRef<PixelShaderType> PSShader;
};

template<typename VertexShaderType, typename PixelShaderType>
class GraphicsPipeline
{
public:
    typedef typename VertexShaderType::FParameters VertexParameters;
    typedef typename PixelShaderType::FParameters  PixelParameters;
    
    explicit GraphicsPipeline(FRHIVertexDeclaration* InVertexDeclaration, FGlobalShaderMap* ShaderMap) : CachedVertexDeclaration(InVertexDeclaration), VSShader(ShaderMap), PSShader(ShaderMap)
    {
    }

    void BindShaders(FRHICommandList& CommandList, FGraphicsPipelineStateInitializer& GraphicsPSOInit)
    {
        GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = CachedVertexDeclaration;
        GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VSShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PSShader.GetPixelShader();
        SetGraphicsPipelineState(CommandList, GraphicsPSOInit, 0, EApplyRendertargetOption::CheckApply, true, EPSOPrecacheResult::NotSupported);
    }

    void SetParameters(FRHICommandList& CommandList, FRHIBatchedShaderParameters& BatchedParameters,
        const VertexParameters& VParameters,
        const PixelParameters& PParameters)
    {
        SetShaderParameters(BatchedParameters, VSShader, VParameters);
        CommandList.SetBatchedShaderParameters(VSShader.GetVertexShader(), BatchedParameters);
        SetShaderParameters(BatchedParameters, PSShader, PParameters);
        CommandList.SetBatchedShaderParameters(PSShader.GetPixelShader(), BatchedParameters);
    }
    
    void SetVertexParameters(FRHICommandList& CommandList, 
        FRHIBatchedShaderParameters& BatchedParameters,
        const VertexParameters& Parameters)
    {
        SetShaderParameters(BatchedParameters, VSShader, Parameters);
        CommandList.SetBatchedShaderParameters(VSShader.GetVertexShader(), BatchedParameters);
    }

    void SetPixelParameters(FRHICommandList& CommandList, 
        FRHIBatchedShaderParameters& BatchedParameters,
        const PixelParameters& Parameters)
    {
        SetShaderParameters(BatchedParameters, PSShader, Parameters);
        CommandList.SetBatchedShaderParameters(PSShader.GetPixelShader(), BatchedParameters);
    }

private:
    FRHIVertexDeclaration* CachedVertexDeclaration;
    TShaderMapRef<VertexShaderType> VSShader;
    TShaderMapRef<PixelShaderType> PSShader;
};

typedef GraphicsPipeline<FRiveImageMeshVertexShader, FRiveImageMeshPixelShader> ImageMeshPipeline;
typedef GraphicsPipeline<FRiveTessVertexShader, FRiveTessPixelShader> TessPipeline;
typedef GraphicsPipeline<FRiveGradientVertexShader, FRiveGradientPixelShader> GradientPipeline;
typedef GraphicsPipeline<FRiveAtomiResolveVertexShader, FRiveAtomiResolvePixelShader> AtomicResolvePipeline;
typedef GraphicsPipeline<FRiveImageRectVertexShader, FRiveImageRectPixelShader> ImageRectPipeline;
typedef SimpleGraphicsPipeline<FRiveTestVertexShader, FRiveTestPixelShader> TestSimplePipeline;
