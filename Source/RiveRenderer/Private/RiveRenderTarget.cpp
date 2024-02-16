// Copyright Rive, Inc. All rights reserved.

#include "RiveRenderTarget.h"

#include "RiveRenderCommand.h"
#include "RiveRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Logs/RiveRendererLog.h"
#include "rive/artboard.hpp"
#include "rive/pls/pls_renderer.hpp"

FTimespan UE::Rive::Renderer::Private::FRiveRenderTarget::ResetTimeLimit = FTimespan(0, 0, 20);

UE_DISABLE_OPTIMIZATION

UE::Rive::Renderer::Private::FRiveRenderTarget::FRiveRenderTarget(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
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

void UE::Rive::Renderer::Private::FRiveRenderTarget::Save()
{
	const FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Save);
	RenderCommands.Enqueue(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Restore()
{
	const FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Restore);
	RenderCommands.Enqueue(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Transform(float X1, float Y1, float X2, float Y2, float TX,
	float TY)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::Transform);
	RenderCommand.X = X1;
	RenderCommand.Y = Y1;
	RenderCommand.X2 = X2;
	RenderCommand.Y2 = Y2;
	RenderCommand.TX = TX;
	RenderCommand.TY = TY;
	RenderCommands.Enqueue(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Draw(rive::Artboard* InArtboard)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::DrawArtboard);
	RenderCommand.NativeArtboard = InArtboard;
	RenderCommands.Enqueue(RenderCommand);
}

void UE::Rive::Renderer::Private::FRiveRenderTarget::Align(ERiveFitType InFit, const FVector2f& InAlignment, rive::Artboard* InArtboard)
{
	FRiveRenderCommand RenderCommand(ERiveRenderCommandType::AlignArtboard);
	RenderCommand.FitType = InFit;
	RenderCommand.X = InAlignment.X;
	RenderCommand.Y = InAlignment.Y;
	RenderCommand.NativeArtboard = InArtboard;
	RenderCommands.Enqueue(RenderCommand);
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
	FScopeLock Lock(&RiveRenderer->GetThreadDataCS());
	
	// Begin Frame
	std::unique_ptr<rive::pls::PLSRenderer> PLSRenderer = BeginFrame();
	if (PLSRenderer == nullptr)
	{
		return;
	}

	FRiveRenderCommand RenderCommand;
	while (RenderCommands.Dequeue(RenderCommand))
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
	// GraphBuilder.Execute(); // this was in DX11
}

UE_ENABLE_OPTIMIZATION
