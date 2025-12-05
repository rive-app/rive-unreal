// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "Slate/SRiveLeafWidget.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#endif
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "IRiveRendererModule.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RiveRenderer.h"
#include "RiveRenderTarget.h"
#include "RiveTypeConversions.h"
#include "TimerManager.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Logs/RiveLog.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"

#include "Stats/RiveStats.h"

#include "Rendering/DrawElements.h"
#include "rive/command_server.hpp"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveDescriptor.h"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/rive_renderer.hpp"
#include "Streaming/TextureMipDataProvider.h"

DECLARE_GPU_STAT_NAMED(FRiveRendererDrawElementDraw,
                       TEXT("FRiveRendererDrawElement::Draw_RenderThread"));

FORCEINLINE rive::AABB AABBForSlateRect(const FSlateRect& Rect)
{
    return rive::AABB(Rect.Left, Rect.Top, Rect.Right, Rect.Bottom);
}

class FRiveRendererDrawElement : public ICustomSlateElement
{
public:
    FRiveRendererDrawElement()
    {
        RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        if (!ensure(RiveRenderer))
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("FRiveRendererDrawElement() "
                        "RiveRenderer is Null"));
            return;
        }

        CommandServer = RiveRenderer->GetCommandServerUnsafe();
        if (!ensure(CommandServer))
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("FRiveRendererDrawElement() "
                        "CommandServer is Null"));
            return;
        }
    }
    FRiveRendererDrawElement(TWeakObjectPtr<URiveArtboard> RiveArtboard) :
        RiveArtboard(MoveTemp(RiveArtboard))
    {
        RiveRenderer = IRiveRendererModule::Get().GetRenderer();
        if (!ensure(RiveRenderer))
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("FRiveRendererDrawElement() "
                        "RiveRenderer is Null"));
            return;
        }

        CommandServer = RiveRenderer->GetCommandServerUnsafe();
        if (!ensure(CommandServer))
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("FRiveRendererDrawElement() "
                        "CommandServer is Null"));
            return;
        }
    }
    /*
     * We have to bypass the command queue here because it's already on the
     * render thread. This will be thread safe but we should find a way to not
     * have to bypass it
     */
    virtual void Draw_RenderThread(FRDGBuilder& GraphBuilder,
                                   const FDrawPassInputs& Inputs) override
    {
        check(RiveRenderer);
        check(CommandServer);

        if (Inputs.OutputTexture == nullptr)
            return;

        SCOPED_NAMED_EVENT_TEXT(
            TEXT("FRiveRendererDrawElement::Draw_RenderThread"),
            FColor::White);

        DECLARE_SCOPE_CYCLE_COUNTER(
            TEXT("FRiveRendererDrawElement::Draw_RenderThread"),
            STAT_RIVE_RENDER_ELEMENT_DRAW,
            STATGROUP_Rive);

        SCOPED_GPU_STAT(GraphBuilder.RHICmdList, FRiveRendererDrawElementDraw);

        auto RiveArtboardLocal = RiveArtboard.Pin();
        auto RenderBoundsLocal = RenderBounds;

        const float TotalScale = Scale * DPIScale;

        if (!RiveArtboardLocal.IsValid())
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("FRiveRendererDrawElement::Draw_RenderThread Artboard "
                        "not set"));
            return;
        }
        // TODO: Find a way to create just one render target for the screen
        // texture rather than one per widget (these all end up with the same
        // target texture)
        if (RenderTarget.Get() == nullptr)
        {
            RenderTarget = RiveRenderer->CreateRenderTarget(
                GraphBuilder,
                "FRiveRendererDrawElement::Draw_RenderThread",
                Inputs.OutputTexture);
        }

        auto renderTarget = RenderTarget->GetRenderTarget();
        if (renderTarget->width() != Inputs.OutputTexture->Desc.Extent.X ||
            renderTarget->height() != Inputs.OutputTexture->Desc.Extent.Y)
        {
            RenderTarget = RiveRenderer->CreateRenderTarget(
                GraphBuilder,
                "FRiveRendererDrawElement::Draw_RenderThread",
                Inputs.OutputTexture);
            renderTarget = RenderTarget->GetRenderTarget();
        }

        RenderTarget->UpdateTargetTexture(Inputs.OutputTexture);

        if (!ensure(RenderTarget))
        {
            UE_LOG(LogRive,
                   Error,
                   TEXT("FRiveRendererDrawElement::Draw_RenderThread RDG "
                        "RenderTarget not supported on this platform"));
            return;
        }

        if (!ensure(renderTarget))
        {
            UE_LOG(
                LogRive,
                Error,
                TEXT("FRiveRendererDrawElement::Draw_RenderThread RDG "
                     "rive::gpu::RenderTarget not supported on this platform"));
            return;
        }

        auto ArtboardHandle = RiveArtboardLocal->GetNativeArtboardHandle();
        if (ArtboardHandle == RIVE_NULL_HANDLE)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("FRiveRendererDrawElement::Draw_RenderThread "
                        "ArtboardHandle is invalid"));
            return;
        }

        auto ArtboardInstance =
            CommandServer->getArtboardInstance(ArtboardHandle);
        if (ArtboardInstance == nullptr)
        {
            UE_LOG(LogRive,
                   Warning,
                   TEXT("FRiveRendererDrawElement::Draw_RenderThread "
                        "ArtboardInstance is invalid"));
            return;
        }

        auto Context = RiveRenderer->GetRenderContext();
        Context->beginFrame({
            .renderTargetWidth =
                static_cast<uint32_t>(Inputs.OutputTexture->Desc.GetSize().X),
            .renderTargetHeight =
                static_cast<uint32_t>(Inputs.OutputTexture->Desc.GetSize().Y),
            .loadAction = rive::gpu::LoadAction::preserveRenderTarget,
            .wireframe = Inputs.bWireFrame,
        });

        rive::RawPath ClipPath;
        ClipPath.addRect(AABBForSlateRect(ClipRect));
        auto ClipRenderPath =
            Context->makeRenderPath(ClipPath, rive::FillRule::nonZero);

        auto Renderer = MakeUnique<rive::RiveRenderer>(Context);
        Renderer->save();
        Renderer->clipPath(ClipRenderPath.get());
        Renderer->align(Fit,
                        Alignment,
                        AABBForSlateRect(RenderBoundsLocal),
                        ArtboardInstance->bounds(),
                        TotalScale);
        if (bDirty)
        {
            bDirty = false;
            if (Fit == rive::Fit::layout)
            {
                ArtboardInstance->width(RenderBoundsLocal.GetSize().X /
                                        TotalScale);
                ArtboardInstance->height(RenderBoundsLocal.GetSize().Y /
                                         TotalScale);
            }
            else
            {
                ArtboardInstance->resetSize();
            }

            // If we updated the size we need to advance for it to be applied
            if (auto NativeStateMachineHandle =
                    RiveArtboardLocal->GetStateMachineHandle();
                NativeStateMachineHandle != RIVE_NULL_HANDLE)
            {
                if (auto StateMachine = CommandServer->getStateMachineInstance(
                        NativeStateMachineHandle))
                {
                    StateMachine->advanceAndApply(0);
                }
            }
        }
        ArtboardInstance->draw(Renderer.Get());
        Renderer->restore();
        Context->flush({.renderTarget = renderTarget.get(),
                        .externalCommandBuffer = &GraphBuilder});
    }

    void SetArtboard(TWeakObjectPtr<URiveArtboard> InRiveArtboard)
    {
        RiveArtboard = InRiveArtboard;
        bDirty = true;
    }

    void SetFromDescriptor(const FRiveDescriptor& InRiveDescriptor)
    {
        Alignment = RiveAlignementToAlignment(InRiveDescriptor.Alignment);
        Fit = RiveFitTypeToFit(InRiveDescriptor.FitType);
        Scale = InRiveDescriptor.ScaleFactor;
        bDirty = true;
    }

    void SetDPIScale(float InDPIScale) { DPIScale = InDPIScale; }

    void SetRenderingBounds(const FSlateRect& InRenderBounds)
    {
        bDirty |= RenderBounds != InRenderBounds;
        RenderBounds = InRenderBounds;
    }

    void SetClipRect(const FSlateRect& InClipRect) { ClipRect = InClipRect; }

private:
    FRiveRenderer* RiveRenderer = nullptr;
    rive::CommandServer* CommandServer = nullptr;
    TSharedPtr<FRiveRenderTarget> RenderTarget;
    TWeakObjectPtr<URiveArtboard> RiveArtboard;
    // Locally copy of descriptor so we can pass it to the renderer
    rive::Alignment Alignment;
    rive::Fit Fit;
    float Scale = 1;
    float DPIScale = 1;
    FSlateRect RenderBounds;
    FSlateRect ClipRect;
    bool bDirty = true;
};

void SRiveLeafWidget::SetRiveDescriptor(const FRiveDescriptor& InDescriptor)
{
    RiveRendererDrawElement->SetFromDescriptor(InDescriptor);
    bScaleByDPI = InDescriptor.bScaleLayoutByDPI;
}

void SRiveLeafWidget::SetArtboard(TWeakObjectPtr<URiveArtboard> InArtboard)
{
    Artboard = InArtboard;
    RiveRendererDrawElement->SetArtboard(InArtboard);
}

void SRiveLeafWidget::Construct(const FArguments& InArgs)
{
    FRiveDescriptor Descriptor;
    if (InArgs._OwningWidget.IsSet())
        OwningWidget = InArgs._OwningWidget.Get();
    if (InArgs._Artboard.IsSet())
        Artboard = InArgs._Artboard.Get();
    if (InArgs._Descriptor.IsSet())
        Descriptor = InArgs._Descriptor.Get();

    RiveRendererDrawElement = MakeShared<FRiveRendererDrawElement>(Artboard);
    RiveRendererDrawElement->SetFromDescriptor(Descriptor);
}

int32 SRiveLeafWidget::OnPaint(const FPaintArgs& Args,
                               const FGeometry& AllottedGeometry,
                               const FSlateRect& MyCullingRect,
                               FSlateWindowElementList& OutDrawElements,
                               int32 LayerId,
                               const FWidgetStyle& InWidgetStyle,
                               bool bParentEnabled) const
{
    if (!bParentEnabled)
        return LayerId;

    // Don't try to draw if we don't have an artboard.
    if (!Artboard.IsValid())
        return LayerId;

    if (bScaleByDPI && IsValid(OwningWidget))
    {
        const float Scale =
            UWidgetLayoutLibrary::GetViewportScale(OwningWidget);
        RiveRendererDrawElement->SetDPIScale(Scale);
    }
    else
    {
        RiveRendererDrawElement->SetDPIScale(1.0f);
    }

    RiveRendererDrawElement->SetRenderingBounds(
        AllottedGeometry.GetRenderBoundingRect());
    RiveRendererDrawElement->SetClipRect(MyCullingRect);
    FSlateDrawElement::MakeCustom(OutDrawElements,
                                  LayerId,
                                  RiveRendererDrawElement);
    return LayerId;
}

FVector2D SRiveLeafWidget::ComputeDesiredSize(float) const
{
    if (auto StrArtboard = Artboard.Pin())
    {
        if (StrArtboard->HasData())
        {
            return StrArtboard->ArtboardDefaultSize;
        }
    }

    return FVector2D(256, 256);
}
