// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_APPLE

#include "RiveRenderTarget.h"

#if WITH_RIVE
#include "RiveCore/Public/PreRiveHeaders.h"
THIRD_PARTY_INCLUDES_START
#include "rive/refcnt.hpp"
THIRD_PARTY_INCLUDES_END

namespace rive::pls
{
    class PLSRenderTargetMetal;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
    /**
     *
     */
    class FRiveRenderTargetMetal final : public FRiveRenderTarget
    {
        /**
         * Structor(s)
         */
        
    public:
        
        FRiveRenderTargetMetal(const TSharedRef<FRiveRenderer>& InRiveRenderer, const FName& InRiveName, UTexture2DDynamic* InRenderTarget);
        virtual ~FRiveRenderTargetMetal() override;
        //~ BEGIN : IRiveRenderTarget Interface
        
        virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) override;
        
#if WITH_RIVE
        
        //~ END : IRiveRenderTarget Interface
        
        //~ BEGIN : FRiveRenderTarget Interface
        
    protected:

        virtual rive::rcp<rive::pls::PLSRenderTarget> GetRenderTarget() const override;
        virtual void EndFrame() const override;
        //~ END : FRiveRenderTarget Interface
        
        /**
         * Attribute(s)
         */
        
    private:
        
        rive::rcp<rive::pls::PLSRenderTargetMetal> CachedPLSRenderTargetMetal;
        
#endif // WITH_RIVE
        
        mutable bool bIsCleared = false;
    };
}

#endif // PLATFORM_APPLE
