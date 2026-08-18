// Copyright 2024-2026 Rive, Inc. All rights reserved.

#include "RivePassFunctions.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphEvent.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"  // TStaticBlendState, etc.
#include "RHICommandList.h"   // FRHICommandList
#include "RenderGraphUtils.h" // Graph building helpers
#include "rive/renderer/draw.hpp"
#include <ClearQuad.h>

#if !UE_VERSION_OLDER_THAN(5, 7, 0)
#include <GlobalRenderResources.h>
#endif

#include "RiveShaderTypes.h"
#include "PlatformRHI.h"

#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(RiveMSAA, true);

#if defined(UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE)
static TAutoConsoleVariable<int32> CVarRiveDynamicPipelineState(
    TEXT("r.rive.dynamicpipelinestate"),
    1,
    TEXT("How the msaa midpoint fan passes reach the gpu, when the rhi "
         "supports dynamic pipeline state.\n")
        TEXT("  0: one pipeline per pass, no overrides\n") TEXT(
            "  1: one pipeline for all three passes, state varied by "
            "override\n")
            TEXT("  2: one pipeline per pass AND a redundant override carrying "
                 "that pass's own values. Diagnostic: it should be a no-op "
                 "against 0, so a difference isolates the override mechanism "
                 "from the collapse itself."),
    ECVF_Scalability | ECVF_RenderThreadSafe);
#endif // UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE

using namespace rive::gpu;

// Used to skip redundant pipeline binds inside one rive render pass. Render
// targets are not compared: they cannot change inside a render pass, and
// RiveInvalidateBoundPipelineState() resets this when one begins.
namespace
{
struct FRiveBoundPipeline
{
    FRHIDepthStencilState* DepthStencil = nullptr;
    FRHIRasterizerState* Rasterizer = nullptr;
    FRHIBlendState* Blend = nullptr;
    FRHIVertexDeclaration* VertexDeclaration = nullptr;
    FRHIVertexShader* VertexShader = nullptr;
    FRHIPixelShader* PixelShader = nullptr;
    uint32 PrimitiveType = 0xffffffff;
    bool bDepthBounds = false;
    bool bValid = false;

    bool Matches(const FGraphicsPipelineStateInitializer& Init) const
    {
        return bValid && DepthStencil == Init.DepthStencilState &&
               Rasterizer == Init.RasterizerState && Blend == Init.BlendState &&
               VertexDeclaration ==
                   Init.BoundShaderState.VertexDeclarationRHI &&
               VertexShader == Init.BoundShaderState.VertexShaderRHI &&
               PixelShader == Init.BoundShaderState.PixelShaderRHI &&
               PrimitiveType == (uint32)Init.PrimitiveType &&
               bDepthBounds == Init.bDepthBounds;
    }

    void Record(const FGraphicsPipelineStateInitializer& Init)
    {
        DepthStencil = Init.DepthStencilState;
        Rasterizer = Init.RasterizerState;
        Blend = Init.BlendState;
        VertexDeclaration = Init.BoundShaderState.VertexDeclarationRHI;
        VertexShader = Init.BoundShaderState.VertexShaderRHI;
        PixelShader = Init.BoundShaderState.PixelShaderRHI;
        PrimitiveType = (uint32)Init.PrimitiveType;
        bDepthBounds = Init.bDepthBounds;
        bValid = true;
    }
};

FRiveBoundPipeline GBoundPipeline;
uint32 GBoundStencilRef = 0;
} // namespace

void RiveInvalidateBoundPipelineState() { GBoundPipeline.bValid = false; }

// Binds only if the batch actually needs a different pipeline. A batch that
// differs only in stencil reference gets a SetStencilRef instead, which is
// dynamic state and far cheaper than going back through the pso cache.
static void RiveSetGraphicsPipelineState(
    FRHICommandList& RHICmdList,
    const FGraphicsPipelineStateInitializer& GraphicsPSOInit,
    uint32 StencilRef)
{
    if (GBoundPipeline.Matches(GraphicsPSOInit))
    {
        if (GBoundStencilRef != StencilRef)
        {
            RHICmdList.SetStencilRef(StencilRef);
            GBoundStencilRef = StencilRef;
        }
        return;
    }

    SET_PIPELINE_STATE(RHICmdList, GraphicsPSOInit, StencilRef);
    GBoundPipeline.Record(GraphicsPSOInit);
    GBoundStencilRef = StencilRef;
}

// Vulkan's InstanceIndex already includes the draw's first instance, so the
// base instance rides along as FirstInstance and the shader reads it straight
// off SV_InstanceID.
static uint32 RiveFirstInstance(uint32 BaseInstance)
{
    return IsVulkanPlatform(GMaxRHIShaderPlatform) ? BaseInstance : 0;
}

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
    const PipelineState& PipelineState,
    uint32_t uniqueKey)
{
    // We shouldn't need locks here because this should only ever happen from
    // the render thread.
    static TMap<uint32_t, FDepthStencilStateRHIRef> StencilStates;
    if (FDepthStencilStateRHIRef* Value = StencilStates.Find(uniqueKey))
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
        StencilOpForStencilFaceOps(FrontOps.stencilFailOp),
        StencilOpForStencilFaceOps(FrontOps.depthFailOp),
        StencilOpForStencilFaceOps(FrontOps.depthStencilPassOp),
        PipelineState.stencilTestEnabled,
        CompareOpForStencilCompareOps(BackOps.compareOp),
        StencilOpForStencilFaceOps(BackOps.stencilFailOp),
        StencilOpForStencilFaceOps(BackOps.depthFailOp),
        StencilOpForStencilFaceOps(BackOps.depthStencilPassOp),
        PipelineState.stencilCompareMask,
        PipelineState.stencilWriteMask);

    auto State = RHICreateDepthStencilState(Initializer);
    StencilStates.Add(uniqueKey, State);
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
#if !UE_VERSION_OLDER_THAN(5, 5, 0) && !RIVE_FORCE_USE_GENERATED_UNIFORMS
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.Gradient");
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
        // Skip the render pass so we can more tightly control transitions
        ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [TessSpanBuffer,
         TessIndexBuffer,
         VertexDeclaration,
         Viewport,
         NumTessellations,
         VertexShader,
         PixelShader,
         TesselationPassParameters](FRHICommandList& RHICmdList) {
            // Transition the tesselation texture since we are controlling its
            // state.
            if (GRiveRHINeedsExplicitVertexReadBarrier)
            {
                RHICmdList.Transition(
                    FRHITransitionInfo(
                        TesselationPassParameters->RenderTargets[0]
                            .GetTexture()
                            ->GetRHI(),
                        ERHIAccess::Unknown,
                        ERHIAccess::RTV),
                    ERHITransitionCreateFlags::None);
            }
            RHICmdList.BeginRenderPass(
                GetRenderPassInfo(TesselationPassParameters),
                TEXT("Rive_Tesselation_Update"));

            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.Tesselation");
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

            RHICmdList.EndRenderPass();
            // This is guaranteed not accessed from pixel shaders. So do
            // SRVGraphicsNonPixel as the final state.
            if (GRiveRHINeedsExplicitVertexReadBarrier)
            {
                RHICmdList.Transition(FRHITransitionInfo(
                    TesselationPassParameters->RenderTargets[0]
                        .GetTexture()
                        ->GetRHI(),
                    ERHIAccess::RTV,
                    ERHIAccess::SRVGraphicsNonPixel));
            }
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

template <typename TPixelShader>
static void AddDrawMSAAPatchesPassImpl(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAPatches");

    TShaderMapRef<FRiveRDGPathMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<TPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    CSV_CUSTOM_STAT(RiveMSAA, Draws, 1, ECsvCustomStatOp::Accumulate);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil = StencilStateForPipeline(
        CommonPassParameters->PipelineState,
        CommonPassParameters->GetUniqueKey(InterlockMode::msaa));

    CSV_SCOPED_TIMING_STAT(RiveMSAA, PSOBuild);
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

    {
        CSV_SCOPED_TIMING_STAT(RiveMSAA, PSOBind);
        RiveSetGraphicsPipelineState(
            RHICmdList,
            GraphicsPSOInit,
            CommonPassParameters->PipelineState.stencilReference);
    }

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

    {
        CSV_SCOPED_TIMING_STAT(RiveMSAA, BindParams);
        SetShaderParameters(RHICmdList,
                            VertexShader,
                            VertexShader.GetVertexShader(),
                            PassParameters->VS);
        SetShaderParameters(RHICmdList,
                            PixelShader,
                            PixelShader.GetPixelShader(),
                            PassParameters->PS);
    }

    CSV_SCOPED_TIMING_STAT(RiveMSAA, Draw);
    RHICmdList.SetStreamSource(0, CommonPassParameters->VertexBuffers[0], 0);
    RHICmdList.DrawIndexedPrimitive(
        CommonPassParameters->IndexBuffer,
        0,
        RiveFirstInstance(PassParameters->VS.baseInstance),
        kPatchVertexBufferCount,
        CommonPassParameters->DrawBatch.baseIndex,
        CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
        CommonPassParameters->DrawBatch.elementCount);
}

#if defined(UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE)
// The three collapsed subpasses, in the order the stencil algorithm needs them.
// Must match the native vulkan backend's for msaaDynamicMidpointFans.
static constexpr DrawType kDynamicMidpointFanPasses[] = {
    DrawType::msaaMidpointFanBorrowedCoverage,
    DrawType::msaaMidpointFans,
    DrawType::msaaMidpointFanStencilReset,
};

template <typename TPixelShader>
static void AddDrawMSAADynamicMidpointFansPassImpl(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters,
    TConstArrayView<rive::gpu::PipelineState> PassPipelineStates)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAADynamicMidpointFans");

    check(PassPipelineStates.Num() ==
          UE_ARRAY_COUNT(kDynamicMidpointFanPasses));

    TShaderMapRef<FRiveRDGPathMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<TPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    CSV_CUSTOM_STAT(RiveMSAA, Draws, 1, ECsvCustomStatOp::Accumulate);

    // Everything below here is shared by all three passes, which is the point
    // of the collapse: one batch's worth of setup instead of three.
    SetFlushUniformsPerShader(PassParameters);

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
    RHICmdList.SetStreamSource(0, CommonPassParameters->VertexBuffers[0], 0);

    // Which way the three passes reach the gpu.
    const int32 DynamicStateMode =
        GRHISupportsDynamicPipelineState
            ? CVarRiveDynamicPipelineState.GetValueOnRenderThread()
            : 0;

    // The three passes differ only in depth/stencil, cull face and colour
    // write, so they can share one pipeline with per-pass overrides.
    if (DynamicStateMode == 1)
    {
        // The shared pipeline is built from pass 0 with colour writes forced
        // ON: dynamic color-write can only mask writes out, never add them to
        // a pipeline whose static write mask is empty.
        rive::gpu::PipelineState ColorWritingState = PassPipelineStates[0];
        ColorWritingState.colorWriteEnabled = true;

        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        {
            CSV_SCOPED_TIMING_STAT(RiveMSAA, PSOBuild);
            GraphicsPSOInit.DepthStencilState = StencilStateForPipeline(
                PassPipelineStates[0],
                CommonPassParameters->GetUniqueKeyForDrawType(
                    kDynamicMidpointFanPasses[0],
                    InterlockMode::msaa));
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<true>(
                    PassPipelineStates[0].cullFace,
                    CommonPassParameters->bWireframe);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
            GraphicsPSOInit.BlendState =
                BlendStateForPipeline(ColorWritingState);
            GraphicsPSOInit.bDepthBounds = true;

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();
            GraphicsPSOInit.bAllowVariableRateShading = false;
        }

        {
            CSV_SCOPED_TIMING_STAT(RiveMSAA, PSOBind);
            RiveSetGraphicsPipelineState(
                RHICmdList,
                GraphicsPSOInit,
                PassPipelineStates[0].stencilReference);
        }

        // One pipeline means one descriptor state for all three draws, so
        // unlike the loop below the parameters only have to be set once.
        {
            CSV_SCOPED_TIMING_STAT(RiveMSAA, BindParams);
            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);
        }

        for (int32 PassIndex = 0; PassIndex < PassPipelineStates.Num();
             ++PassIndex)
        {
            const rive::gpu::PipelineState& PassPipelineState =
                PassPipelineStates[PassIndex];

            // Held alive by the cache inside StencilStateForPipeline, so it
            // outlives execution of a deferred command list.
            FDepthStencilStateRHIRef DepthStencil = StencilStateForPipeline(
                PassPipelineState,
                CommonPassParameters->GetUniqueKeyForDrawType(
                    kDynamicMidpointFanPasses[PassIndex],
                    InterlockMode::msaa));

            RHICmdList.SetDynamicPipelineStateOverride(
                DepthStencil.GetReference(),
                PassPipelineState.stencilReference,
                RasterStateForCullModeAndDrawMode<true>(
                    PassPipelineState.cullFace,
                    CommonPassParameters->bWireframe),
                PassPipelineState.colorWriteEnabled);

            CSV_SCOPED_TIMING_STAT(RiveMSAA, Draw);
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,
                RiveFirstInstance(PassParameters->VS.baseInstance),
                kPatchVertexBufferCount,
                CommonPassParameters->DrawBatch.baseIndex,
                CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
                CommonPassParameters->DrawBatch.elementCount);
        }

        // The rhi also drops the override on the next pipeline bind, but the
        // batch owns it, so end it here rather than relying on whatever runs
        // next to do that.
        RHICmdList.ClearDynamicPipelineStateOverride();
        return;
    }

    for (int32 PassIndex = 0; PassIndex < PassPipelineStates.Num(); ++PassIndex)
    {
        const rive::gpu::PipelineState& PassPipelineState =
            PassPipelineStates[PassIndex];

        FDepthStencilStateRHIRef DepthStencil = StencilStateForPipeline(
            PassPipelineState,
            CommonPassParameters->GetUniqueKeyForDrawType(
                kDynamicMidpointFanPasses[PassIndex],
                InterlockMode::msaa));

        FGraphicsPipelineStateInitializer GraphicsPSOInit;
        {
            CSV_SCOPED_TIMING_STAT(RiveMSAA, PSOBuild);
            GraphicsPSOInit.DepthStencilState = DepthStencil;
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<true>(
                    PassPipelineState.cullFace,
                    CommonPassParameters->bWireframe);
            GraphicsPSOInit.PrimitiveType = EPrimitiveType::PT_TriangleList;
            GraphicsPSOInit.BlendState =
                BlendStateForPipeline(PassPipelineState);
            GraphicsPSOInit.bDepthBounds = true;

            RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

            GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
                CommonPassParameters->VertexDeclarationRHI;
            GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
                VertexShader.GetVertexShader();
            GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
                PixelShader.GetPixelShader();
            GraphicsPSOInit.bAllowVariableRateShading = false;
        }

        {
            CSV_SCOPED_TIMING_STAT(RiveMSAA, PSOBind);
            RiveSetGraphicsPipelineState(RHICmdList,
                                         GraphicsPSOInit,
                                         PassPipelineState.stencilReference);
        }

        // Each pipeline carries its own descriptor state in the rhi, so the
        // parameters have to be re-set after every pipeline change even though
        // the values are identical.
        {
            CSV_SCOPED_TIMING_STAT(RiveMSAA, BindParams);
            SetShaderParameters(RHICmdList,
                                VertexShader,
                                VertexShader.GetVertexShader(),
                                PassParameters->VS);
            SetShaderParameters(RHICmdList,
                                PixelShader,
                                PixelShader.GetPixelShader(),
                                PassParameters->PS);
        }

        // Diagnostic mode 2: same pipelines as mode 0, plus a redundant
        // override restating each pipeline's own values.
        if (DynamicStateMode == 2)
        {
            RHICmdList.SetDynamicPipelineStateOverride(
                DepthStencil.GetReference(),
                PassPipelineState.stencilReference,
                RasterStateForCullModeAndDrawMode<true>(
                    PassPipelineState.cullFace,
                    CommonPassParameters->bWireframe),
                PassPipelineState.colorWriteEnabled);
        }

        CSV_SCOPED_TIMING_STAT(RiveMSAA, Draw);
        RHICmdList.DrawIndexedPrimitive(
            CommonPassParameters->IndexBuffer,
            0,
            RiveFirstInstance(PassParameters->VS.baseInstance),
            kPatchVertexBufferCount,
            CommonPassParameters->DrawBatch.baseIndex,
            CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
            CommonPassParameters->DrawBatch.elementCount);
    }

    if (DynamicStateMode == 2)
    {
        RHICmdList.ClearDynamicPipelineStateOverride();
    }
}

void AddDrawMSAADynamicMidpointFansPass(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters,
    TConstArrayView<rive::gpu::PipelineState> PassPipelineStates)
{
    if (CommonPassParameters->UseSubpassPixelShader())
    {
        AddDrawMSAADynamicMidpointFansPassImpl<
            FRiveRDGPathMSAASubpassPixelShader>(RHICmdList,
                                                PassName,
                                                CommonPassParameters,
                                                PassParameters,
                                                PassPipelineStates);
    }
    else
    {
        AddDrawMSAADynamicMidpointFansPassImpl<FRiveRDGPathMSAAPixelShader>(
            RHICmdList,
            PassName,
            CommonPassParameters,
            PassParameters,
            PassPipelineStates);
    }
}
#endif // UE_RHI_HAS_DYNAMIC_PIPELINE_STATE_OVERRIDE

void AddDrawMSAAPatchesPass(
    FRHICommandList& RHICmdList,
    const FString& PassName,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    if (CommonPassParameters->UseSubpassPixelShader())
    {
        AddDrawMSAAPatchesPassImpl<FRiveRDGPathMSAASubpassPixelShader>(
            RHICmdList,
            PassName,
            CommonPassParameters,
            PassParameters);
    }
    else
    {
        AddDrawMSAAPatchesPassImpl<FRiveRDGPathMSAAPixelShader>(
            RHICmdList,
            PassName,
            CommonPassParameters,
            PassParameters);
    }
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

    auto DepthStencil = StencilStateForPipeline(
        CommonPassParameters->PipelineState,
        CommonPassParameters->GetUniqueKey(InterlockMode::msaa));

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

    RiveSetGraphicsPipelineState(
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

template <typename TPixelShader>
static void AddDrawMSAAAtlasBlitPassImpl(
    FRHICommandList& RHICmdList,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAAtlasBlit");

    TShaderMapRef<FRiveRDGAtlasBlitMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<TPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil = StencilStateForPipeline(
        CommonPassParameters->PipelineState,
        CommonPassParameters->GetUniqueKey(InterlockMode::msaa));

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

    RiveSetGraphicsPipelineState(
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
    if (CommonPassParameters->UseSubpassPixelShader())
    {
        AddDrawMSAAAtlasBlitPassImpl<FRiveRDGAtlasBlitMSAASubpassPixelShader>(
            RHICmdList,
            CommonPassParameters,
            PassParameters);
    }
    else
    {
        AddDrawMSAAAtlasBlitPassImpl<FRiveRDGAtlasBlitMSAAPixelShader>(
            RHICmdList,
            CommonPassParameters,
            PassParameters);
    }
}

template <typename TPixelShader>
static void AddDrawMSAAImageMeshPassImpl(
    FRHICommandList& RHICmdList,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    RHI_BREADCRUMB_EVENT(RHICmdList, "rive.MSAAImageMesh");

    TShaderMapRef<FRiveRDGImageMeshMSAAVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);
    TShaderMapRef<TPixelShader> PixelShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->PixelPermutationDomain);

    SetFlushUniformsPerShader(PassParameters);

    auto DepthStencil = StencilStateForPipeline(
        CommonPassParameters->PipelineState,
        CommonPassParameters->GetUniqueKey(InterlockMode::msaa));

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

    RiveSetGraphicsPipelineState(
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
    // Per-instance gpu::ImageDrawInstance data on stream 2, offset here instead
    // of using FirstInstance, which isn't portable (GRHISupportsFirstInstance).
    RHICmdList.SetStreamSource(2,
                               CommonPassParameters->VertexBuffers[2],
                               CommonPassParameters->DrawBatch.baseElement *
                                   sizeof(rive::gpu::ImageDrawInstance));
    RHICmdList.DrawIndexedPrimitive(
        CommonPassParameters->IndexBuffer,
        0,                                         // BaseVertexIndex
        0,                                         // FirstInstance
        NumVertices,                               // NumVertices
        CommonPassParameters->DrawBatch.baseIndex, // StartIndex
        CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
        CommonPassParameters->DrawBatch.elementCount);
}

void AddDrawMSAAImageMeshPass(
    FRHICommandList& RHICmdList,
    uint32_t NumVertices,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveMSAAFlushPassParameters* PassParameters)
{
    if (CommonPassParameters->UseSubpassPixelShader())
    {
        AddDrawMSAAImageMeshPassImpl<FRiveRDGImageMeshMSAASubpassPixelShader>(
            RHICmdList,
            NumVertices,
            CommonPassParameters,
            PassParameters);
    }
    else
    {
        AddDrawMSAAImageMeshPassImpl<FRiveRDGImageMeshMSAAPixelShader>(
            RHICmdList,
            NumVertices,
            CommonPassParameters,
            PassParameters);
    }
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
#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
    using PixelShaderType = FRiveABRDGPathPixelShader;
#else
    using PixelShaderType = FRiveRDGPathPixelShader;
#endif

    TShaderMapRef<PixelShaderType> PixelShader(
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.AtomicPatch");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<false>(
                    CullFace::counterclockwise,
                    CommonPassParameters->bWireframe);
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
                CommonPassParameters->DrawBatch.baseIndex,
                CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
                CommonPassParameters->DrawBatch.elementCount);
        });
}

FRDGPassRef AddDrawInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveFlushPassParameters* PassParameters)
{
    TShaderMapRef<FRiveRDGInteriorTrianglesVertexShader> VertexShader(
        CommonPassParameters->ShaderMap,
        CommonPassParameters->VertexPermutationDomain);

#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
    using PixelShaderType = FRiveABRDGInteriorTrianglesPixelShader;
#else
    using PixelShaderType = FRiveRDGInteriorTrianglesPixelShader;
#endif

    TShaderMapRef<PixelShaderType> PixelShader(
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.InteriorTriangles");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<false>(
                    CullFace::counterclockwise,
                    CommonPassParameters->bWireframe);
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

#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
    using PixelShaderType = FRiveABRDGAtlasBlitPixelShader;
#else
    using PixelShaderType = FRiveRDGAtlasBlitPixelShader;
#endif

    TShaderMapRef<PixelShaderType> PixelShader(
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.AtlasBlit");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<false>(
                    CullFace::counterclockwise,
                    CommonPassParameters->bWireframe);
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

#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
    using PixelShaderType = FRiveABRDGImageRectPixelShader;
#else
    using PixelShaderType = FRiveRDGImageRectPixelShader;
#endif

    TShaderMapRef<PixelShaderType> PixelShader(
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.ImageRect");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                CommonPassParameters->bWireframe
                    ? RASTER_STATE(FM_Wireframe,
                                   CM_None,
                                   ERasterizerDepthClipMode::DepthClamp,
                                   false)
                    : RASTER_STATE(FM_Solid,
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
            // Per-instance gpu::ImageDrawInstance data on stream 1, offset here
            // instead of using FirstInstance, which isn't portable
            // (GRHISupportsFirstInstance).
            RHICmdList.SetStreamSource(
                1,
                CommonPassParameters->VertexBuffers[1],
                CommonPassParameters->DrawBatch.baseElement *
                    sizeof(rive::gpu::ImageDrawInstance));
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,                                // BaseVertexIndex
                0,                                // FirstInstance
                std::size(kImageRectVertices),    // NumVertices
                0,                                // StartIndex
                std::size(kImageRectIndices) / 3, // NumPrimitives
                CommonPassParameters->DrawBatch.elementCount); // NumInstances
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

#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
    using PixelShaderType = FRiveABRDGImageMeshPixelShader;
#else
    using PixelShaderType = FRiveRDGImageMeshPixelShader;
#endif

    TShaderMapRef<PixelShaderType> PixelShader(
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.ImageMesh");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                CommonPassParameters->bWireframe
                    ? RASTER_STATE(FM_Wireframe,
                                   CM_None,
                                   ERasterizerDepthClipMode::DepthClamp,
                                   false)
                    : RASTER_STATE(FM_Solid,
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
            // Per-instance gpu::ImageDrawInstance data on stream 2, offset here
            // instead of using FirstInstance, which isn't portable
            // (GRHISupportsFirstInstance).
            RHICmdList.SetStreamSource(
                2,
                CommonPassParameters->VertexBuffers[2],
                CommonPassParameters->DrawBatch.baseElement *
                    sizeof(rive::gpu::ImageDrawInstance));
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,                                         // BaseVertexIndex
                0,                                         // FirstInstance
                NumVertices,                               // NumVertices
                CommonPassParameters->DrawBatch.baseIndex, // StartIndex
                CommonPassParameters->DrawBatch.indexCountPerInstance /
                    3,                                         // NumPrimitives
                CommonPassParameters->DrawBatch.elementCount); // NumInstances
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

#if PLATFORM_APPLE || FORCE_ATOMIC_BUFFER
    using PixelShaderType = FRiveABRDGAtomicResolvePixelShader;
#else
    using PixelShaderType = FRiveRDGAtomicResolvePixelShader;
#endif

    TShaderMapRef<PixelShaderType> PixelShader(
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.AtomicResolve");
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
    FRiveRasterOrderFlushPassParameters* PassParameters)
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.RasterOrderDrawPatch");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<false>(
                    CullFace::counterclockwise,
                    CommonPassParameters->bWireframe);
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
                CommonPassParameters->DrawBatch.baseIndex,
                CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
                CommonPassParameters->DrawBatch.elementCount);
        });
}

FRDGPassRef AddDrawRasterOrderInteriorTrianglesPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveRasterOrderFlushPassParameters* PassParameters)
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
            RHI_BREADCRUMB_EVENT(RHICmdList,
                                 "rive.RasterOrderInteriorTriangles");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<false>(
                    CullFace::counterclockwise,
                    CommonPassParameters->bWireframe);
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

FRDGPassRef AddDrawRasterOrderAtlasBlitPass(
    FRDGBuilder& GraphBuilder,
    const FRiveCommonPassParameters* CommonPassParameters,
    FRiveRasterOrderFlushPassParameters* PassParameters)
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.AtlasBlit");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                RasterStateForCullModeAndDrawMode<false>(
                    CullFace::counterclockwise,
                    CommonPassParameters->bWireframe);
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
    FRiveRasterOrderFlushPassParameters* PassParameters)
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.RasterOrderImageMesh");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.RasterizerState =
                CommonPassParameters->bWireframe
                    ? RASTER_STATE(FM_Wireframe,
                                   CM_None,
                                   ERasterizerDepthClipMode::DepthClamp,
                                   false)
                    : RASTER_STATE(FM_Solid,
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
            // Per-instance gpu::ImageDrawInstance data on stream 2, offset here
            // instead of using FirstInstance, which isn't portable
            // (GRHISupportsFirstInstance).
            RHICmdList.SetStreamSource(
                2,
                CommonPassParameters->VertexBuffers[2],
                CommonPassParameters->DrawBatch.baseElement *
                    sizeof(rive::gpu::ImageDrawInstance));
            RHICmdList.DrawIndexedPrimitive(
                CommonPassParameters->IndexBuffer,
                0,                                         // BaseVertexIndex
                0,                                         // FirstInstance
                NumVertices,                               // NumVertices
                CommonPassParameters->DrawBatch.baseIndex, // StartIndex
                CommonPassParameters->DrawBatch.indexCountPerInstance / 3,
                CommonPassParameters->DrawBatch.elementCount);
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.AtlasFill");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.BlendState =
                TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One>::GetRHI();
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
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
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.AtlasStroke");
            FGraphicsPipelineStateInitializer GraphicsPSOInit;
            GraphicsPSOInit.BlendState =
                TStaticBlendState<CW_RGBA, BO_Max, BF_One, BF_One>::GetRHI();
            GraphicsPSOInit.DepthStencilState =
                TStaticDepthStencilState<false,
                                         ECompareFunction::CF_Always>::GetRHI();
            GraphicsPSOInit.PrimitiveType = PT_TriangleList;
            GraphicsPSOInit.RasterizerState =
                RASTER_STATE(FM_Solid,
                             CM_None,
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
                              FRiveDrawTextureBltParameters* PassParameters,
                              bool isMSAAResolve,
                              bool bOpaque)
{
    FRiveBltTextureAsDrawPixelShader::FPermutationDomain Domain;
    Domain.Set<FEnableMSAASourceTexture>(isMSAAResolve);

    TShaderMapRef<FRiveBltTextureAsDrawVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveBltTextureAsDrawPixelShader> PixelShader(ShaderMap,
                                                                Domain);

    ClearUnusedGraphResources(PixelShader, &PassParameters->PS);
    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("Rive_Draw_Texture_Blt"),
        PassParameters,
        ERDGPassFlags::Raster,
        [Viewport,
         VertexDeclarationRHI,
         PassParameters,
         VertexShader,
         PixelShader,
         bOpaque](FRHICommandList& RHICmdList) {
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.TextureBlt");
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
                bOpaque ? TStaticBlendState<>::GetRHI()
                        : TStaticBlendState<CW_RGBA,
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
                       const rive::gpu::DrawBatch& Batch,
                       bool isMSAAResolve)
{
    FRiveBltTextureAsDrawPixelShader::FPermutationDomain Domain;
    Domain.Set<FEnableMSAASourceTexture>(isMSAAResolve);

    TShaderMapRef<FRiveBltTextureAsDrawVertexShader> VertexShader(ShaderMap);
    TShaderMapRef<FRiveBltTextureAsDrawPixelShader> PixelShader(ShaderMap,
                                                                Domain);

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

FRDGPassRef AddDrawClearQuadPass(FRDGBuilder& GraphBuilder,
                                 FRDGTextureRef RenderTarget,
                                 FLinearColor ClearColor)
{
    FRenderTargetParameters* Parameters =
        GraphBuilder.AllocParameters<FRenderTargetParameters>();
    Parameters->RenderTargets[0] =
        FRenderTargetBinding(RenderTarget,
                             ERenderTargetLoadAction::ENoAction,
                             0,
                             0);

    const float Width = RenderTarget->Desc.GetSize().X;
    const float Height = RenderTarget->Desc.GetSize().Y;

    return GraphBuilder.AddPass(
        RDG_EVENT_NAME("rive.ClearQuad %s", RenderTarget->Name),
        Parameters,
        ERDGPassFlags::Raster,
        [Parameters, ClearColor, Width, Height](FRDGAsyncTask,
                                                FRHICommandList& RHICmdList) {
            RHI_BREADCRUMB_EVENT(RHICmdList, "rive.ClearQuad");
            RHICmdList.SetViewport(0.0f, 0.0f, 0.0f, Width, Height, 1.0f);
            DrawClearQuad(RHICmdList, ClearColor);
        });
}
