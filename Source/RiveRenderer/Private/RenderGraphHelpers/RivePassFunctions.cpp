// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RivePassFunctions.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphEvent.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"  // TStaticBlendState, etc.
#include "RHICommandList.h"   // FRHICommandList
#include "RenderGraphUtils.h" // Graph building helpers
#include "rive/renderer/draw.hpp"

using namespace rive::gpu;

TEnumAsByte<EStencilOp> StencilOpForStencilFaceOps(const StencilOp Op)
{
    switch (Op)
    {
        case StencilOp::keep:
            return SO_Keep;
        case StencilOp::replace:
            return SO_Replace;
        case StencilOp::zero:
            return SO_Zero;
        case StencilOp::decrClamp:
            return SO_SaturatedDecrement;
        case StencilOp::incrWrap:
            return SO_Increment;
        case StencilOp::decrWrap:
            return SO_Decrement;
    }

    // This won't compile without a default return here.
    return SO_Keep;
}

TEnumAsByte<ECompareFunction> CompareOpForStencilCompareOps(
    const StencilCompareOp Op)
{
    switch (Op)
    {
        case StencilCompareOp::less:
            return CF_Less;
        case StencilCompareOp::equal:
            return CF_Equal;
        case StencilCompareOp::lessOrEqual:
            return CF_LessEqual;
        case StencilCompareOp::notEqual:
            return CF_NotEqual;
        case StencilCompareOp::always:
            return CF_Always;
    }

    // This won't compile without a default return here.
    return CF_Always;
}

FDepthStencilStateRHIRef StencilStateForPipeline(
    const PipelineState& PipelineState)
{
    // We shouldn't need locks here because this should only ever happen from
    // the render thread.
    static TMap<uint32_t, FDepthStencilStateRHIRef> StencilStates;
    if (FDepthStencilStateRHIRef* Value =
            StencilStates.Find(PipelineState.uniqueKey))
    {
        return *Value;
    }

    // RHI expects the front face to be CCW and rive expects it to be CW. So
    // reverse the logic here to make the stencil tests line up.

    const auto& FrontOps = PipelineState.stencilDoubleSided
                               ? PipelineState.stencilBackOps
                               : PipelineState.stencilFrontOps;
    const auto& BackOps = PipelineState.stencilFrontOps;

    FDepthStencilStateInitializerRHI Initializer(
        PipelineState.depthWriteEnabled,
        PipelineState.depthTestEnabled ? CF_Less : CF_Always,
        PipelineState.stencilTestEnabled,
        CompareOpForStencilCompareOps(FrontOps.compareOp),
        StencilOpForStencilFaceOps(FrontOps.failOp),
        StencilOpForStencilFaceOps(FrontOps.depthFailOp),
        StencilOpForStencilFaceOps(FrontOps.passOp),
        PipelineState.stencilTestEnabled,
        CompareOpForStencilCompareOps(BackOps.compareOp),
        StencilOpForStencilFaceOps(BackOps.failOp),
        StencilOpForStencilFaceOps(BackOps.depthFailOp),
        StencilOpForStencilFaceOps(BackOps.passOp),
        PipelineState.stencilCompareMask,
        PipelineState.stencilWriteMask);

    auto State = RHICreateDepthStencilState(Initializer);
    StencilStates.Add(PipelineState.uniqueKey, State);
    return State;
}

FBlendStateRHIRef BlendStateForPipeline(const PipelineState& PipelineState)
{
    if (!PipelineState.colorWriteEnabled)
        return TStaticBlendState<CW_NONE>::GetRHI();
    switch (PipelineState.blendEquation)
    {
        case BlendEquation::none:
        case BlendEquation::srcOver:
            return TStaticBlendState<CW_RGBA,
                                     BO_Add,
                                     BF_One,
                                     BF_InverseSourceAlpha,
                                     BO_Add,
                                     BF_One,
                                     BF_InverseSourceAlpha>::GetRHI();
        case BlendEquation::plus:
        case BlendEquation::max:
        case BlendEquation::screen:
        case BlendEquation::overlay:
        case BlendEquation::darken:
        case BlendEquation::lighten:
        case BlendEquation::colorDodge:
        case BlendEquation::colorBurn:
        case BlendEquation::hardLight:
        case BlendEquation::softLight:
        case BlendEquation::difference:
        case BlendEquation::exclusion:
        case BlendEquation::multiply:
        case BlendEquation::hue:
        case BlendEquation::saturation:
        case BlendEquation::color:
        case BlendEquation::luminosity:
            RIVE_UNREACHABLE();
            break;
    }

    return TStaticBlendState<CW_NONE>::GetRHI();
}

template <bool EnableMSAA>
FRHIRasterizerState* RasterStateForCullModeAndDrawMode(CullFace CF,
                                                       bool WireFrame)
{
    switch (CF)
    {
        case CullFace::none:
            if (WireFrame)
                return RASTER_STATE(FM_Wireframe,
                                    CM_None,
                                    ERasterizerDepthClipMode::DepthClip,
                                    EnableMSAA);
            return RASTER_STATE(FM_Solid,
                                CM_None,
                                ERasterizerDepthClipMode::DepthClip,
                                EnableMSAA);
        case CullFace::clockwise:
            if (WireFrame)
                return RASTER_STATE(FM_Wireframe,
                                    CM_CW,
                                    ERasterizerDepthClipMode::DepthClip,
                                    EnableMSAA);
            return RASTER_STATE(FM_Solid,
                                CM_CW,
                                ERasterizerDepthClipMode::DepthClip,
                                EnableMSAA);
        case CullFace::counterclockwise:
            if (WireFrame)
                return RASTER_STATE(FM_Wireframe,
                                    CM_CCW,
                                    ERasterizerDepthClipMode::DepthClip,
                                    EnableMSAA);
            return RASTER_STATE(FM_Solid,
                                CM_CCW,
                                ERasterizerDepthClipMode::DepthClip,
                                EnableMSAA);
    }

    // This won't compile without a default return here.
    return RASTER_STATE(FM_Solid,
                        CM_None,
                        ERasterizerDepthClipMode::DepthClamp,
                        EnableMSAA);
}

template <typename PassParamType>
void SetFlushUniformsPerShader(PassParamType* PassParams)
{
    // for 5.5 we have to not use static uniform slots. Unreal keeps giving us
    // an error about SlateView static slot not being bound when we include
    // Engine/Generated/GeneratedUniformBuffers so for now we just dont use it
    // in 5.5
#if UE_VERSION_OLDER_THAN(5, 5, 0)
#else
    PassParams->VS.GLSL_FlushUniforms_raw = PassParams->FlushUniforms;
    PassParams->PS.GLSL_FlushUniforms_raw = PassParams->FlushUniforms;
#endif
}

BEGIN_SHADER_PARAMETER_STRUCT(FRDGPassParameters, )
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGGradientVertexShader::FParameters, VS)
SHADER_PARAMETER_STRUCT_INCLUDE(FRiveRDGGradientPixelShader::FParameters, PS)
RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

FRDGPassRef AddGradientPass(FRDGBuilder& GraphBuilder,
                            TRDGUniformBufferRef<FFlushUniforms> FlushUniforms,
                            FVertexDeclarationRHIRef VertexDeclaration,
                            FRDGTextureRef GradientTexture,
                            FBufferRHIRef GradientSpanBuffer,
                            FUint32Rect Viewport,
                            uint32_t NumGradients)
{
    auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    TShaderMapRef<FRiveRDGGradientVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveRDGGradientPixelShader> PixelShader(ShaderMap);

    FRDGPassParameters* GradientPassParams =
        GraphBuilder.AllocParameters<FRDGPassParameters>();
    GradientPassParams->FlushUniforms = FlushUniforms;
    GradientPassParams->RenderTargets[0] =
        FRenderTargetBinding(GradientTexture, ERenderTargetLoadAction::ELoad);

    SetFlushUniformsPerShader(GradientPassParams);

    ClearUnusedGraphResources(PixelShader, &GradientPassParams->PS);
    ClearUnusedGraphResources(VertexShader, &GradientPassParams->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Render_Gradient"),
        GradientPassParams,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
        [PassParameters = GradientPassParams,
         Viewport,
         GradientSpanBuffer,
         NumGradients,
         VertexDeclaration,
         VertexShader,
         PixelShader](FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

            FRHIBatchedShaderParameters& BatchedShaderParameters =
                RHICmdList.GetScratchShaderParameters();

            RHICmdList.SetViewport(Viewport.Min.X,
                                   Viewport.Min.Y,
                                   0,
                                   Viewport.Max.X,
                                   Viewport.Max.Y,
                                   1.0);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                VertexDeclaration;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();
            SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);
            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);

            RHICmdList.SetStreamSource(0, GradientSpanBuffer, 0);

            RHICmdList.DrawPrimitive(
                0,
                rive::gpu::GRAD_SPAN_TRI_STRIP_VERTEX_COUNT - 2,
                NumGradients);
        });
}

FRDGPassRef AddTessellationPass(
    FRDGBuilder& GraphBuilder,
    FVertexDeclarationRHIRef VertexDeclaration,
    FBufferRHIRef TessSpanBuffer,
    FBufferRHIRef TessIndexBuffer,
    FUint32Rect Viewport,
    uint32_t NumTessellations,
    FRiveTesselationPassParameters* TesselationPassParameters)
{
    const auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

    SetFlushUniformsPerShader(TesselationPassParameters);

    TShaderMapRef<FRiveRDGTessVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveRDGTessPixelShader> PixelShader(ShaderMap);

    ClearUnusedGraphResources(PixelShader, &TesselationPassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &TesselationPassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Tesselation_Update"),
        TesselationPassParameters,
        ERDGPassFlags::Raster,
        [TessSpanBuffer,
         TessIndexBuffer,
         VertexDeclaration,
         Viewport,
         NumTessellations,
         VertexShader,
         PixelShader,
         TesselationPassParameters](FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_CCW,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            FRHIBatchedShaderParameters& BatchedShaderParameters =
                RHICmdList.GetScratchShaderParameters();

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                VertexDeclaration;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();
            SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                TesselationPassParameters->PS);
            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                TesselationPassParameters->VS);

            RHICmdList.SetStreamSource(0, TessSpanBuffer, 0);

            RHICmdList.SetViewport(Viewport.Min.X,
                                   Viewport.Min.Y,
                                   0,
                                   Viewport.Max.X,
                                   Viewport.Max.Y,
                                   1);

            RHICmdList.DrawIndexedPrimitive(TessIndexBuffer,
                                            0,
                                            0,
                                            8,
                                            0,
                                            std::size(kTessSpanIndices) / 3,
                                            NumTessellations);
        });
}

FRHIBlendState* RHIBlendStateForBlendType(EBlendType BlendType)
{
    switch (BlendType)
    {
        case EBlendType::None:
            return TStaticBlendState<CW_NONE>::GetRHI();
        case EBlendType::WriteOnly:
            return TStaticBlendState<CW_RGB>::GetRHI();
        case EBlendType::Blend:
            return TStaticBlendState<CW_RGBA,
                                     BO_Add,
                                     BF_One,
                                     BF_InverseSourceAlpha,
                                     BO_Add,
                                     BF_One,
                                     BF_InverseSourceAlpha>::GetRHI();
    }

    return TStaticBlendState<CW_NONE>::GetRHI();
}

FRDGPassRef AddDrawPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGPathVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGPathPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);
    // PassParameters->VS.baseInstance = 0;
    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Patch %s", *PassName),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,
                0,
                kPatchVertexBufferCount,
                PatchBaseIndex(CommonPassParameters->DrawBatch.drawType),
                PatchIndexCount(CommonPassParameters->DrawBatch.drawType) / 3,
                CommonPassParameters->DrawBatch.elementCount);
        });
}

void AddDrawMSAAPatchesPass(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAPatches %f", PassName);

    TShaderMapRef<FRiveRDGPathMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGPathMSAAPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil =
        StencilStateForPipeline(CommonPassParameters->PipelineState);

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.DepthStencilState = DepthStencil;

    GraphicsPSOInit.RasterizerState = RasterStateForCullModeAndDrawMode<true>(
        CommonPassParameters->PipelineState.cullFace,
        CommonPassParameters->bWireframe);

    GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
    GraphicsPSOInit.BlendState =
        BlendStateForPipeline(CommonPassParameters->PipelineState);
    GraphicsPSOInit.bDepthBounds = true;

    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
        CommonPassParameters->VertexDeclarationRHI;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
        PixelShader.GetPixelShader();

    GraphicsPSOInit.bAllowVariableRateShading = false;

    SET_PIPELINE_STATE(RHICmdList,
                       GraphicsPSOInit,
                       CommonPassParameters->PipelineState.stencilReference);

    RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                           CommonPassParameters->Viewport.Min.Y,
                           0,
                           CommonPassParameters->Viewport.Max.X,
                           CommonPassParameters->Viewport.Max.Y,
                           1);

    RHICmdList.SetScissorRect(true,
                              CommonPassParameters->Scissor.Min.X,
                              CommonPassParameters->Scissor.Min.Y,
                              CommonPassParameters->Scissor.Max.X,
                              CommonPassParameters->Scissor.Max.Y);

    RHICmdList.SetDepthBounds(rive::gpu::DEPTH_MIN, rive::gpu::DEPTH_MAX);

    SetShaderParameters(RHICmdList,
                        VertexShader,
                        VertexShader.GetVertexShader(),
                        PassParameters->VS);
    SetShaderParameters(RHICmdList,
                        PixelShader,
                        PixelShader.GetPixelShader(),
                        PassParameters->PS);

    RHICmdList.SetStreamSource(0, CommonPassParameters->VertexBuffers[0], 0);
    RHICmdList.DrawIndexedPrimitive(
        CommonPassParameters->IndexBuffer,
        0,
        0,
        kPatchVertexBufferCount,
        PatchBaseIndex(CommonPassParameters->DrawBatch.drawType),
        PatchIndexCount(CommonPassParameters->DrawBatch.drawType) / 3,
        CommonPassParameters->DrawBatch.elementCount);
}

void AddDrawMSAAStencilClipResetPass(
    FRHICommandList& RHICmdList,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAStencilClipReset");

    TShaderMapRef<FRiveRDGStencilMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap);
    TShaderMapRef<FRiveRDGStencilMSAAPixelShader> PixelShader(
        CommonPassParameters->ShaderMap);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil =
        StencilStateForPipeline(CommonPassParameters->PipelineState);

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.DepthStencilState = DepthStencil;

    GraphicsPSOInit.RasterizerState = RasterStateForCullModeAndDrawMode<true>(
        CommonPassParameters->PipelineState.cullFace,
        CommonPassParameters->bWireframe);

    GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
    GraphicsPSOInit.BlendState =
        BlendStateForPipeline(CommonPassParameters->PipelineState);
    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

    GraphicsPSOInit.bDepthBounds = true;

    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
        CommonPassParameters->VertexDeclarationRHI;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
        PixelShader.GetPixelShader();

    GraphicsPSOInit.bAllowVariableRateShading = false;

    SET_PIPELINE_STATE(RHICmdList,
                       GraphicsPSOInit,
                       CommonPassParameters->PipelineState.stencilReference);

    RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                           CommonPassParameters->Viewport.Min.Y,
                           0,
                           CommonPassParameters->Viewport.Max.X,
                           CommonPassParameters->Viewport.Max.Y,
                           1);

    RHICmdList.SetScissorRect(true,
                              CommonPassParameters->Scissor.Min.X,
                              CommonPassParameters->Scissor.Min.Y,
                              CommonPassParameters->Scissor.Max.X,
                              CommonPassParameters->Scissor.Max.Y);

    RHICmdList.SetDepthBounds(rive::gpu::DEPTH_MIN, rive::gpu::DEPTH_MAX);
    SetShaderParameters(RHICmdList,
                        VertexShader,
                        VertexShader.GetVertexShader(),
                        PassParameters->VS);
    SetShaderParameters(RHICmdList,
                        PixelShader,
                        PixelShader.GetPixelShader(),
                        PassParameters->PS);

    RHICmdList.SetStreamSource(0, CommonPassParameters->VertexBuffers[0], 0);
    RHICmdList.DrawPrimitive(CommonPassParameters->DrawBatch.baseElement,
                             CommonPassParameters->DrawBatch.elementCount / 3,
                             1);
}

void AddDrawMSAAAtlasBlitPass(
    FRHICommandList& RHICmdList,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAAtlasBlit");

    TShaderMapRef<FRiveRDGAtlasBlitMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGAtlasBlitMSAAPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil =
        StencilStateForPipeline(CommonPassParameters->PipelineState);

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.DepthStencilState = DepthStencil;

    GraphicsPSOInit.RasterizerState = RasterStateForCullModeAndDrawMode<true>(
        CommonPassParameters->PipelineState.cullFace,
        CommonPassParameters->bWireframe);

    GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

    GraphicsPSOInit.BlendState =
        BlendStateForPipeline(CommonPassParameters->PipelineState);

    GraphicsPSOInit.bDepthBounds = true;

    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
        CommonPassParameters->VertexDeclarationRHI;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
        PixelShader.GetPixelShader();

    GraphicsPSOInit.bAllowVariableRateShading = false;

    SET_PIPELINE_STATE(RHICmdList,
                       GraphicsPSOInit,
                       CommonPassParameters->PipelineState.stencilReference);

    RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                           CommonPassParameters->Viewport.Min.Y,
                           0,
                           CommonPassParameters->Viewport.Max.X,
                           CommonPassParameters->Viewport.Max.Y,
                           1);

    RHICmdList.SetScissorRect(true,
                              CommonPassParameters->Scissor.Min.X,
                              CommonPassParameters->Scissor.Min.Y,
                              CommonPassParameters->Scissor.Max.X,
                              CommonPassParameters->Scissor.Max.Y);

    RHICmdList.SetDepthBounds(rive::gpu::DEPTH_MIN, rive::gpu::DEPTH_MAX);

    SetShaderParameters(RHICmdList,
                        VertexShader,
                        VertexShader.GetVertexShader(),
                        PassParameters->VS);
    SetShaderParameters(RHICmdList,
                        PixelShader,
                        PixelShader.GetPixelShader(),
                        PassParameters->PS);

    RHICmdList.SetStreamSource(0, CommonPassParameters->VertexBuffers[0], 0);
    RHICmdList.DrawPrimitive(CommonPassParameters->DrawBatch.baseElement,
                             CommonPassParameters->DrawBatch.elementCount / 3,
                             1);
}

void AddDrawMSAAImageMeshPass(
    FRHICommandList& RHICmdList,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAImageMesh");

    TShaderMapRef<FRiveRDGImageMeshMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGImageMeshMSAAPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil =
        StencilStateForPipeline(CommonPassParameters->PipelineState);

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.DepthStencilState = DepthStencil;

    GraphicsPSOInit.RasterizerState = RasterStateForCullModeAndDrawMode<true>(
        CommonPassParameters->PipelineState.cullFace,
        CommonPassParameters->bWireframe);

    GraphicsPSOInit.PrimitiveType = PT_TriangleList;

    GraphicsPSOInit.BlendState =
        BlendStateForPipeline(CommonPassParameters->PipelineState);

    GraphicsPSOInit.bDepthBounds = true;

    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
        CommonPassParameters->VertexDeclarationRHI;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
        PixelShader.GetPixelShader();

    GraphicsPSOInit.bAllowVariableRateShading = false;

    SET_PIPELINE_STATE(RHICmdList,
                       GraphicsPSOInit,
                       CommonPassParameters->PipelineState.stencilReference);

    RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                           CommonPassParameters->Viewport.Min.Y,
                           0,
                           CommonPassParameters->Viewport.Max.X,
                           CommonPassParameters->Viewport.Max.Y,
                           1);

    RHICmdList.SetScissorRect(true,
                              CommonPassParameters->Scissor.Min.X,
                              CommonPassParameters->Scissor.Min.Y,
                              CommonPassParameters->Scissor.Max.X,
                              CommonPassParameters->Scissor.Max.Y);

    RHICmdList.SetDepthBounds(rive::gpu::DEPTH_MIN, rive::gpu::DEPTH_MAX);

    SetShaderParameters(RHICmdList,
                        VertexShader,
                        VertexShader.GetVertexShader(),
                        PassParameters->VS);
    SetShaderParameters(RHICmdList,
                        PixelShader,
                        PixelShader.GetPixelShader(),
                        PassParameters->PS);

    RHICmdList.SetStreamSource(0, CommonPassParameters->VertexBuffers[0], 0);
    RHICmdList.SetStreamSource(1, CommonPassParameters->VertexBuffers[1], 0);
    RHICmdList.DrawIndexedPrimitive(
        CommonPassParameters->IndexBuffer,
        0,
        0,
        NumVertices,
        0,
        CommonPassParameters->DrawBatch.elementCount / 3,
        1);
}

FRDGPassRef AddDrawInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGInteriorTrianglesVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGInteriorTrianglesPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Interior_Triangles"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.DrawPrimitive(
                CommonPassParameters->DrawBatch.baseElement,
                CommonPassParameters->DrawBatch.elementCount / 3,
                1);
        });
}

FRDGPassRef AddDrawAtlasBlitPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGAtlasBlitVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGAtlasBlitPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Atlas_Blit"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.DrawPrimitive(
                CommonPassParameters->DrawBatch.baseElement,
                CommonPassParameters->DrawBatch.elementCount / 3,
                1);
        });
}

FRDGPassRef AddDrawImageRectPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGImageRectVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGImageRectPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Image_Rect"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.DrawIndexedPrimitive(CommonPassParameters->IndexBuffer,
                                            0,
                                            0,
                                            std::size(kImageRectVertices),
                                            0,
                                            std::size(kImageRectIndices) / 3,
                                            1);
        });
}

FRDGPassRef AddDrawImageMeshPass(
    FRDGBuilder& GraphBuilder,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGImageMeshVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGImageMeshPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Image_Mesh"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters,
         PassParameters,
         NumVertices,
         VertexShader,
         PixelShader](FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.SetStreamSource(1,
                                       CommonPassParameters->VertexBuffers[1],
                                       0);
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,
                0,
                NumVertices,
                0,
                CommonPassParameters->DrawBatch.elementCount / 3,
                1);
        });
}

FRDGPassRef AddAtomicResolvePass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGAtomicResolveVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGAtomicResolvePixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Atomic_Resolve"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);
            GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);
            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);

            RHICmdList.DrawPrimitive(0, 2, 1);
        });
}

FRDGPassRef AddDrawRasterOrderPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGRasterOrderPathVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGRasterOrderPathPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);
    // PassParameters->VS.baseInstance = 0;
    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Raster_Order_Draw_Patch %s", *PassName),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,
                0,
                kPatchVertexBufferCount,
                PatchBaseIndex(CommonPassParameters->DrawBatch.drawType),
                PatchIndexCount(CommonPassParameters->DrawBatch.drawType) / 3,
                CommonPassParameters->DrawBatch.elementCount);
        });
}

FRDGPassRef AddDrawRasterOrderInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGRasterOrderInteriorTrianglesVertexShader>
        VertexShader(CommonPassParameters->ShaderMap,
                     CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGRasterOrderInteriorTrianglesPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Raster_Order_Interior_Triangles"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.DrawPrimitive(
                CommonPassParameters->DrawBatch.baseElement,
                CommonPassParameters->DrawBatch.elementCount / 3,
                1);
        });
}

FRDGPassRef AddDrawRasterOrderImageMeshPass(
    FRDGBuilder& GraphBuilder,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGRasterOrderImageMeshVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGRasterOrderImageMeshPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Raster_Order_Image_Mesh"),
        PassParameters,
        ERDGPassFlags::Raster,
        [CommonPassParameters,
         PassParameters,
         NumVertices,
         VertexShader,
         PixelShader](FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;

            GraphicsPSOInit.BlendState =
                RHIBlendStateForBlendType(CommonPassParameters->BlendType);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            RHICmdList.SetScissorRect(true,
                                      CommonPassParameters->Scissor.Min.X,
                                      CommonPassParameters->Scissor.Min.Y,
                                      CommonPassParameters->Scissor.Max.X,
                                      CommonPassParameters->Scissor.Max.Y);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(
                RHICmdList,
                GraphicsPSOInit,
                CommonPassParameters->PipelineState.stencilReference);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.SetStreamSource(0,
                                       CommonPassParameters->VertexBuffers[0],
                                       0);
            RHICmdList.SetStreamSource(1,
                                       CommonPassParameters->VertexBuffers[1],
                                       0);
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,
                0,
                NumVertices,
                0,
                CommonPassParameters->DrawBatch.elementCount / 3,
                1);
        });
}

FRDGPassRef AddFeatherAtlasFillDrawPass(FRDGBuilder& GraphBuilder,
                                        FRiveAtlasParameters* AtlasParameters,
                                        FRDGAtlasPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGDrawAtlasVertexShader> VertexShader(
        AtlasParameters->ShaderMap);
    TShaderMapRef<FRiveRDGDrawAtlasFillPixelShader> PixelShader(
        AtlasParameters->ShaderMap);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);
    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Atlas_Fill"),
        PassParameters,
        ERDGPassFlags::Raster,
        [AtlasParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.BlendState =
                TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One>::GetRHI();
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_CCW,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(AtlasParameters->Viewport.Min.X,
                                   AtlasParameters->Viewport.Min.Y,
                                   0,
                                   AtlasParameters->Viewport.Max.X,
                                   AtlasParameters->Viewport.Max.Y,
                                   1.0);

            FRHIBatchedShaderParameters& BatchedShaderParameters =
                RHICmdList.GetScratchShaderParameters();

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                AtlasParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();

            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);

            auto& batch = AtlasParameters->DrawBatch;

            RHICmdList.SetScissorRect(true,
                                      batch.scissor.left,
                                      batch.scissor.top,
                                      batch.scissor.right,
                                      batch.scissor.bottom);

            RHICmdList.SetStreamSource(0, AtlasParameters->VertexBuffer, 0);

            RHICmdList.DrawIndexedPrimitive(
                AtlasParameters->IndexBuffer,
                0,
                0,
                0,
                rive::gpu::kMidpointFanCenterAAPatchBaseIndex,
                rive::gpu::kMidpointFanCenterAAPatchIndexCount / 3,
                batch.patchCount);
        });
}

FRDGPassRef AddFeatherAtlasStrokeDrawPass(
    FRDGBuilder& GraphBuilder,
    FRiveAtlasParameters* AtlasParameters,
    FRDGAtlasPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGDrawAtlasVertexShader> VertexShader(
        AtlasParameters->ShaderMap);
    TShaderMapRef<FRiveRDGDrawAtlasStrokePixelShader> PixelShader(
        AtlasParameters->ShaderMap);

    SetFlushUniformsPerShader(PassParameters);

    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);
    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Atlas_Stroke"),
        PassParameters,
        ERDGPassFlags::Raster,
        [AtlasParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.BlendState =
                TStaticBlendState<CW_RGBA, BO_Max, BF_One, BF_One>::GetRHI();
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_CCW,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(AtlasParameters->Viewport.Min.X,
                                   AtlasParameters->Viewport.Min.Y,
                                   0,
                                   AtlasParameters->Viewport.Max.X,
                                   AtlasParameters->Viewport.Max.Y,
                                   1.0);

            FRHIBatchedShaderParameters& BatchedShaderParameters =
                RHICmdList.GetScratchShaderParameters();

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                AtlasParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();

            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            auto& batch = AtlasParameters->DrawBatch;

            RHICmdList.SetScissorRect(true,
                                      batch.scissor.left,
                                      batch.scissor.top,
                                      batch.scissor.right,
                                      batch.scissor.bottom);

            RHICmdList.SetStreamSource(0, AtlasParameters->VertexBuffer, 0);

            RHICmdList.DrawIndexedPrimitive(
                AtlasParameters->IndexBuffer,
                0,
                0,
                0,
                rive::gpu::kMidpointFanPatchBaseIndex,
                rive::gpu::kMidpointFanPatchBorderIndexCount / 3,
                batch.patchCount);
        });
}

FRDGPassRef AddDrawTextureBlt(FRDGBuilder& GraphBuilder,
                              FVertexDeclarationRHIRef VertexDeclarationRHI,
                              FUint32Rect Viewport,
                              FGlobalShaderMap* ShaderMap,
                              FRiveDrawTextureBltParameters* PassParameters)
{
    TShaderMapRef<FRiveBltTextureAsDrawVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveBltTextureAsDrawPixelShader> PixelShader(ShaderMap);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Texture_Blt"),
        PassParameters,
        ERDGPassFlags::Raster,
        [Viewport,
         VertexDeclarationRHI,
         PassParameters,
         VertexShader,
         PixelShader](FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp,
                             false);
            GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

            GraphicsPSOInit.BlendState =
                TStaticBlendState<CW_RGBA,
                                  BO_Add,
                                  BF_One,
                                  BF_InverseSourceAlpha,
                                  BO_Add,
                                  BF_One,
                                  BF_InverseSourceAlpha>::GetRHI();

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(Viewport.Min.X,
                                   Viewport.Min.Y,
                                   0,
                                   Viewport.Max.X,
                                   Viewport.Max.Y,
                                   1.0);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.DrawPrimitive(0, 2, 1);
        });
}

void AddDrawTextureBlt(FRHICommandList& RHICmdList,
                       FVertexDeclarationRHIRef VertexDeclarationRHI,
                       FGlobalShaderMap* ShaderMap,
                       FRiveDrawTextureBltParameters& PassParameters,
                       const rive::gpu::DrawBatch& Batch)
{
    TShaderMapRef<FRiveBltTextureAsDrawVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveBltTextureAsDrawPixelShader> PixelShader(ShaderMap);

    // PassParameters.PS.beRed = 1;
    RHICmdList.BeginRenderPass(GetRenderPassInfo(&PassParameters),
                               TEXT("Rive_Draw_Texture_Blt"));

    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    GraphicsPSOInit.DepthStencilState =
        TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
    GraphicsPSOInit.RasterizerState =
        RASTER_STATE(FM_Solid,
                     CM_None,
                     ERasterizerDepthClipMode::DepthClamp,
                     false);
    GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

    GraphicsPSOInit.BlendState =
        TStaticBlendState<CW_RGBA,
                          BO_Add,
                          BF_One,
                          BF_InverseSourceAlpha,
                          BO_Add,
                          BF_One,
                          BF_InverseSourceAlpha>::GetRHI();

    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

    auto RT = PassParameters.RenderTargets[0].GetTexture();
    check(RT);
    auto RTSize = RT->Desc.GetSize();

    RHICmdList.SetViewport(0, 0, 0, RTSize.X, RTSize.Y, 1.0);

    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
        VertexDeclarationRHI;
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
        VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
        PixelShader.GetPixelShader();

    SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, 0);

    SetShaderParameters(RHICmdList,
                        PixelShader,
                        PixelShader.GetPixelShader(),
                        PassParameters.PS);
    for (const rive::gpu::Draw* draw = Batch.dstReadList; draw != nullptr;
         draw = draw->nextDstRead())
    {
        const auto& PixelBounds = draw->pixelBounds();
        RHICmdList.SetScissorRect(true,
                                  PixelBounds.left,
                                  PixelBounds.top,
                                  PixelBounds.right,
                                  PixelBounds.bottom);

        RHICmdList.DrawPrimitive(0, 2, 1);
    }

    RHICmdList.EndRenderPass();
}
