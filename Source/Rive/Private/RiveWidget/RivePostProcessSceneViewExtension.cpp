// Copyright Epic Games, Inc. All Rights Reserved.

#include "RivePostProcessSceneViewExtension.h"

#include "Engine/TextureRenderTarget2D.h"
#include "ScreenPass.h"
#include "TextureResource.h"


FRivePostProcessSceneViewExtension::FRivePostProcessSceneViewExtension(const FAutoRegister& AutoRegister, UTextureRenderTarget2D& WidgetRenderTarget)
	: Super(AutoRegister)
	, WidgetRenderTarget(&WidgetRenderTarget)
{}

void FRivePostProcessSceneViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{}

void FRivePostProcessSceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{}

void FRivePostProcessSceneViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{}

void FRivePostProcessSceneViewExtension::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView)
{}

void FRivePostProcessSceneViewExtension::PreRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily)
{}

void FRivePostProcessSceneViewExtension::PostRenderViewFamily_RenderThread(FRDGBuilder& GraphBuilder,FSceneViewFamily& InViewFamily)
{
	if (!WidgetRenderTarget.IsValid())
	{
		return;
	}

	FRDGTextureRef ViewFamilyTexture = TryCreateViewFamilyTexture(GraphBuilder, InViewFamily);
	if (!ViewFamilyTexture)
	{
		return;
	}

	for (int32 ViewIndex = 0; ViewIndex < InViewFamily.Views.Num(); ++ViewIndex)
	{
		RenderMaterial_RenderThread(GraphBuilder, *InViewFamily.Views[ViewIndex], ViewFamilyTexture);
	}
}

bool FRivePostProcessSceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return WidgetRenderTarget.IsValid();
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
	{}
};
IMPLEMENT_GLOBAL_SHADER(FRiveDrawTextureInShaderPS, "/Plugin/Rive/Private/RiveFullScreenWidgetOverlay.usf", "OverlayWidgetPS", SF_Pixel);

void FRivePostProcessSceneViewExtension::RenderMaterial_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, FRDGTextureRef ViewFamilyTexture)
{
	// Can be invalidated after exiting PIE
	if (!WidgetRenderTarget.IsValid() || !WidgetRenderTarget->GetRenderTargetResource() || !WidgetRenderTarget->GetRenderTargetResource()->GetTexture2DRHI())
	{
		return;
	}
	
	const FTexture2DRHIRef WidgetRenderTarget_RHI = WidgetRenderTarget->GetRenderTargetResource()->GetTexture2DRHI();
	const FRDGTextureRef WidgetRenderTarget_RDG = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(WidgetRenderTarget_RHI, TEXT("WidgetRenderTarget")));

	const FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FScreenPassVS> VertexShader(GlobalShaderMap);
	const TShaderMapRef<FRiveDrawTextureInShaderPS> PixelShader(GlobalShaderMap);
	
	FRiveDrawTextureInShaderPS::FParameters* Parameters = GraphBuilder.AllocParameters<FRiveDrawTextureInShaderPS::FParameters>();
	Parameters->WidgetTexture = WidgetRenderTarget_RDG;
	Parameters->WidgetSampler = TStaticSamplerState<SF_Point>::GetRHI();
	Parameters->SceneTexture = ViewFamilyTexture;
	Parameters->SceneSampler = TStaticSamplerState<SF_Point>::GetRHI();
	Parameters->RenderTargets[0] = FRenderTargetBinding{ ViewFamilyTexture, ERenderTargetLoadAction::ELoad };

	const FScreenPassTextureViewport InputViewport(WidgetRenderTarget_RDG);
	const FScreenPassTextureViewport OutputViewport(ViewFamilyTexture);
	// Note that we reference WidgetRenderTarget here, which means RDG will synchronize access to it.
	// That means that the DrawWindow operation (see FVPFullScreenUserWidget_PostProcessBase) will finish writing into WidgetRenderTarget before we access it with this pass.
	AddDrawScreenPass(
		GraphBuilder,
		RDG_EVENT_NAME("RiveScreenPostProcessOverlay"),
		InView,
		OutputViewport,
		InputViewport,
		VertexShader,
		PixelShader,
		Parameters
		);
}
