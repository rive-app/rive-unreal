// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_APPLE

#include "RiveRenderer.h"

#if WITH_RIVE

namespace rive::pls
{
	class PLSRenderTargetMetal;

	class PLSRenderContextMetalImpl;
}

#endif // WITH_RIVE

/**
 *
 */
class RIVERENDERER_API FRiveRendererMetal : public FRiveRenderer
{
	/**
	 * Structor(s)
	 */

public:
	//~ BEGIN : IRiveRenderer Interface

public:
	virtual TSharedPtr<IRiveRenderTarget> CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget) override;

	virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;

	//~ END : IRiveRenderer Interface
};

#endif // PLATFORM_APPLE
