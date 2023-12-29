// Copyright Rive, Inc. All rights reserved.


#include "RiveRenderTarget.h"

#include "RiveRenderer.h"
#include "Engine/TextureRenderTarget2D.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/pls_render_context.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION
UE::Rive::Renderer::Private::FRiveRenderTarget::FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
	: RiveRenderer(InRiveRenderer)
	, RiveName(InRiveName)
	, RenderTarget(InRenderTarget)
{
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::AlignArtboard(uint8 InFit, float AlignX, float AlignY,
	rive::Artboard* InNativeArtBoard)
{
	check(IsInGameThread());

	FScopeLock Lock(&ThreadDataCS);

#if WITH_RIVE

	// rive::pls::PLSRenderer* PLSRenderer = GetPLSRenderer();
	// if (PLSRenderer == nullptr)
	// {
	// 	return;
	// }
	//
	// const rive::Fit& Fit = *reinterpret_cast<rive::Fit*>(&InFit);
	// const rive::Alignment& Alignment = rive::Alignment(AlignX, AlignY);
	//
	// const uint32 TextureWidth = GetWidth();
	// const uint32 TextureHeight = GetHeight();
	//
	// rive::Mat2D transform = rive::computeAlignment(
	// 	Fit,
	// 	Alignment,
	// 	rive::AABB(0.0f, 0.0f, TextureWidth, TextureHeight),
	// 	InNativeArtBoard->bounds());
	//
	// PLSRenderer->transform(transform);

#endif // WITH_RIVE
}

uint32 UE::Rive::Renderer::Private::FRiveRenderTarget::GetWidth() const
{
	return RenderTarget->SizeX;
}

uint32 UE::Rive::Renderer::Private::FRiveRenderTarget::GetHeight() const
{
	return RenderTarget->SizeY;
}

UE_ENABLE_OPTIMIZATION
