// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTarget.h"

#include "RiveRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"

FTimespan UE::Rive::Renderer::Private::FRiveRenderTarget::ResetTimeLimit = FTimespan(0, 0, 20);

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRenderTarget::FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
	: RiveRenderer(InRiveRenderer)
	, RiveName(InRiveName)
	, RenderTarget(InRenderTarget)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s   FRiveRenderTarget::FRiveRenderTarget"), FDebugLogger::Ind());
}

UE::Rive::Renderer::Private::FRiveRenderTarget::~FRiveRenderTarget()
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s   FRiveRenderTarget::~FRiveRenderTargetL"), FDebugLogger::Ind());
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
