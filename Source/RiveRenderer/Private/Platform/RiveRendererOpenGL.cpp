// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererOpenGL.h"

#if PLATFORM_ANDROID
#include "IRiveRendererModule.h"
#include "OpenGLDrv.h"
#include "DynamicRHI.h"
#include "RiveRenderTargetOpenGL.h"
#include "Logs/RiveRendererLog.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/pls/gl/pls_render_context_gl_impl.hpp"
#include "rive/pls/pls_renderer.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

// ---------------------------------------------------
// --------------- FRiveRendererOpenGL ---------------
// ---------------------------------------------------

UE::Rive::Renderer::Private::FRiveRendererOpenGL::FRiveRendererOpenGL()
{
	RIVE_DEBUG_FUNCTION_INDENT;
}

UE::Rive::Renderer::Private::FRiveRendererOpenGL::~FRiveRendererOpenGL()
{
	RIVE_DEBUG_FUNCTION_INDENT;
}

void UE::Rive::Renderer::Private::FRiveRendererOpenGL::Initialize()
{
	check(IsInGameThread());
	RIVE_DEBUG_FUNCTION_INDENT
	
	{
		FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
		RIVE_DEBUG_VERBOSE("GAMETHREAD: glVersionStr %s", *glVersionStr);
            
		ENQUEUE_RENDER_COMMAND(URiveFileInitialize_RenderThread)(
		[](FRHICommandListImmediate& RHICmdList)
		{
			FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
			RIVE_DEBUG_VERBOSE("RENDERTHREAD: glVersionStr %s", *glVersionStr);
			RHICmdList.EnqueueLambda([](FRHICommandListImmediate&)
			{
				FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
				RIVE_DEBUG_VERBOSE("RHITHREAD: glVersionStr %s", *glVersionStr);
			});
		});
	}

	if (IRiveRendererModule::RunInGameThread())
	{
		FScopeLock Lock(&ThreadDataCS);
		
		CreatePLSContext_GameThread();
		// CreatePLSRenderer_RenderThread(RHICmdList);

		// Should give more data how that was initialized
		bIsInitialized = true;
	}
	else
	{
		FRiveRenderer::Initialize();
	}
}

rive::pls::PLSRenderContext* UE::Rive::Renderer::Private::FRiveRendererOpenGL::GetPLSRenderContextPtr()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	if (ensure(GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::OpenGL))
	{
		FScopeLock Lock(&ContextsCS);
		if (PLSRenderContext)
		{
			return PLSRenderContext.get();
		}
	}
	
	UE_LOG(LogRiveRenderer, Error, TEXT("Rive PLS Render Context is uninitialized for the Current OpenGL Context."));
	return nullptr;
}

TSharedPtr<UE::Rive::Renderer::IRiveRenderTarget> UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreateTextureTarget_GameThread(const FName& InRiveName, UTextureRenderTarget2D* InRenderTarget)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread());
	
	const TSharedPtr<FRiveRenderTargetOpenGL> RiveRenderTarget = MakeShared<FRiveRenderTargetOpenGL>(SharedThis(this), InRiveName, InRenderTarget);
	RenderTargets.Add(InRiveName, RiveRenderTarget);
	
	return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreatePLSContext, TEXT("CreatePLSContext_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInRenderingThread());

	RHICmdList.EnqueueLambda(
		GET_FUNCTION_NAME_STRING_CHECKED(FRiveRendererOpenGL, GetOrCreatePLSRenderContextPtr_Internal),
		[this](FRHICommandListImmediate& RHICmdList)
		{
			GetOrCreatePLSRenderContextPtr_Internal();
		});
}

void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSContext_GameThread()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread());
	
	GetOrCreatePLSRenderContextPtr_Internal();
}

DECLARE_GPU_STAT_NAMED(CreatePLSRenderer, TEXT("CreatePLSRenderer_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	RIVE_DEBUG_FUNCTION_INDENT;
	//checkf(false, TEXT("This function should not be called directly for OpenGL"));
}

rive::pls::PLSRenderContext* UE::Rive::Renderer::Private::FRiveRendererOpenGL::GetOrCreatePLSRenderContextPtr_Internal()
{
	RIVE_DEBUG_FUNCTION_INDENT;
	check(IsInGameThread() || IsInRHIThread());
	ENABLE_VERIFY_GL_THREAD;
	
	if (ensure(IsRHIOpenGL()))
	{
#if WITH_RIVE
		FScopeLock Lock(&ContextsCS);
		if (PLSRenderContext)
		{
			return PLSRenderContext.get();
		}
		
		
		RIVE_DEBUG_VERBOSE("--- OpenGL Console Variables ---");
		RIVE_DEBUG_VERBOSE(" - FOpenGL::SupportsBufferStorage():  %s", FOpenGL::SupportsBufferStorage() ? TEXT("TRUE") : TEXT("false"));
		RIVE_DEBUG_VERBOSE(" - FOpenGL::DiscardFrameBufferToResize():  %s", FOpenGL::DiscardFrameBufferToResize() ? TEXT("TRUE") : TEXT("false"));
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UsePersistentMappingStagingBuffer")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UsePersistentMappingStagingBuffer`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseStagingBuffer")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UseStagingBuffer`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UBODirectWrite")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UBODirectWrite`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseMapBuffer")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UseMapBuffer`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseBufferDiscard")))
		{
			RIVE_DEBUG_VERBOSE(" - `OpenGL.UseBufferDiscard`:  %s", CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		RIVE_DEBUG_VERBOSE(" - `GUseThreadedRendering`:  %s", GUseThreadedRendering ? TEXT("TRUE") : TEXT("false"));
		
		
		PLSRenderContext = rive::pls::PLSRenderContextGLImpl::MakeContext();
		RIVE_DEBUG_VERBOSE("PLSRenderContext: %p", PLSRenderContext.get());
		
		if (!ensure(PLSRenderContext.get() != nullptr))
		{
			UE_LOG(LogRiveRenderer, Error, TEXT("Not able to create an OpenGL PLS Render Context"))
		}
		return PLSRenderContext.get();
#endif // WITH_RIVE
	}
	return nullptr;
}

bool UE::Rive::Renderer::Private::FRiveRendererOpenGL::IsRHIOpenGL()
{
	return GDynamicRHI != nullptr && GDynamicRHI->GetInterfaceType() == ERHIInterfaceType::OpenGL;
}

UE_ENABLE_OPTIMIZATION

#endif // PLATFORM_ANDROID