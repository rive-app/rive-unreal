// Copyright 2024-2026 Rive, Inc. All rights reserved.

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
#undef PI
#include "rive/refcnt.hpp"
#include "rive/renderer/cmd/deferred_host.hpp"
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

    TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FString& InRiveName,
        UTexture2DDynamic* InRenderTarget);

    TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FString& InRiveName,
        UTextureRenderTarget2D* InRenderTarget);

    TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        FRDGBuilder& GraphBuilder,
        const FString& InRiveName,
        FRDGTextureRef InRenderTarget);

    TSharedPtr<FRiveRenderTarget> CreateRenderTarget(
        const FString& InRiveName,
        FRenderTarget* RenderTarget);

    void CreateRenderContext(FRHICommandListImmediate& RHICmdList);

    rive::gpu::RenderContext* GetRenderContext();

    // Opens this frame's recording and returns the recorder every draw in it
    // goes through. The recorder is the session's, so it outlives the frame;
    // ReplayDeferredFrame closes the recording and issues the real draws.
    rive::Renderer* BeginDeferredFrame();

    // BeginScreen opens the real frame for the target being presented, and
    // Present issues its flush, running only when replay reached the screen.
    // Canvas passes replayed along the way flush into GraphBuilder, so it has
    // to outlive the call.
    void ReplayDeferredFrame(
        FRDGBuilder& GraphBuilder,
        TFunctionRef<TUniquePtr<rive::Renderer>()> BeginScreen,
        TFunctionRef<void()> Present);

    // Replays onto one render target, which opens and flushes its own frame.
    void ReplayDeferredFrame(const TSharedPtr<FRiveRenderTarget>& RenderTarget);

    // The factory every file, artboard and render resource is created
    // through, and the stream their draws record into.
    rive::cmd::DeferredSession* GetDeferredSession() const
    {
        return DeferredSession.Get();
    }

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
    rive::cmd::DeferredInlineHost InlineHost;
    TUniquePtr<rive::cmd::DeferredSession> DeferredSession;
    TUniquePtr<rive::CommandServer> CommandServer;
    rive::rcp<rive::CommandQueue> CommandQueue;
    FRiveCommandBuilder CommandBuilder;
};
