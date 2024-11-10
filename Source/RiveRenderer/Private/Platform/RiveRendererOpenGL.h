// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_ANDROID

#include "RiveRenderer.h"
#include "IOpenGLDynamicRHI.h"

class FOpenGLBase;

class RIVERENDERER_API FRiveRendererOpenGL : public FRiveRenderer
{
    /**
     * Structor(s)
     */
public:
    //~ BEGIN : IRiveRenderer Interface
public:
    virtual void Initialize() override;
#if WITH_RIVE
    virtual rive::gpu::RenderContext* GetRenderContext() override;
    virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(
        const FName& InRiveName,
        UTexture2DDynamic* InRenderTarget) override;
    virtual void CreateRenderContext_GameThread();
    virtual void CreateRenderContext_RenderThread(
        FRHICommandListImmediate& RHICmdList) override;
    //~ END : IRiveRenderer Interface

    virtual rive::gpu::RenderContext* GetOrCreateRenderContext_Internal();
#endif // WITH_RIVE
    static bool IsRHIOpenGL();

private:
    mutable FCriticalSection ContextsCS;

public:
    static void DebugLogOpenGLStatus();
};

#endif // PLATFORM_ANDROID
