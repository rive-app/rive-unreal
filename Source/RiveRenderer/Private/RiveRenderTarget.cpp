// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTarget.h"

#include "RiveRenderer.h"
#include "Engine/Texture2DDynamic.h"
#include "Logs/RiveRendererLog.h"

FTimespan UE::Rive::Renderer::Private::FRiveRenderTarget::ResetTimeLimit = FTimespan(0, 0, 20);

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRenderTarget::FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
	: RiveRenderer(InRiveRenderer)
	, RiveName(InRiveName)
	, RenderTarget(InRenderTarget)
{
	RIVE_DEBUG_FUNCTION_INDENT;
}

UE::Rive::Renderer::Private::FRiveRenderTarget::~FRiveRenderTarget()
{
	RIVE_DEBUG_FUNCTION_INDENT;
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
