// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererD3D11.h"

#include "RiveRenderTargetD3D11.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Windows/D3D11ThirdParty.h"
#include "ID3D11DynamicRHI.h"

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRendererD3D11::FRiveRendererD3D11()
{
}

UE::Rive::Renderer::Private::FRiveRendererD3D11::~FRiveRendererD3D11()
{
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::SetTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

	FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
	[RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
	{
		CacheTextureTarget_RenderThread(RHICmdList, RenderTargetResource->TextureRHI->GetTexture2D());
	});
	
	RenderTargets.Add(InRiveName, MakeShared<FRiveRenderTargetD3D11>());
}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRendererD3D11::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererD3D11::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource)
{
	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	// if (m_plsContext == nullptr)
	// {
	// 	return;
	// }

	SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);
	if (IsRHID3D11() && InRHIResource.IsValid())
	{
		ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
		ID3D11Device* D3D11DevicePtr = D3D11RHI->RHIGetDevice();
		ID3D11Texture2D* D3D11ResourcePtr = (ID3D11Texture2D*)D3D11RHI->RHIGetResource(InRHIResource);
		D3D11_TEXTURE2D_DESC desc;
		D3D11ResourcePtr->GetDesc(&desc);
		UE_LOG(LogTemp, Warning, TEXT("D3D11ResourcePtr texture %dx%d"), desc.Width, desc.Height);
	}
}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::CreatePLSContext()
{

}

void UE::Rive::Renderer::Private::FRiveRendererD3D11::CreatePLSRenderer()
{
	FRiveRenderer::CreatePLSRenderer();
}

UE_ENABLE_OPTIMIZATION
