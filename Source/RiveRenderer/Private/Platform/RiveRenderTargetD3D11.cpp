// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTargetD3D11.h"

#include "RenderGraphBuilder.h"

#if PLATFORM_WINDOWS

#include "Engine/TextureRenderTarget2D.h"
#include "ID3D11DynamicRHI.h"
#include "Logs/RiveRendererLog.h"
#include "RiveRenderer.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/d3d/pls_render_context_d3d_impl.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::FRiveRenderTargetD3D11(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
	: FRiveRenderTarget(InRiveRenderer, InRiveName, InRenderTarget)
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

#if WITH_RIVE

	rive::pls::PLSRenderContext* PLSRenderContext = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContext == nullptr)
	{
		return;
	}

#endif // WITH_RIVE

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

		ID3D11Texture2D* D3D11ResourcePtr = (ID3D11Texture2D*)D3D11RHI->RHIGetResource(InTexture);

		D3D11_TEXTURE2D_DESC Desc;

		D3D11ResourcePtr->GetDesc(&Desc);

		UE_LOG(LogRiveRenderer, Warning, TEXT("D3D11ResourcePtr texture %dx%d"), Desc.Width, Desc.Height);

#if WITH_RIVE

		// For now we just set one renderer and one texture
		rive::pls::PLSRenderContextD3DImpl* const PLSRenderContextD3DImpl = PLSRenderContext->static_impl_cast<rive::pls::PLSRenderContextD3DImpl>();
		
		CachedPLSRenderTargetD3D = PLSRenderContextD3DImpl->makeRenderTarget(Desc.Width, Desc.Height);
		
		CachedPLSRenderTargetD3D->setTargetTexture(D3D11ResourcePtr);

#endif // WITH_RIVE
	}
}

#if WITH_RIVE

void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::DrawArtboard(uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

	ENQUEUE_RENDER_COMMAND(AlignArtboard)(
		[this, InFit, AlignX, AlignY, InNativeArtboard, DebugColor](FRHICommandListImmediate& RHICmdList)
		{
			DrawArtboard_RenderThread(RHICmdList, InFit, AlignX, AlignY, InNativeArtboard, DebugColor);
		});
}

DECLARE_GPU_STAT_NAMED(DrawArtboard, TEXT("FRiveRenderTargetD3D11::DrawArtboard"));
void UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::DrawArtboard_RenderThread(FRHICommandListImmediate& RHICmdList, uint8 InFit, float AlignX, float AlignY, rive::Artboard* InNativeArtboard, const FLinearColor DebugColor)
{
	FRDGBuilder GraphBuilder(RHICmdList);
	
	SCOPED_GPU_STAT(RHICmdList, DrawArtboard);

	check(IsInRenderingThread());

	FScopeLock Lock(&ThreadDataCS);

	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}

	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = GetPLSRenderer(DebugColor);

	if (PLSRenderer == nullptr)
	{
		return;
	}

	// Align Artboard
	const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);

	const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);

	const uint32 TextureWidth = GetWidth();

	const uint32 TextureHeight = GetHeight();

	rive::Mat2D Transform = rive::computeAlignment(
		Fit,
		Alignment,
		rive::AABB(0.f, 0.f, TextureWidth, TextureHeight),
		InNativeArtboard->bounds());

	PLSRenderer->transform(Transform);

	// Draw Artboard
	InNativeArtboard->draw(PLSRenderer.get());

	{ // End drawing a frame.
		// Flush
		PLSRenderContextPtr->flush();

		const FDateTime Now = FDateTime::Now();

		const int32 TimeElapsed = (Now - LastResetTime).GetSeconds();

		if (TimeElapsed >= ResetTimeLimit.GetSeconds())
		{
			// Reset
			PLSRenderContextPtr->shrinkGPUResourcesToFit();

			PLSRenderContextPtr->resetGPUResources();

			LastResetTime = Now;
		}
	}

	GraphBuilder.Execute();
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTargetD3D11::GetPLSRenderer(const FLinearColor DebugColor) const
{
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();

	if (PLSRenderContextPtr == nullptr)
	{
		return nullptr;
	}

	FColor ClearColor = DebugColor.ToRGBE();
	
	rive::pls::PLSRenderContext::FrameDescriptor FrameDescriptor;

	FrameDescriptor.renderTarget = CachedPLSRenderTargetD3D;
	
	FrameDescriptor.loadAction = bIsCleared ? rive::pls::LoadAction::clear : rive::pls::LoadAction::preserveRenderTarget;
	
	FrameDescriptor.clearColor = rive::colorARGB(ClearColor.A, ClearColor.R,ClearColor.G,ClearColor.B);
		
	FrameDescriptor.wireframe = false;
	
	FrameDescriptor.fillsDisabled = false;
	
	FrameDescriptor.strokesDisabled = false;

	if (bIsCleared == false)
	{
		bIsCleared = true;
	}

	PLSRenderContextPtr->beginFrame(std::move(FrameDescriptor));

	return std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);
}

#endif // WITH_RIVE

UE_ENABLE_OPTIMIZATION

#endif // PLATFORM_WINDOWS
