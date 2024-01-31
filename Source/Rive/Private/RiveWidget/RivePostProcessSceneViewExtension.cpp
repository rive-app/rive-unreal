// Copyright Rive, Inc. All rights reserved.

#include "RivePostProcessSceneViewExtension.h"

#include "Engine/TextureRenderTarget2D.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessMaterialInputs.h"

FRivePostProcessSceneViewExtension::FRivePostProcessSceneViewExtension(const FAutoRegister& AutoRegister, UTextureRenderTarget2D& WidgetRenderTarget)
	: Super(AutoRegister)
	, WidgetRenderTarget(&WidgetRenderTarget)
{
}

void FRivePostProcessSceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FRivePostProcessSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
}

void FRivePostProcessSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FRivePostProcessSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{
}

void FRivePostProcessSceneViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{
}

void FRivePostProcessSceneViewExtension::PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder,FSceneViewFamily& InViewFamily)
{
}

bool FRivePostProcessSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return WidgetRenderTarget.IsValid();
}

void FRivePostProcessSceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId,
	FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (PassId == EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FRivePostProcessSceneViewExtension::PostProcessPassAfterTonemap_RenderThread));
	}
}



/**
 * This shaders overlays WidgetTexture of SceneTexture by blending it like so
 *	color = WidgetTexture.A * WidgetTexture.RGB + (1 - WidgetTexture.A) * SceneTexture.RGB
 */
class FRiveDrawTextureInShaderPS : public FGlobalShader
{
public:

	DECLARE_GLOBAL_SHADER(FRiveDrawTextureInShaderPS);

	SHADER_USE_PARAMETER_STRUCT(FRiveDrawTextureInShaderPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WidgetTexture) 
		SHADER_PARAMETER_SAMPLER(SamplerState, WidgetSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneTexture) 
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneSampler)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
	}
};

IMPLEMENT_GLOBAL_SHADER(FRiveDrawTextureInShaderPS, "/Plugin/Rive/Private/RiveFullScreenWidgetOverlay.usf", "OverlayWidgetPS", SF_Pixel);

FScreenPassTexture FRivePostProcessSceneViewExtension::PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InSceneView, const FPostProcessMaterialInputs& InOutInputs)
{
	check(IsInRenderingThread());
	
	const FScreenPassTexture& SceneColor = InOutInputs.GetInput(EPostProcessMaterialInput::SceneColor);
	check(SceneColor.IsValid());

	FScreenPassRenderTarget Output = InOutInputs.OverrideOutput;

	// If the override output is provided, it means that this is the last pass in post processing.
	if (!Output.IsValid())
	{
		Output = FScreenPassRenderTarget::CreateFromInput(GraphBuilder, SceneColor, InSceneView.GetOverwriteLoadAction(), TEXT("RiveRenderTarget"));
	}

	// Can be invalidated after exiting PIE
	if (!WidgetRenderTarget.IsValid() || !WidgetRenderTarget->GetRenderTargetResource() || !WidgetRenderTarget->GetRenderTargetResource()->GetTexture2DRHI())
	{
		return MoveTemp(Output);
	}
	
	const FTexture2DRHIRef WidgetRenderTarget_RHI = WidgetRenderTarget->GetRenderTargetResource()->GetTexture2DRHI();
	const FRDGTextureRef WidgetRenderTarget_RDG = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(WidgetRenderTarget_RHI, TEXT("WidgetRenderTarget")));

	const FScreenPassTextureViewport InputViewport(SceneColor);
	const FScreenPassTextureViewport OutputViewport(Output);

	static constexpr float DefaultDisplayGamma = 2.2f;

	const float EngineDisplayGamma = InSceneView.Family->RenderTarget->GetDisplayGamma();
	// There is a special case where post processing and tonemapper are disabled. In this case tonemapper applies a static display Inverse of Gamma which defaults to 2.2.
	// In the case when Both PostProcessing and ToneMapper are disabled we apply gamma manually. In every other case we apply inverse gamma before applying OCIO.
	float DisplayGamma = (InSceneView.Family->EngineShowFlags.Tonemapper == 0) || (InSceneView.Family->EngineShowFlags.PostProcessing == 0) ? DefaultDisplayGamma : DefaultDisplayGamma / EngineDisplayGamma;


	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(InSceneView.GetFeatureLevel());
	TShaderMapRef<FScreenPassVS> VertexShader(ShaderMap);
	const TShaderMapRef<FRiveDrawTextureInShaderPS> PixelShader(ShaderMap);

	FRiveDrawTextureInShaderPS::FParameters* Parameters = GraphBuilder.AllocParameters<FRiveDrawTextureInShaderPS::FParameters>();
	
	Parameters->WidgetTexture = WidgetRenderTarget_RDG;
	Parameters->WidgetSampler = TStaticSamplerState<SF_Point>::GetRHI();
	Parameters->SceneTexture = SceneColor.Texture;
	Parameters->SceneSampler = TStaticSamplerState<SF_Point>::GetRHI();

	Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	AddDrawScreenPass(
		GraphBuilder,
		RDG_EVENT_NAME("RivePostProcessPassAfterTonemap_RenderThread"),
		InSceneView,
		OutputViewport,
		InputViewport,
		VertexShader,
		PixelShader,
		Parameters
		);

	return MoveTemp(Output);
}
