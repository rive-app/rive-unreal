// Copyright 2024-2026 Rive, Inc. All rights reserved.

#pragma once
#include <memory>

#include "RiveCommandBuilder.h"
#include "Engine/Texture2DDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UnrealClient.h"
#include "RenderGraphFwd.h"

class FRiveRenderTargetRHI;

namespace rive::gpu
{
class RenderTarget;
}

THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive
{
class CommandQueue;
class CommandServer;
} // namespace rive

namespace rive::gpu
{
class Renderer;
class RenderContext;
} // namespace rive::gpu

class FRiveRenderTarget;

class RIVERENDERER_API FRiveRenderer
{
public:
    FRiveRenderer();
    ~FRiveRenderer();

    TSharedPtr<FRiveRenderTargetRHI> CreateRenderTarget(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget);

    TSharedPtr<FRiveRenderTargetRHI> CreateRenderTarget(
        const FName& InRiveName,
        UTextureRenderTarget2D* InRenderTarget);

    TSharedPtr<FRiveRenderTargetRHI> CreateRenderTarget(
        FRDGBuilder& GraphBuilder,
        const FName& InRiveName,
        FRDGTextureRef InRenderTarget);

    TSharedPtr<FRiveRenderTargetRHI> CreateRenderTarget(
        const FName& InRiveName,
        FRenderTarget* RenderTarget);

    void CreateRenderContext(FRHICommandListImmediate& RHICmdList);

    rive::gpu::RenderContext* GetRenderContext();

    void BeginFrameRenderThread();
    void BeginFrameGameThread();
    void EndFrameGameThread();

    FRiveCommandBuilder& GetCommandBuilder()
    {
        check(IsInGameThread());
        return CommandBuilder;
    }

    rive::CommandServer* GetCommandServer() const
    {
        check(IsInRenderingThread());
        return CommandServer.Get();
    }

    // This skips the check for rendering thread. Only use this if you know you
    // will only use the returned pointer inside of the rendering thread and not
    // the one it's gotten from
    rive::CommandServer* GetCommandServerUnsafe() const
    {
        return CommandServer.Get();
    }

private:
    std::unique_ptr<rive::gpu::RenderContext> RenderContext;
    TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

    FDelegateHandle OnBeingFrameRenderThreadHandle;
    FDelegateHandle OnBeingFrameGameThreadHandle;
    FDelegateHandle OnEndFrameGameThreadHandle;
    TUniquePtr<rive::CommandServer> CommandServer;
    rive::rcp<rive::CommandQueue> CommandQueue;
    FRiveCommandBuilder CommandBuilder;
};
