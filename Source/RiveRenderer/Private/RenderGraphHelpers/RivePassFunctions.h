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

BEGIN_SHADER_PARAMETER_STRUCT(FRivePatchPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGPathVertexShader::FParameters, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGPathPixelShader::FParameters, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRivePatchPassParameters* PassParameters);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveInteriorTrianglePassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(
    FRiveRDGInteriorTrianglesVertexShader::FParameters,
    VS)
SHADER_PARAMETER_STRUCT_INCLUDE(
    FRiveRDGInteriorTrianglesPixelShader::FParameters,
    PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveInteriorTrianglePassParameters* PassParameters);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveImageRectPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)

SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGImageRectVertexShader::FParameters, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGImageRectPixelShader::FParameters, PS)

RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawImageRectPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveImageRectPassParameters* PassParameters);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveImageMeshPassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGImageMeshVertexShader::FParameters, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGImageMeshPixelShader::FParameters, PS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddDrawImageMeshPass(
    FRDGBuilder& GraphBuilder,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveImageMeshPassParameters* PassParameters);

BEGIN_SHADER_PARAMETER_STRUCT(FRiveAtomicResolvePassParameters, )

SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGAtomicResolvePixelShader::FParameters,
                                PS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGAtomicResolveVertexShader::FParameters,
                                VS)
RENDER_TARGET_BINDING_SLOTS()

END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddAtomicResolvePass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveAtomicResolvePassParameters* PassParameters);
