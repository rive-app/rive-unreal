#pragma once
#include <CoreMinimal.h>

#include <RiveShaders/Public/RiveShaderTypes.h>

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/renderer/gpu.hpp"
THIRD_PARTY_INCLUDES_END

struct FRiveCommonPassParameters
{
    AtomicPixelPermutationDomain PixelPermutationDomain;
    AtomicVertexPermutationDomain VertexPermutationDomain;
    FVertexDeclarationRHIRef VertexDeclarationRHI = nullptr;
    FBufferRHIRef VertexBuffers[2];
    FBufferRHIRef IndexBuffer = nullptr;
    FUint32Rect Viewport;
    FGlobalShaderMap* ShaderMap;
    const rive::gpu::DrawBatch
        DrawBatch; // Copy intentionally since lamnda execution is defered for
                   // RenderGraph
    bool NeedsSourceBlending = false;

    FRiveCommonPassParameters(const rive::gpu::DrawBatch& DrawBatch,
                              FGlobalShaderMap* ShaderMap) :
        ShaderMap(ShaderMap), DrawBatch(DrawBatch)
    {}
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
SHADER_PARAMETER_STRUCT_INCLUDE(FRivePixelDrawUniforms, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters);

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
