// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTarget.h"

#include "RenderGraphBuilder.h"
#include "RiveRenderCommand.h"
#include "RiveRenderer.h"
#include "Engine/Texture2DDynamic.h"
#include "Logs/RiveRendererLog.h"

THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END

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

void UE::Rive::Renderer::Private::FRiveRenderTarget::Initialize()
{
	check(IsInGameThread());

	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	FTextureResource* RenderTargetResource = RenderTarget->GetResource();
	ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)(
		[RenderTargetResource, this](FRHICommandListImmediate& RHICmdList)
		{
			CacheTextureTarget_RenderThread(RHICmdList, RenderTargetResource->TextureRHI->GetTexture2D());
		});
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Submit()
{
	check(IsInGameThread());

	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	ENQUEUE_RENDER_COMMAND(Render)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			Render_RenderThread(RHICmdList);
		});
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::SubmitAndClear()
{
	{
		FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
		bClearQueue = true;
	}
	
	Submit();
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Save()
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	const FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Save);
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Restore()
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	const FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Restore);
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Transform(float X1, float Y1, float X2, float Y2, float TX,
	float TY)
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Transform);
	RenderCommand.X = X1;
	RenderCommand.Y = Y1;
	RenderCommand.X2 = X2;
	RenderCommand.Y2 = Y2;
	RenderCommand.TX = TX;
	RenderCommand.TY = TY;
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Draw(rive::Artboard* InArtboard)
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::DrawArtboard);
	RenderCommand.NativeArtboard = InArtboard;
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Align(ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard)
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::AlignArtboard);
	RenderCommand.FitType = InFit;
	RenderCommand.X = InAlignment.X;
	RenderCommand.Y = InAlignment.Y;
	RenderCommand.NativeArtboard = InArtboard;
	RenderCommands.Push(RenderCommand);
}

std::unique_ptr<rive::pls::PLSRenderer> UE::Rive::Renderer::Private::FRiveRenderTarget::BeginFrame()
{
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();
	if (PLSRenderContextPtr == nullptr)
	{
		return nullptr;
	}

	FColor Color = ClearColor.ToRGBE();
	rive::pls::PLSRenderContext::FrameDescriptor FrameDescriptor;
	FrameDescriptor.renderTarget = GetRenderTarget(); // get the platform specific render target
	FrameDescriptor.loadAction = bIsCleared ? rive::pls::LoadAction::clear : rive::pls::LoadAction::preserveRenderTarget;
	FrameDescriptor.clearColor = rive::colorARGB(Color.A, Color.R, Color.G, Color.B);
	FrameDescriptor.wireframe = false;
	FrameDescriptor.fillsDisabled = false;
	FrameDescriptor.strokesDisabled = false;

	if (bIsCleared == false)
	{
		bIsCleared = true;
	}

#if PLATFORM_ANDROID
	RIVE_DEBUG_VERBOSE("FRiveRenderTargetOpenGL PLSRenderContextPtr->beginFrame %p", PLSRenderContextPtr);
#endif
	PLSRenderContextPtr->beginFrame(std::move(FrameDescriptor));
	return std::make_unique<rive::pls::PLSRenderer>(PLSRenderContextPtr);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::EndFrame() const
{
	rive::pls::PLSRenderContext* PLSRenderContextPtr = RiveRenderer->GetPLSRenderContextPtr();
	if (PLSRenderContextPtr == nullptr)
	{
		return;
	}

	// End drawing a frame.
	// Flush
#if PLATFORM_ANDROID
	RIVE_DEBUG_VERBOSE("PLSRenderContextPtr->flush %p", PLSRenderContextPtr);
#endif
	PLSRenderContextPtr->flush();

	const FDateTime Now = FDateTime::Now();
	const int32 TimeElapsed = (Now - LastResetTime).GetSeconds();
	if (TimeElapsed >= ResetTimeLimit.GetSeconds())
	{
		// Reset
		// PLSRenderContextPtr->shrinkGPUResourcesToFit();
		// PLSRenderContextPtr->resetGPUResources();
		LastResetTime = Now;
	}
}

uint32 UE::Rive::Renderer::Private::FRiveRenderTarget::GetWidth() const
{
	return RenderTarget->SizeX;
}

uint32 UE::Rive::Renderer::Private::FRiveRenderTarget::GetHeight() const
{
	return RenderTarget->SizeY;
}

DECLARE_GPU_STAT_NAMED(Render, TEXT("RiveRenderTarget::Render"));
void UE::Rive::Renderer::Private::FRiveRenderTarget::Render_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	// FRDGBuilder GraphBuilder(RHICmdList); // this was in DX11
	SCOPED_GPU_STAT(RHICmdList, Render);
	check(IsInRenderingThread());

	Render_Internal();
	// GraphBuilder.Execute(); // this was in DX11
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Render_Internal()
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = BeginFrame();
	if (PLSRenderer == nullptr)
	{
		return;
	}
	
	for (const FRiveRenderCommand& RenderCommand : RenderCommands)
	{
		switch (RenderCommand.Type)
		{
		case ERiveRenderCommandType::Save:
			PLSRenderer->save();
			break;
		case ERiveRenderCommandType::Restore:
			PLSRenderer->restore();
			break;
		case ERiveRenderCommandType::Transform:
			PLSRenderer->transform(
				rive::Mat2D(
					RenderCommand.X,
					RenderCommand.Y,
					RenderCommand.X2,
					RenderCommand.Y2,
					RenderCommand.TX,
					RenderCommand.TY));
			break;
		case ERiveRenderCommandType::DrawArtboard:
#if PLATFORM_ANDROID
			RIVE_DEBUG_VERBOSE("RenderCommand.NativeArtboard->draw()");
#endif
			RenderCommand.NativeArtboard->draw(PLSRenderer.get());
			break;
		case ERiveRenderCommandType::DrawPath:
			break;
		case ERiveRenderCommandType::ClipPath:
			break;
		case ERiveRenderCommandType::AlignArtboard:
			rive::Mat2D Transform = rive::computeAlignment(
				static_cast<rive::Fit>(RenderCommand.FitType),
				rive::Alignment(RenderCommand.X, RenderCommand.Y),
				rive::AABB(0.f, 0.f, GetWidth(), GetHeight()),
				RenderCommand.NativeArtboard->bounds());

			PLSRenderer->transform(Transform);
			break;
		}
	}

	EndFrame();
	
	if (bClearQueue)
	{
		bClearQueue = false;
		RenderCommands.Empty();
	}
}

UE_ENABLE_OPTIMIZATION
