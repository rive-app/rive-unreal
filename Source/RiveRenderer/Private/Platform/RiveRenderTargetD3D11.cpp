// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTargetD3D11.h"

#if PLATFORM_WINDOWS
#include "CommonRenderResources.h"
#include "RenderGraphBuilder.h"
#include "ScreenRendering.h"
#include "Engine/Texture2DDynamic.h"

#include "RiveRendererD3D11.h"
#include "ID3D11DynamicRHI.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderer.h"


UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::FRiveRenderTargetD3D11(const TSharedRef<FRiveRendererD3D11>& InRiveRendererD3D11, const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
	: FRiveRenderTarget(InRiveRendererD3D11, InRiveName, InRenderTarget), RiveRendererD3D11(InRiveRendererD3D11)
{
}

UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::~FRiveRenderTargetD3D11()
{
	RIVE_DEBUG_FUNCTION_INDENT
	CachedPLSRenderTargetD3D.reset();
}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRenderTargetD3D11::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTexture)
{
	check(IsInRenderingThread());
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

#if WITH_RIVE
	rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();
	if (PLSRenderContext == nullptr)
	{
		return;
	}
#endif // WITH_RIVE
	
	EPixelFormat PixelFormat = InTexture->GetFormat();

	if (PixelFormat != PF_R8G8B8A8)
	{
		return;
	}

	SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);
	if (IsRHID3D11() && InTexture.IsValid())
	{
		ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
		ID3D11Texture2D* D3D11ResourcePtr = (ID3D11Texture2D*)D3D11RHI->RHIGetResource(InTexture);
		D3D11_TEXTURE2D_DESC Desc;
		D3D11ResourcePtr->GetDesc(&Desc);
		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11ResourcePtr texture %dx%d"), Desc.Width, Desc.Height);

#if WITH_RIVE
		if (CachedPLSRenderTargetD3D)
		{
			CachedPLSRenderTargetD3D.reset();
		}
		// For now we just set one renderer and one texture
		rive::pls::PLSRenderContextD3DImpl* const PLSRenderContextD3DImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextD3DImpl>();
		CachedPLSRenderTargetD3D = PLSRenderContextD3DImpl->makeRenderTarget(Desc.Width, Desc.Height);
		CachedPLSRenderTargetD3D->setTargetTexture(D3D11ResourcePtr);
#endif // WITH_RIVE
	}
}

void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
	// ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
	// ID3D11Device* D3D11DevicePtr = D3D11RHI->RHIGetDevice();
	// ID3D11DeviceContext* D3D11DeviceContext = nullptr;
	// D3D11DevicePtr->GetImmediateContext(&D3D11DeviceContext);

	// FRDGBuilder GraphBuilder(RHICmdList);

	// -- First, we start a render pass todo: might not be needed
	FTextureRHIRef TargetTexture = RenderTarget->GetResource()->TextureRHI;
	RHICmdList.Transition(FRHITransitionInfo(TargetTexture, ERHIAccess::Unknown, ERHIAccess::RTV));
	FRHIRenderPassInfo RPInfo(TargetTexture, ERenderTargetActions::Load_Store);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("RiveRenderingTexture"));
	// {
	// 	FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	// 	TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
	// 	TShaderMapRef<FScreenPS> PixelShader(ShaderMap);
	// 		
	// 	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	// 	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	// 	GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
	// 	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	// 	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false>::GetRHI();
	// 	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	// 	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	// 	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	// 	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	// 	
	// 	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
	// 	SetShaderParametersLegacyPS(RHICmdList, PixelShader, TStaticSamplerState<SF_Point>::GetRHI(), TargetTexture);
	// }
	// RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);

	
	// -- Then we render Rive, ensuring the DX11 states are reset before the call
	RHICmdList.EnqueueLambda([this, RiveRenderCommands](FRHICommandListImmediate& RHICmdList)
	{
		RiveRendererD3D11->ResetDXStateForRive();
		FRiveRenderTarget::Render_Internal(RiveRenderCommands, &RHICmdList);
		RiveRendererD3D11->ResetDXState();
	});
	// RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);

	
	// ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
	// if (IRHICommandContextPSOFallback* PsoFallback = (IRHICommandContextPSOFallback*)D3D11RHI)
	// {
	// 	PsoFallback->RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	// 	PsoFallback->RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI(), FLinearColor(1.0f, 1.0f, 1.0f));
	// 	PsoFallback->RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI(), 0);
	// }

	// -- Lastly, now that we have manually reset DX11, we need to update the state to ensure it matches the direct calls we just made
	{
		FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
		TShaderMapRef<FScreenVS> VertexShader(ShaderMap); // just a default todo: might need to be forced set
		TShaderMapRef<FScreenPS> PixelShader(ShaderMap); // just a default todo: might need to be forced set
		
		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		// same as default D3D11_BLEND_DESC which is same as passing null to OMSetBlendState
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI(); 
		// NOT the same as default D3D11_RASTERIZER_DESC, a matching Rasterizer has been created in FRiveRendererD3D11GPUAdapter::InitBlendState
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		// NOT the same as default D3D11_DEPTH_STENCIL_DESC, a matching DepthStencil has been created in FRiveRendererD3D11GPUAdapter::InitBlendState
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI; // just a default todo: might need to be forced set
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList; // just a default todo: might need to be forced set
		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0, EApplyRendertargetOption::CheckApply, false);
	}
	RHICmdList.EndRenderPass();
	RHICmdList.Transition(FRHITransitionInfo(TargetTexture, ERHIAccess::RTV, ERHIAccess::SRVMask));
	// RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
	
	// GraphBuilder.Execute();
}

rive::rcp<rive::pls::PLSRenderTarget> UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::GetRenderTarget() const
{
	return CachedPLSRenderTargetD3D;
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::BeginFrame(FRHICommandListImmediate* RHICmdListPtr)
{
	check(RHICmdListPtr);
	check(RenderTarget->GetResource()->TextureRHI);
	
	return FRiveRenderTarget::BeginFrame(RHICmdListPtr);
}

#if WITH_RIVE
void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::EndFrame(FRHICommandListImmediate* RHICmdListPtr) const
{
	FRiveRenderTarget::EndFrame(RHICmdListPtr);
}

#endif // WITH_RIVE
#endif // PLATFORM_WINDOWS
