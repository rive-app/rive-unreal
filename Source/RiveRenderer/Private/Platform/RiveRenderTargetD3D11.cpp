// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTargetD3D11.h"

#if PLATFORM_WINDOWS
#include "RiveRendererD3D11.h"
#include "ID3D11DynamicRHI.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderer.h"


UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::FRiveRenderTargetD3D11(const TSharedRef<FRiveRendererD3D11>& InRiveRendererD3D11, const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
	: FRiveRenderTarget(InRiveRendererD3D11, InRiveName, InRenderTarget), RiveRendererD3D11(InRiveRendererD3D11)
{
}

UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::~FRiveRenderTargetD3D11()
{
	RIVE_DEBUG_FUNCTION_INDENT
	CachedPLSRenderTargetD3D.release();
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
			CachedPLSRenderTargetD3D.release();
		}
		// For now we just set one renderer and one texture
		rive::pls::PLSRenderContextD3DImpl* const PLSRenderContextD3DImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextD3DImpl>();
		CachedPLSRenderTargetD3D = PLSRenderContextD3DImpl->makeRenderTarget(Desc.Width, Desc.Height);
		CachedPLSRenderTargetD3D->setTargetTexture(D3D11ResourcePtr);
#endif // WITH_RIVE
	}
}

rive::rcp<rive::pls::PLSRenderTarget> UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::GetRenderTarget() const
{
	return CachedPLSRenderTargetD3D;
}

#if WITH_RIVE
void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::EndFrame() const
{
	FRiveRenderTarget::EndFrame();
	ResetBlendState();
}

void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::ResetBlendState() const
{
	check(IsInRenderingThread());
	RiveRendererD3D11->ResetBlendState();
}

#endif // WITH_RIVE
UE_ENABLE_OPTIMIZATION
#endif // PLATFORM_WINDOWS
