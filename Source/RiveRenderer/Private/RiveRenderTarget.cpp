// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTarget.h"

#include "RiveRenderer.h"
#include "Engine/Texture2DDynamic.h"
#include "Logs/RiveRendererLog.h"
#include "RenderingThread.h"
#include "TextureResource.h"

#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END

#if PLATFORM_APPLE
#include "Mac/AutoreleasePool.h"
#endif

FTimespan UE::Rive::Renderer::Private::FRiveRenderTarget::ResetTimeLimit = FTimespan(0, 0, 20);

UE::Rive::Renderer::Private::FRiveRenderTarget::FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget)
	: RiveName(InRiveName)
	, RenderTarget(InRenderTarget)
	, RiveRenderer(InRiveRenderer)
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

	// Making a copy of the RenderCommands to be processed on RenderingThread
	ENQUEUE_RENDER_COMMAND(Render)(
		[this, RiveRenderCommands = RenderCommands](FRHICommandListImmediate& RHICmdList)
		{
			Render_RenderThread(RHICmdList, RiveRenderCommands);
		});
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::SubmitAndClear()
{
	Submit();
	RenderCommands.Empty();
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Save()
{
	const FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Save);
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Restore()
{
	const FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Restore);
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Transform(float X1, float Y1, float X2, float Y2, float TX, float TY)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Transform);
	RenderCommand.X = X1;
	RenderCommand.Y = Y1;
	RenderCommand.X2 = X2;
	RenderCommand.Y2 = Y2;
	RenderCommand.TX = TX;
	RenderCommand.TY = TY;
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Translate(const FVector2f& InVector)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Translate);
	RenderCommand.TX = InVector.X;
	RenderCommand.TY = InVector.Y;
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Draw(rive::Artboard* InArtboard)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::DrawArtboard);
	RenderCommand.NativeArtboard = InArtboard;
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Align(const FBox2f& InBox, ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::AlignArtboard);
	RenderCommand.FitType = InFit;
	RenderCommand.X = InAlignment.X;
	RenderCommand.Y = InAlignment.Y;

	RenderCommand.TX = InBox.Min.X;
	RenderCommand.TY = InBox.Min.Y;
	RenderCommand.X2 = InBox.Max.X;
	RenderCommand.Y2 = InBox.Max.Y;
	
	RenderCommand.NativeArtboard = InArtboard;
	RenderCommands.Push(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Align(ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard)
{
	Align(FBox2f(FVector2f{0.f,0.f},FVector2f(GetWidth(), GetHeight())), InFit, InAlignment, InArtboard);
}

FMatrix UE::Rive::Renderer::Private::FRiveRenderTarget::GetTransformMatrix() const
{
	TArray<FMatrix> SavedMatrices;
	FMatrix CurrentMatrix = FMatrix::Identity;
	
	for (const FRiveRenderCommand& RenderCommand : RenderCommands)
	{
		switch (RenderCommand.Type)
		{
		case ERiveRenderCommandType::Save:
			SavedMatrices.Add(CurrentMatrix);
			break;
		case ERiveRenderCommandType::Restore:
			CurrentMatrix = SavedMatrices.IsEmpty() ? FMatrix::Identity : SavedMatrices.Pop();
			break;
		case ERiveRenderCommandType::AlignArtboard:
		case ERiveRenderCommandType::Transform:
		case ERiveRenderCommandType::Translate:
			CurrentMatrix = RenderCommand.GetSavedTransform() * CurrentMatrix;
			break;
		default:
			break;
		}
	}
	return CurrentMatrix;
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
	FrameDescriptor.renderTargetWidth = GetWidth();
	FrameDescriptor.renderTargetHeight = GetHeight();
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
	const rive::pls::PLSRenderContext::FlushResources FlushResources
	{
		GetRenderTarget().get()
	};
	PLSRenderContextPtr->flush(FlushResources);
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
void UE::Rive::Renderer::Private::FRiveRenderTarget::Render_RenderThread(FRHICommandListImmediate& RHICmdList, const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
	SCOPED_GPU_STAT(RHICmdList, Render);
	check(IsInRenderingThread());
	
	Render_Internal(RiveRenderCommands);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Render_Internal(const TArray<FRiveRenderCommand>& RiveRenderCommands)
{
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());

	// Sometimes Render commands can be empty (perhaps an issue with Lock contention)
	// Checking for empty here will prevent rendered "blank" frames
	if (RiveRenderCommands.IsEmpty())
	{
		return;
	}
    
#if PLATFORM_APPLE
    AutoreleasePool Pool;
#endif
    
	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = BeginFrame();
	if (PLSRenderer == nullptr)
	{
		return;
	}
	
#if PLATFORM_ANDROID
	// We need to invert the Y Axis for OpenGL, and this needs to not affect input transforms
	PLSRenderer->transform(rive::Mat2D::fromScaleAndTranslation(1.f, -1.f, 0.f, GetHeight()));
#endif
	
	RIVE_DEBUG_VERBOSE("Executing queue with %d items for '%s'", RiveRenderCommands.Num(), *RiveName.ToString());
	for (const FRiveRenderCommand& RenderCommand : RiveRenderCommands)
	{
		switch (RenderCommand.Type)
		{
		case ERiveRenderCommandType::Save:
			PLSRenderer->save();
			break;
		case ERiveRenderCommandType::Restore:
			PLSRenderer->restore();
			break;
		case ERiveRenderCommandType::DrawArtboard:
#if PLATFORM_ANDROID
			RIVE_DEBUG_VERBOSE("RenderCommand.NativeArtboard->draw()");
#endif
			RenderCommand.NativeArtboard->draw(PLSRenderer.get());
			break;
		case ERiveRenderCommandType::DrawPath:
			// TODO: Support DrawPath
			break;
		case ERiveRenderCommandType::ClipPath:
			// TODO: Support ClipPath
			break;
		case ERiveRenderCommandType::Transform:
		case ERiveRenderCommandType::AlignArtboard:
		case ERiveRenderCommandType::Translate:
			PLSRenderer->transform(RenderCommand.GetSaved2DTransform());
			break;
		}
	}

	EndFrame();
}
