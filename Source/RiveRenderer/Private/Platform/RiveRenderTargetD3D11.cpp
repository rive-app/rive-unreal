// Copyright Rive, Inc. All rights reserved.


#include "RiveRenderTargetD3D11.h"

#include "ID3D11DynamicRHI.h"
#include "RiveRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/d3d/pls_render_context_d3d_impl.hpp"

UE_DISABLE_OPTIMIZATION
UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::FRiveRenderTargetD3D11(
	const TSharedPtr<Private::FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget):
	FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget)
{
}

void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::Initialize()
{
	check(IsInGameThread());
	
	FScopeLock Lock(&ThreadDataCS);

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
	[RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
	{
		CacheTextureTarget_RenderThread(RHICmdList, RenderTargetResource->TextureRHI->GetTexture2D());
	});

}

DECLARE_GPU_STAT_NAMED(CacheTextureTarget, TEXT("FRiveRenderTargetD3D11::CacheTextureTarget_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InTexture)
{
	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContext == nullptr)
	{
		return;
	}

	// TODO, make sure we have correct texture DXGI_FORMAT_R8G8B8A8_UNORM
	EPixelFormat PixelFormat = InTexture->GetFormat();
	if (PixelFormat != PF_R8G8B8A8)
	{
		return;
	}

	SCOPED_GPU_STAT(RHICmdList, CacheTextureTarget);
	if (IsRHID3D11() && InTexture.IsValid())
	{
		ID3D11DynamicRHI* D3D11RHI = GetID3D11DynamicRHI();
		ID3D11Device* D3D11DevicePtr = D3D11RHI->RHIGetDevice();
		ID3D11Texture2D* D3D11ResourcePtr = (ID3D11Texture2D*)D3D11RHI->RHIGetResource(InTexture);
		D3D11_TEXTURE2D_DESC desc;
		D3D11ResourcePtr->GetDesc(&desc);
		UE_LOG(LogTemp, Warning, TEXT("D3D11ResourcePtr texture %dx%d"), desc.Width, desc.Height);


		// For now we just set one renderer and one texture
		rive::pls::PLSRenderContextD3DImpl* const PLSRenderContextD3DImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextD3DImpl>();
		CachedPLSRenderTargetD3D = PLSRenderContextD3DImpl->makeRenderTarget(desc.Width, desc.Height);
		CachedPLSRenderTargetD3D->setTargetTexture(D3D11DevicePtr, D3D11ResourcePtr);
	}
}

void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::DrawArtboard(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

	ENQUEUE_RENDER_COMMAND(DebugColorDraw)(
[this, InFit, AlignX, AlignY, InNativeArtBoard](FRHICommandListImmediate& RHICmdList)
	{
		DrawArtboard_RenderThread(RHICmdList, InFit, AlignX, AlignY, InNativeArtBoard);
	});
}

DECLARE_GPU_STAT_NAMED(DrawArtboard, TEXT("FRiveRenderTargetD3D11::DrawArtboard"));
void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard)
{
	SCOPED_GPU_STAT(RHICmdList, DrawArtboard);
	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}

	// Begin frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = GetPLSRenderer();
	if (PLSRenderer == nullptr)
	{
		return;
	}
	

	// alignArtboard
	{
			
		const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);
		const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);

		const uint32 TextureWidth = GetWidth();
		const uint32 TextureHeight = GetHeight();
	
		rive::Mat2D transform = rive::computeAlignment(
			Fit,
			Alignment,
			rive::AABB(0.0f, 0.0f, TextureWidth, TextureHeight),
			InNativeArtBoard->bounds());

		PLSRenderer->transform(transform);
	}

	// drawArtboard
	InNativeArtBoard->draw(PLSRenderer.get());

	// Flush
	PLSRenderContextPtr->flush();

	//PLSRenderContextPtr->resetGPUResources();
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::GetPLSRenderer() const
{
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return nullptr;
	}
	
	rive::pls::PLSRenderContext::FrameDescriptor frameDescriptor;
	frameDescriptor.renderTarget = CachedPLSRenderTargetD3D;
	frameDescriptor.loadAction = !bIsCleared ? rive::pls::LoadAction::clear : rive::pls::LoadAction::preserveRenderTarget;
	frameDescriptor.clearColor = 0x00000000;
	frameDescriptor.wireframe = false;
	frameDescriptor.fillsDisabled = false;
	frameDescriptor.strokesDisabled = false;

	if (bIsCleared == false)
	{
		bIsCleared = true;
	}


	PLSRenderContextPtr->beginFrame(std::move(frameDescriptor));

	return std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);
}

UE_ENABLE_OPTIMIZATION
