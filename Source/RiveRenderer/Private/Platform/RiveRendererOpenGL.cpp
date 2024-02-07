// Copyright Rive, Inc. All rights reserved.

#include "RiveRendererOpenGL.h"

#if PLATFORM_ANDROID
#include "IRiveRendererModule.h"
#include "OpenGLDrv.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "OpenGLDrv/Public/IOpenGLDynamicRHI.h"
#include "CanvasTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DynamicRHI.h"
#include "RiveRenderTargetOpenGL.h"
#include "IOpenGLDynamicRHI.h"
#include "Logs/RiveRendererLog.h"

#if WITH_RIVE
THIRD_PARTY_INCLUDES_START
#include "rive/artboard.hpp"
#include "rive/pls/gl/pls_render_context_gl_impl.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/file.hpp"
#include "rive/pls/pls_image.hpp"
THIRD_PARTY_INCLUDES_END
#endif // WITH_RIVE

UE_DISABLE_OPTIMIZATION

// ---------------------------------------------------
// --------------- FRiveRendererOpenGL ---------------
// ---------------------------------------------------

UE::Rive::Renderer::Private::FRiveRendererOpenGL::FRiveRendererOpenGL()
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s === FRiveRendererOpenGL::FRiveRendererOpenGL"), FDebugLogger::Ind());
}

UE::Rive::Renderer::Private::FRiveRendererOpenGL::~FRiveRendererOpenGL()
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s ~~~ FRiveRendererOpenGL::~FRiveRendererOpenGL"), FDebugLogger::Ind());
}

void UE::Rive::Renderer::Private::FRiveRendererOpenGL::Initialize()
{
	check(IsInGameThread());
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::Initialize()"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};

	{
		IOpenGLDynamicRHI* OpenGLDynamicRHI = GetIOpenGLDynamicRHI();
		auto Context = OpenGLDynamicRHI->RHIGetEGLContext();
		UE_LOG(LogRiveRenderer, Error, TEXT("GAMETHREAD: EGLContext %p"), Context);
		FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
		UE_LOG(LogRiveRenderer, Error, TEXT("GAMETHREAD: glVersionStr %s"), *glVersionStr);
            
		ENQUEUE_RENDER_COMMAND(URiveFileInitialize_RenderThread)(
		[](FRHICommandListImmediate& RHICmdList)
		{
			IOpenGLDynamicRHI* OpenGLDynamicRHI = GetIOpenGLDynamicRHI();
			auto Context = OpenGLDynamicRHI->RHIGetEGLContext();
			UE_LOG(LogRiveRenderer, Error, TEXT("RENDERTHREAD: EGLContext %p"), Context);
			FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
			UE_LOG(LogRiveRenderer, Error, TEXT("RENDERTHREAD: glVersionStr %s"), *glVersionStr);
			RHICmdList.EnqueueLambda([](FRHICommandListImmediate&)
			{
				IOpenGLDynamicRHI* OpenGLDynamicRHI = GetIOpenGLDynamicRHI();
				auto Context = OpenGLDynamicRHI->RHIGetEGLContext();
				UE_LOG(LogRiveRenderer, Error, TEXT("RHITHREAD: EGLContext %p"), Context);
				FString glVersionStr = ANSI_TO_TCHAR((const ANSICHAR*) glGetString(GL_VERSION));
				UE_LOG(LogRiveRenderer, Error, TEXT("RHITHREAD: glVersionStr %s"), *glVersionStr);
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
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::GetPLSRenderContextPtr()   %s %p"), FDebugLogger::Ind(), IsInGameThread() ? TEXT("GAME THREAD") : (IsInRHIThread() ? TEXT("RHI THREAD") : TEXT("OTHER THREAD")), this); FScopeLogIndent LogIndent{};
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
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::CreateTextureTarget_GameThread()"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	check(IsInGameThread());
	
	const TSharedPtr<FRiveRenderTargetOpenGL> RiveRenderTarget = MakeShared<FRiveRenderTargetOpenGL>(SharedThis(this), InRiveName, InRenderTarget);
	RenderTargets.Add(InRiveName, RiveRenderTarget);
	
	return RiveRenderTarget;
}

DECLARE_GPU_STAT_NAMED(CreatePLSContext, TEXT("CreatePLSContext_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSContext_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	check(IsInRenderingThread());
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::CreatePLSContext_RenderThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};

	RHICmdList.EnqueueLambda(
		GET_FUNCTION_NAME_STRING_CHECKED(FRiveRendererOpenGL,GetOrCreatePLSRenderContextPtr_Internal),
		[this](FRHICommandListImmediate& RHICmdList)
		{
			GetOrCreatePLSRenderContextPtr_Internal();
		});
}

void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSContext_GameThread()
{
	check(IsInGameThread());
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::CreatePLSContext_GameThread"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
	
	GetOrCreatePLSRenderContextPtr_Internal();
}

DECLARE_GPU_STAT_NAMED(CreatePLSRenderer, TEXT("CreatePLSRenderer_RenderThread"));
void UE::Rive::Renderer::Private::FRiveRendererOpenGL::CreatePLSRenderer_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::CreatePLSRenderer_RenderThread -skipping"), FDebugLogger::Ind()); FScopeLogIndent LogIndent{};
}

rive::pls::PLSRenderContext* UE::Rive::Renderer::Private::FRiveRendererOpenGL::GetOrCreatePLSRenderContextPtr_Internal()
{
	UE_LOG(LogRiveRenderer, Warning, TEXT("%s-- FRiveRendererOpenGL::CreatePLSContext_Internal  [%s]"), FDebugLogger::Ind(), *FDebugLogger::CurrentThread()); FScopeLogIndent LogIndent{};
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
		
		auto Context = GetIOpenGLDynamicRHI()->RHIGetEGLContext();
		
		UE_LOG(LogRiveRenderer, Warning, TEXT("%s   --- OpenGL Console Variables ---"), FDebugLogger::Ind(), Context);
		bool bSupportsBufferStorage = FOpenGL::SupportsBufferStorage();
		UE_LOG(LogRiveRenderer, Display, TEXT("%s - FOpenGL::SupportsBufferStorage():  %s"), FDebugLogger::Ind(), bSupportsBufferStorage ? TEXT("TRUE") : TEXT("false"));
		bool bDiscardFrameBufferToResize = FOpenGL::DiscardFrameBufferToResize();
		UE_LOG(LogRiveRenderer, Display, TEXT("%s - FOpenGL::DiscardFrameBufferToResize():  %s"), FDebugLogger::Ind(), bDiscardFrameBufferToResize ? TEXT("TRUE") : TEXT("false"));
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UsePersistentMappingStagingBuffer")))
		{
			UE_LOG(LogRiveRenderer, Display, TEXT("%s - `OpenGL.UsePersistentMappingStagingBuffer`:  %s"), FDebugLogger::Ind(), CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseStagingBuffer")))
		{
			UE_LOG(LogRiveRenderer, Display, TEXT("%s - `OpenGL.UseStagingBuffer`:  %s"), FDebugLogger::Ind(), CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UBODirectWrite")))
		{
			UE_LOG(LogRiveRenderer, Display, TEXT("%s - `OpenGL.UBODirectWrite`:  %s"), FDebugLogger::Ind(), CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseMapBuffer")))
		{
			UE_LOG(LogRiveRenderer, Display, TEXT("%s - `OpenGL.UseMapBuffer`:  %s"), FDebugLogger::Ind(), CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("OpenGL.UseBufferDiscard")))
		{
			UE_LOG(LogRiveRenderer, Display, TEXT("%s - `OpenGL.UseBufferDiscard`:  %s"), FDebugLogger::Ind(), CVar->GetBool() ? TEXT("TRUE") : TEXT("false"));
		}
		
		UE_LOG(LogRiveRenderer, Display, TEXT("%s - `GUseThreadedRendering`:  %s"), FDebugLogger::Ind(), GUseThreadedRendering ? TEXT("TRUE") : TEXT("false"));
		
		
		UE_LOG(LogRiveRenderer, Display, TEXT("%s Dynamic OpenGL RHI: Creating PLS Render Context in EGL Context: %p on RHI THREAD"), FDebugLogger::Ind(), Context);
		PLSRenderContext = rive::pls::PLSRenderContextGLImpl::MakeContext();
		UE_LOG(LogRiveRenderer, Display, TEXT("%s PLSRenderContext: %p"), FDebugLogger::Ind(), PLSRenderContext.get());
		
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