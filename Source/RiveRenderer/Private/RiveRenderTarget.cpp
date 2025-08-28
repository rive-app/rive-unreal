// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveRenderTarget.h"

#include "RenderGraphBuilder.h"
#include "RiveRenderer.h"
#include "Engine/Texture2DDynamic.h"
#include "Logs/RiveRendererLog.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTarget2D.h"
#include "rive/command_server.hpp"

THIRD_PARTY_INCLUDES_START
#undef PI
#include "rive/artboard.hpp"
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/render_target.hpp"
THIRD_PARTY_INCLUDES_END

FRiveRenderTarget::FRiveRenderTarget(FRiveRenderer* Renderer,
                                     const FName& InRiveName,
                                     UTexture2DDynamic* InRenderTarget) :
    RiveName(InRiveName), RiveRenderer(Renderer), RenderTarget(InRenderTarget)

{
    check(IsInGameThread());
    RIVE_DEBUG_FUNCTION_INDENT;
}

FRiveRenderTarget::FRiveRenderTarget(FRiveRenderer* Renderer,
                                     const FName& InRiveName,
                                     UTextureRenderTarget2D* InRenderTarget) :
    RiveName(InRiveName),
    RiveRenderer(Renderer),
    RenderToTextureTarget(InRenderTarget)
{
    check(IsInGameThread());
}

FRiveRenderTarget::FRiveRenderTarget(FRiveRenderer* Renderer,
                                     const FName& InRiveName,
                                     FRenderTarget* InRenderTarget) :
    RiveName(InRiveName),
    RiveRenderer(Renderer),
    ThumbnailRenderTarget(InRenderTarget)
{
    check(IsInGameThread());
    RIVE_DEBUG_FUNCTION_INDENT;
}

FRiveRenderTarget::FRiveRenderTarget(FRiveRenderer* Renderer,
                                     const FName& InRiveName,
                                     FRDGTextureRef InRenderTarget) :
    RiveName(InRiveName),
    RiveRenderer(Renderer),
    RenderTargetRDG(InRenderTarget)

{
    check(IsInRenderingThread());
    RIVE_DEBUG_FUNCTION_INDENT;
}

FRiveRenderTarget::~FRiveRenderTarget() { RIVE_DEBUG_FUNCTION_INDENT; }

void FRiveRenderTarget::Initialize()
{
    check(IsInGameThread());
    check(RenderTarget || RenderToTextureTarget);
    FTextureResource* RenderTargetResource =
        RenderTarget ? RenderTarget->GetResource()
                     : RenderToTextureTarget->GetResource();
    check(RenderTargetResource);
    ENQUEUE_RENDER_COMMAND(CacheTextureTarget_RenderThread)
    ([RenderTargetResource, this](FRHICommandListImmediate& RHICmdList) {
        RHIResource = RenderTargetResource->TextureRHI;
        CacheTextureTarget_RenderThread(RHICmdList,
                                        RenderTargetResource->TextureRHI);
    });
}

void FRiveRenderTarget::UpdateTargetTexture(UTexture2DDynamic* InRenderTarget)
{
    RenderTargetRDG = nullptr;
    ThumbnailRenderTarget = nullptr;
    RenderTarget = InRenderTarget;
    Initialize();
}

void FRiveRenderTarget::UpdateTargetTexture(FRDGTextureRef InRenderTarget)
{
    check(IsInRenderingThread());
    RenderTarget = nullptr;
    ThumbnailRenderTarget = nullptr;
    RenderTargetRDG = InRenderTarget;
}

void FRiveRenderTarget::UpdateTargetTexture(FRenderTarget* InRenderTarget)
{
    RenderTarget = nullptr;
    RenderTargetRDG = nullptr;
    ThumbnailRenderTarget = InRenderTarget;
}

void FRiveRenderTarget::UpdateRHIResorourceDirect(FTextureRHIRef InRenderTarget)
{
    check(IsInRenderingThread());
    RenderTarget = nullptr;
    RenderTargetRDG = nullptr;
    ThumbnailRenderTarget = nullptr;
    RHIResource = InRenderTarget;
}

TUniquePtr<rive::Renderer> FRiveRenderTarget::BeginRenderFrame(
    rive::gpu::RenderContext* RenderContextPtr)
{
    check(IsInRenderingThread());
    if (RHIResource)
    {
        FLinearColor Color = RHIResource->GetClearColor();
        rive::gpu::RenderContext::FrameDescriptor FrameDescriptor;
        FrameDescriptor.renderTargetWidth = RHIResource->GetSizeX();
        FrameDescriptor.renderTargetHeight = RHIResource->GetSizeY();
        FrameDescriptor.loadAction =
            bClearRenderTarget ? rive::gpu::LoadAction::clear
                               : rive::gpu::LoadAction::preserveRenderTarget;
        FrameDescriptor.clearColor =
            rive::colorARGB(Color.A, Color.R, Color.G, Color.B);
        FrameDescriptor.wireframe = false;
        FrameDescriptor.fillsDisabled = false;
        FrameDescriptor.strokesDisabled = false;

        RenderContextPtr->beginFrame(std::move(FrameDescriptor));
    }
    else if (ThumbnailRenderTarget)
    {
        // Assume black because FRenderTarget does not have a way to query clear
        // color
        FLinearColor Color = FLinearColor::Black;
        rive::gpu::RenderContext::FrameDescriptor FrameDescriptor;
        FrameDescriptor.renderTargetWidth =
            ThumbnailRenderTarget->GetSizeXY().X;
        FrameDescriptor.renderTargetHeight =
            ThumbnailRenderTarget->GetSizeXY().Y;
        FrameDescriptor.loadAction =
            bClearRenderTarget ? rive::gpu::LoadAction::clear
                               : rive::gpu::LoadAction::preserveRenderTarget;
        FrameDescriptor.clearColor =
            rive::colorARGB(Color.A, Color.R, Color.G, Color.B);
        FrameDescriptor.wireframe = false;
        FrameDescriptor.fillsDisabled = false;
        FrameDescriptor.strokesDisabled = false;

        RenderContextPtr->beginFrame(std::move(FrameDescriptor));
    }
    else if (RenderTargetRDG)
    {
        // Assume black because FRenderTarget does not have a way to query clear
        // color
        FLinearColor Color = RenderTargetRDG->Desc.ClearValue.GetClearColor();
        rive::gpu::RenderContext::FrameDescriptor FrameDescriptor;
        FrameDescriptor.renderTargetWidth = RenderTargetRDG->Desc.GetSize().X;
        FrameDescriptor.renderTargetHeight = RenderTargetRDG->Desc.GetSize().Y;
        FrameDescriptor.loadAction =
            bClearRenderTarget ? rive::gpu::LoadAction::clear
                               : rive::gpu::LoadAction::preserveRenderTarget;
        FrameDescriptor.clearColor =
            rive::colorARGB(Color.A, Color.R, Color.G, Color.B);
        FrameDescriptor.wireframe = false;
        FrameDescriptor.fillsDisabled = false;
        FrameDescriptor.strokesDisabled = false;

        RenderContextPtr->beginFrame(std::move(FrameDescriptor));
    }
    else
    {
        RIVE_UNREACHABLE();
    }
    return MakeUnique<rive::RiveRenderer>(RenderContextPtr);
}

void FRiveRenderTarget::EndRenderFrame(
    rive::gpu::RenderContext* RenderContextPtr)
{
    check(IsInRenderingThread());
    auto& RHICmdList = GRHICommandList.GetImmediateCommandList();
    FRDGBuilder GraphBuilder(RHICmdList);
    RenderContextPtr->flush({GetRenderTarget().get(), &GraphBuilder});
    GraphBuilder.Execute();
}

uint32 FRiveRenderTarget::GetWidth() const { return RenderTarget->SizeX; }

uint32 FRiveRenderTarget::GetHeight() const { return RenderTarget->SizeY; }
