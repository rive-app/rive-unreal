#include "RivePassFunctions.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphEvent.h"
#include "RenderGraphUtils.h"
#include "HLSLTree/HLSLTreeTypes.h"

using namespace rive::gpu;

BEGIN_SHADER_PARAMETER_STRUCT(FRDGPassParameters, )
SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FFlushUniforms, FlushUniforms)
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
                             ERasterizerDepthClipMode::DepthClamp);
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

            BindShaders(RHICmdList,
                        GraphicsPSOInit,
                        VertexShader,
                        PixelShader,
                        VertexDeclaration);

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

    TShaderMapRef<FRiveRDGTessVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveRDGTessPixelShader> PixelShader(ShaderMap);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Tesselation_Update"),
        TesselationPassParameters,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
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
                             ERasterizerDepthClipMode::DepthClamp);

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            FRHIBatchedShaderParameters& BatchedShaderParameters =
                RHICmdList.GetScratchShaderParameters();

            BindShaders(RHICmdList,
                        GraphicsPSOInit,
                        VertexShader,
                        PixelShader,
                        VertexDeclaration);

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

FRDGPassRef AddDrawPatchesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRivePatchPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGPathVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGPathPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);
    // PassParameters->VS.baseInstance = 0;
    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Patch"),
        PassParameters,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            if (CommonPassParameters->NeedsSourceBlending)
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_RGBA,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha>::GetRHI();
            else
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_NONE>::CreateRHI();

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SetGraphicsPipelineState(RHICmdList,
                                     GraphicsPSOInit,
                                     0,
                                     EApplyRendertargetOption::CheckApply,
                                     true,
                                     EPSOPrecacheResult::Unknown);

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

FRDGPassRef AddDrawInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveInteriorTrianglePassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGInteriorTrianglesVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGInteriorTrianglesPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Interior_Triangles"),
        PassParameters,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                GetStaticRasterizerState<false>(FM_Solid, CM_CCW);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            if (CommonPassParameters->NeedsSourceBlending)
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_RGBA,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha>::GetRHI();
            else
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_NONE>::CreateRHI();

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SetGraphicsPipelineState(RHICmdList,
                                     GraphicsPSOInit,
                                     0,
                                     EApplyRendertargetOption::CheckApply,
                                     true,
                                     EPSOPrecacheResult::Unknown);

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
    FRiveImageRectPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGImageRectVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGImageRectPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Image_Rect"),
        PassParameters,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;

            if (CommonPassParameters->NeedsSourceBlending)
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_RGBA,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha>::GetRHI();
            else
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_NONE>::CreateRHI();

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SetGraphicsPipelineState(RHICmdList,
                                     GraphicsPSOInit,
                                     0,
                                     EApplyRendertargetOption::CheckApply,
                                     true,
                                     EPSOPrecacheResult::Unknown);

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
    FRiveImageMeshPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGImageMeshVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGImageMeshPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    ClearUnusedGraphResources(VertexShader, &PassParameters->VS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Image_Mesh"),
        PassParameters,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
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
                             ERasterizerDepthClipMode::DepthClamp);
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;

            if (CommonPassParameters->NeedsSourceBlending)
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_RGBA,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha>::GetRHI();
            else
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_NONE>::CreateRHI();

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SetGraphicsPipelineState(RHICmdList,
                                     GraphicsPSOInit,
                                     0,
                                     EApplyRendertargetOption::CheckApply,
                                     true,
                                     EPSOPrecacheResult::Unknown);

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
    FRiveAtomicResolvePassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGAtomicResolveVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<FRiveRDGAtomicResolvePixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Atomic_Resolve"),
        PassParameters,
        ERDGPassFlags::Raster | ERDGPassFlags::NeverParallel,
        [CommonPassParameters, PassParameters, VertexShader, PixelShader](
            FRHICommandList& RHICmdList) {
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
                             ERasterizerDepthClipMode::DepthClamp);
            GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

            if (CommonPassParameters->NeedsSourceBlending)
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_RGBA,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha,
                                      BO_Add,
                                      BF_One,
                                      BF_InverseSourceAlpha>::GetRHI();
            else
                GraphicsPSOInit.BlendState =
                    TStaticBlendState<CW_NONE>::CreateRHI();

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            RHICmdList.SetViewport(CommonPassParameters->Viewport.Min.X,
                                   CommonPassParameters->Viewport.Min.Y,
                                   0,
                                   CommonPassParameters->Viewport.Max.X,
                                   CommonPassParameters->Viewport.Max.Y,
                                   1.0);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();

            SetGraphicsPipelineState(RHICmdList,
                                     GraphicsPSOInit,
                                     0,
                                     EApplyRendertargetOption::CheckApply,
                                     true,
                                     EPSOPrecacheResult::Unknown);

            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);

            RHICmdList.DrawPrimitive(0, 2, 1);
        });
}