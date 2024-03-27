// Copyright Rive, Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_ANDROID

#include "RiveRenderer.h"

#if WITH_RIVE
#include "IOpenGLDynamicRHI.h"

class FOpenGLBase;

namespace rive
{
	class File;
}

namespace rive::pls
{
	class PLSImage;
	class PLSRenderTargetGL;
}

#endif // WITH_RIVE

namespace UE::Rive::Renderer::Private
{
	
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
		virtual rive::pls::PLSRenderContext* GetPLSRenderContextPtr() override;
		virtual IRiveRenderTargetPtr CreateTextureTarget_GameThread(const FName& InRiveName, UTexture2DDynamic* InRenderTarget) override;
		virtual void CreatePLSContext_GameThread();
		virtual void CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList) override;
		//~ END : IRiveRenderer Interface
		
		virtual rive::pls::PLSRenderContext* GetOrCreatePLSRenderContextPtr_Internal();
#endif // WITH_RIVE
		static bool IsRHIOpenGL();
	private:
		mutable FCriticalSection ContextsCS;

	public:
		static void DebugLogOpenGLStatus();
	};
}

#endif // PLATFORM_ANDROID
