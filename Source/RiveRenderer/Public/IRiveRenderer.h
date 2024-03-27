// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "IRiveRenderTarget.h"

class URiveFile;
class UTexture2DDynamic;
class UTextureRenderTarget2D;

#if WITH_RIVE

// just for testig
namespace rive
{
    class Artboard;

    namespace pls
    {
        class PLSRenderContext;
        class PLSRenderer;
    }
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer
{
    class IRiveRenderer;

    /**
     * Type definition for shared pointer reference to the instance of IRiveRenderer.
     */
    using IRiveRendererPtr = TSharedPtr<IRiveRenderer>;

    /**
     * Type definition for shared pointer reference to the instance of IRiveRenderer.
     */
    using IRiveRendererRef = TSharedRef<IRiveRenderer>;

    class IRiveRenderer : public TSharedFromThis<IRiveRenderer>
    {
        /**
         * Structor(s)
         */

    public:
        virtual ~IRiveRenderer() {}
        
        DECLARE_MULTICAST_DELEGATE_OneParam(FOnRendererInitialized, IRiveRenderer* /* RiveRenderer */);

        /**
         * Implementation(s)
         */

    public:

        virtual void Initialize() = 0;

        virtual bool IsInitialized() const = 0;
        
        virtual void QueueTextureRendering(TObjectPtr<URiveFile> InRiveFile) = 0;

        virtual IRiveRenderTargetPtr CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget) = 0;
        
        virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) = 0;

        virtual UTextureRenderTarget2D* CreateDefaultRenderTarget(FIntPoint InTargetSize) = 0;

        virtual FCriticalSection& GetThreadDataCS() = 0;

        virtual void CallOrRegister_OnInitialized(FOnRendererInitialized::FDelegate&& Delegate) = 0;
    
#if WITH_RIVE

        virtual rive::pls::PLSRenderContext* GetPLSRenderContextPtr() = 0;

#endif // WITH_RIVE
    };
}
