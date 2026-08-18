// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once
#include <CoreMinimal.h>

#include <RiveShaders/Public/RiveShaderTypes.h>

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/gpu.hpp"
THIRD_PARTY_INCLUDES_END

enum EBlendType
{
    None,
    WriteOnly,
    Blend
};

struct FRiveCommonPassParameters
{
    AtomicPixelPermutationDomain PixelPermutationDomain;
    AtomicVertexPermutationDomain VertexPermutationDomain;
    FVertexDeclarationRHIRef VertexDeclarationRHI = nullptr;
    // [0]/[1] geometry streams; imageMesh uses [2] for the per-instance
    // gpu::ImageDrawInstance stream (imageRect uses [1]).
    FBufferRHIRef VertexBuffers[3];
    FBufferRHIRef IndexBuffer = nullptr;
    FUint32Rect Viewport;
    FUint32Rect Scissor;
    bool bWireframe = false;
    FGlobalShaderMap* ShaderMap;
    const rive::gpu::DrawBatch
        DrawBatch; // Copy intentionally since lambda execution is deferred for
                   // RenderGraph
    EBlendType BlendType = EBlendType::None;
    // Copy for the same reason as above.
    // This is currently only used for MSAA mode, but may get used for other
    // modes in the future
    rive::gpu::PipelineState PipelineState;
    const rive::gpu::PlatformFeatures& PlatformFeatures;
    // Picks the subpass read msaa pixel shaders over the sampled dst color
    // ones. Both variants are compiled, so this is a runtime choice.
    bool bUseSubpassLoad = false;

    // The subpass variants only exist for permutations that actually read the
    // dst color, which is what their ShouldCompilePermutation allows. Asking
    // for one outside that set would look up a shader that was never compiled.
    bool UseSubpassPixelShader() const
    {
        return bUseSubpassLoad &&
               PixelPermutationDomain.Get<FEnableAdvanceBlend>() &&
               !PixelPermutationDomain.Get<FEnableFixedFunctionColorOutput>();
    }
    FRiveCommonPassParameters(
        const rive::gpu::DrawBatch& DrawBatch,
        const rive::gpu::PipelineState& PipelineState,
        const rive::gpu::PlatformFeatures& PlatformFeatures,
        FGlobalShaderMap* ShaderMap) :
        ShaderMap(ShaderMap),
        DrawBatch(DrawBatch),
        PipelineState(PipelineState),
        PlatformFeatures(PlatformFeatures)
    {}
    uint32_t GetUniqueKey(rive::gpu::InterlockMode Interlock) const
    {
        return GetUniqueKeyForDrawType(DrawBatch.drawType, Interlock);
    }

    // msaaDynamicMidpointFans draws one batch once per collapsed subpass, so
    // the state it caches has to be keyed off the pass rather than the batch.
    uint32_t GetUniqueKeyForDrawType(rive::gpu::DrawType DrawType,
                                     rive::gpu::InterlockMode Interlock) const
    {
        return pipeline_unique_key(DrawType,
                                   DrawBatch.shaderFeatures,
                                   Interlock,
                                   DrawBatch.shaderMiscFlags,
                                   DrawBatch.drawContents,
                                   BlendType == EBlendType::Blend,
                                   rive::BlendMode::srcOver,
                                   PlatformFeatures);
    }
};

struct FRiveAtlasParameters
{
    FVertexDeclarationRHIRef VertexDeclarationRHI = nullptr;
    FBufferRHIRef VertexBuffer = nullptr;
    FBufferRHIRef IndexBuffer = nullptr;
    FUint32Rect Viewport;
    FGlobalShaderMap* ShaderMap;
    const rive::gpu::AtlasDrawBatch
        DrawBatch; // Copy intentionally since lamnda execution is defered for
                   // RenderGraph

    FRiveAtlasParameters(const rive::gpu::AtlasDrawBatch& DrawBatch,
                         FGlobalShaderMap* ShaderMap) :
        ShaderMap(ShaderMap), DrawBatch(DrawBatch)
    {}
};

BEGIN_SHADER_PARAMETER_STRUCT(FRDGAtlasPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveVertexDrawUniforms, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FAtlasDrawUniforms, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddGradientPass(FRDGBuilder& GraphBuilder,
                            TRDGUniformBufferRef<FFlushUniforms> FlushUniforms,
                            FVertexDeclarationRHIRef VertexDeclaration,
                            FRDGTextureRef GradientTexture,
                            FBufferRHIRef GradientSpanBuffer,
                            FUint32Rect Viewport,
                            uint32_t NumGradients);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveTesselationPassParameters, )
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGTessVertexShader::FParameters, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGTessPixelShader::FParameters, PS)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddTessellationPass(
    FRDGBuilder& GraphBuilder,
    FVertexDeclarationRHIRef VertexDeclaration,
    FBufferRHIRef TessSpanBuffer,
    FBufferRHIRef TessIndexBuffer,
    FUint32Rect Viewport,
    uint32_t NumTessellations,
    FRiveTesselationPassParameters* TesselationPassParameters);

FRDGPassRef AddFeatherAtlasFillDrawPass(
    FRDGBuilder& GraphBuilder,
    FRiveAtlasParameters* AtlasParameters,
    FRDGAtlasPassParameters* PassParameters);

FRDGPassRef AddFeatherAtlasStrokeDrawPass(
    FRDGBuilder& GraphBuilder,
    FRiveAtlasParameters* AtlasParameters,
    FRDGAtlasPassParameters* PassParameters);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveFlushPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveVertexDrawUniforms, VS)
#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
SHADER_PARAMETER_STRUCT_INCLUDE(FRivePixelDrawUniformsAtomicBuffers, PS)
#else
SHADER_PARAMETER_STRUCT_INCLUDE(FRivePixelDrawUniforms, PS)
#endif
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FRiveRasterOrderFlushPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveVertexDrawUniforms, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRivePixelDrawUniforms, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FRiveMSAAFlushPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveMSAAVertexDrawUniforms, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveMSAAPixelDrawUniforms, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

// Resets the bound-pipeline record the AddDrawMSAA* passes use to skip
// redundant binds. Call when a render pass begins, and before any pipeline
// bind that does not go through those functions.
void RiveInvalidateBoundPipelineState();

void AddDrawMSAAPatchesPass(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters);

#if defined(UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE)
// The msaa fast path fill, drawn as one batch instead of three.
void AddDrawMSAADynamicMidpointFansPass(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters,
    TConstArrayView<rive::gpu::PipelineState> PassPipelineStates);
#endif // UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE

void AddDrawMSAAStencilClipResetPass(
    FRHICommandList& RHICmdList,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters);

void AddDrawMSAAAtlasBlitPass(
    FRHICommandList& RHICmdList,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters);

void AddDrawMSAAImageMeshPass(
    FRHICommandList& RHICmdList,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters);

FRDGPassRef AddDrawInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

FRDGPassRef AddDrawAtlasBlitPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

FRDGPassRef AddDrawImageRectPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

FRDGPassRef AddDrawImageMeshPass(
    FRDGBuilder& GraphBuilder,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

FRDGPassRef AddAtomicResolvePass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

FRDGPassRef AddDrawRasterOrderPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveRasterOrderFlushPassParameters* PassParameters);

FRDGPassRef AddDrawRasterOrderInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveRasterOrderFlushPassParameters* PassParameters);

FRDGPassRef AddDrawRasterOrderImageMeshPass(
    FRDGBuilder& GraphBuilder,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveRasterOrderFlushPassParameters* PassParameters);

FRDGPassRef AddDrawRasterOrderAtlasBlitPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveRasterOrderFlushPassParameters* PassParameters);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveDrawTextureBltParameters, )
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveBltTextureDrawUniforms, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawTextureBlt(FRDGBuilder& GraphBuilder,
                              FVertexDeclarationRHIRef VertexDeclarationRHI,
                              FUint32Rect Viewport,
                              FGlobalShaderMap* ShaderMap,
                              FRiveDrawTextureBltParameters* PassParameters,
                              bool isMSAAResolve,
                              bool bOpaque = false);

void AddDrawTextureBlt(FRHICommandList& RHICmdList,
                       FVertexDeclarationRHIRef VertexDeclarationRHI,
                       FGlobalShaderMap* ShaderMap,
                       FRiveDrawTextureBltParameters& PassParameters,
                       const rive::gpu::DrawBatch& Batch,
                       bool isMSAAResolve);

FRDGPassRef AddDrawClearQuadPass(FRDGBuilder& GraphBuilder,
                                 FRDGTextureRef RenderTarget,
                                 FLinearColor ClearColor);