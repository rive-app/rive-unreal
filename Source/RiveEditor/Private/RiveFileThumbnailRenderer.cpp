// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#include "RiveFileThumbnailRenderer.h"

#include "CanvasTypes.h"
#include "IRiveRendererModule.h"
#include "RenderGraphBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveTexture.h"
#include "RiveRenderer.h"
#include "RiveRenderTarget.h"
#include "Logs/RiveEditorLog.h"
THIRD_PARTY_INCLUDES_START
#include "rive/command_server.hpp"
#include "rive/renderer/gpu.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/rive_renderer.hpp"
THIRD_PARTY_INCLUDES_END

URiveFileThumbnailRenderer::URiveFileThumbnailRenderer() {}

bool URiveFileThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
    return Object->IsA<URiveFile>();
}

EThumbnailRenderFrequency URiveFileThumbnailRenderer::
    GetThumbnailRenderFrequency(UObject* Object) const
{
    return EThumbnailRenderFrequency::OnAssetSave;
}

void URiveFileThumbnailRenderer::Draw(UObject* Object,
                                      int32 X,
                                      int32 Y,
                                      uint32 Width,
                                      uint32 Height,
                                      FRenderTarget* Viewport,
                                      FCanvas* Canvas,
                                      bool bAdditionalViewFamily)
{
    if (URiveFile* RiveFile = Cast<URiveFile>(Object))
    {
        FRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

        if (!RiveRenderTarget)
        {
            RiveRenderTarget =
                RiveRenderer->CreateRenderTarget(GetFName(), Viewport);
        }
        else
        {
            RiveRenderTarget->UpdateTargetTexture(Viewport);
        }

        auto NativeFileHandle = RiveFile->GetNativeFileHandle();

        FBox2f AlignmentBox{
            {static_cast<float>(X), static_cast<float>(Y)},
            {static_cast<float>(Width), static_cast<float>(Height)}};

        // This completly bypasses the command server / queue. This is because
        // we have to render and return now. Any delay prevents the icon from
        // rendering.
        ENQUEUE_RENDER_COMMAND(URiveFileThumbnailRenderer_Draw)
        ([RiveRenderer,
          AlignmentBox,
          NativeFileHandle,
          RiveRenderTarget =
              RiveRenderTarget](FRHICommandListImmediate& RHICmdList) {
            auto CommandServer = RiveRenderer->GetCommandServer();
            auto RenderTarget = RiveRenderTarget->GetRenderTarget();

            if (!CommandServer)
            {
                UE_LOG(
                    LogRiveEditor,
                    Error,
                    TEXT(
                        "URiveFileThumbnailRenderer::Draw No Command Server !"));
                return;
            }

            if (!RenderTarget)
            {
                UE_LOG(
                    LogRiveEditor,
                    Error,
                    TEXT(
                        "URiveFileThumbnailRenderer::Draw No Render Target !"));
                return;
            }

            auto File = CommandServer->getFile(NativeFileHandle);

            if (!File)
            {
                UE_LOG(LogRiveEditor,
                       Error,
                       TEXT("URiveFileThumbnailRenderer::Draw Invalid File"));
                return;
            }

            auto ArtboardInstance = File->artboardDefault();

            if (!ArtboardInstance)
            {
                UE_LOG(LogRiveEditor,
                       Error,
                       TEXT("URiveFileThumbnailRenderer::Draw Invalid Default "
                            "Artboard Instance"));
                return;
            }

            ArtboardInstance->advance(1.0);

            auto Context = RiveRenderer->GetRenderContext();
            Context->beginFrame({
                .renderTargetWidth =
                    static_cast<uint32_t>(AlignmentBox.GetSize().X),
                .renderTargetHeight =
                    static_cast<uint32_t>(AlignmentBox.GetSize().Y),
                .loadAction = rive::gpu::LoadAction::clear,
                .clearColor = rive::colorRed(255),
                .wireframe = false,
            });

            auto Renderer = MakeUnique<rive::RiveRenderer>(Context);
            Renderer->save();
            Renderer->align(rive::Fit::contain,
                            rive::Alignment::center,
                            {AlignmentBox.Min.X,
                             AlignmentBox.Min.Y,
                             AlignmentBox.Max.X,
                             AlignmentBox.Max.Y},
                            ArtboardInstance->bounds());
            ArtboardInstance->draw(Renderer.Get());
            Renderer->restore();
            FRDGBuilder GraphBuilder(RHICmdList);
            Context->flush({.renderTarget = RenderTarget.get(),
                            .externalCommandBuffer = &GraphBuilder});
            GraphBuilder.Execute();
        });
    }
    Canvas->SetRenderTargetDirty(true);
}
