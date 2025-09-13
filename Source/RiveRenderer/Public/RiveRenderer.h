// Copyright 2024, 2025 Rive, Inc. All rights reserved.

#pragma once

#include <memory>

#include "RiveCommandBuilder.h"
#include "Engine/Texture2DDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UnrealClient.h"
#include "RenderGraphFwd.h"

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

class FRiveRenderer
{
public:
    FRiveRenderer();

    virtual ~FRiveRenderer();
    virtual void Initialize() {};

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

    virtual rive::gpu::RenderContext* GetRenderContext();

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) = 0;

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FName& InRiveName,
        UTextureRenderTarget2D* InRenderTarget) = 0;

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FName& InRiveName,
        FRenderTarget* RenderTarget);

    virtual TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        FRDGBuilder& GraphBuilder,
        const FName& InRiveName,
        FRDGTextureRef InRenderTarget);

protected:
    virtual void CreateRenderContext(FRHICommandListImmediate& RHICmdList) = 0;

    std::unique_ptr<rive::gpu::RenderContext> RenderContext;
    TMap<FName, TSharedPtr<FRiveRenderTarget>> RenderTargets;

private:
    FDelegateHandle OnBeingFrameRenderThreadHandle;
    FDelegateHandle OnBeingFrameGameThreadHandle;
    FDelegateHandle OnEndFrameGameThreadHandle;
    TUniquePtr<rive::CommandServer> CommandServer;
    rive::rcp<rive::CommandQueue> CommandQueue;
    FRiveCommandBuilder CommandBuilder;
};
